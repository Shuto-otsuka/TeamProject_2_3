import os
import re
import struct
import subprocess
import tempfile

LABEL = "シェーダ事前コンパイル"

EXTENSIONS = set()

# Release の ShaderCompiler(GraphicsEngine/Shader/ShaderCompiler.cpp) と同じプロファイルとオプション。
# シェーダ種別はファイル名の接尾辞で決まり、*RT.hlsl のように接尾辞で決まらないものは
# 内容で判定する（[shader("...")] があれば DXR ライブラリ、[NumThreads] があれば CS）。
SHADER_PROFILE_BY_SUFFIX = {
    "VS": "vs_6_6",
    "HS": "hs_6_6",
    "DS": "ds_6_6",
    "GS": "gs_6_6",
    "PS": "ps_6_6",
    "AS": "as_6_6",
    "MS": "ms_6_6",
    "CS": "cs_6_6",
}
SHADER_COMPILE_OPTIONS = ["-HV", "2021", "-Zi", "-Qstrip_debug", "-O3"]
SHADER_LIBRARY_PATTERN = re.compile(r'\[\s*shader\s*\(\s*"', re.IGNORECASE)
SHADER_NUMTHREADS_PATTERN = re.compile(r'\[\s*numthreads', re.IGNORECASE)

# CS のエントリポイント = [NumThreads(...)] の付いた関数（属性は大文字小文字を区別しない）。
# 1ファイルに複数のエントリを持つ CS（ブラー各パス、A-Trous 各パス等）もこれで全て拾う。
SHADER_COMPUTE_ENTRY_PATTERN = re.compile(r'\[\s*numthreads\s*\([^)]*\)\s*\]\s*(?:\[[^\]]*\]\s*)*void\s+([A-Za-z_]\w*)\s*\(', re.IGNORECASE)

# PS のエントリポイントは、パイプラインの記述(GraphicsEngine の *.h の struct)が
# pixelFile_ と一緒に pixelEntryPoint_ で指定したもの（無ければ main）。
# 1ファイルに複数のエントリを持つ PS（GeometryPS の Depth / Visibility 等）もこれで全て拾う。
SHADER_PIXEL_FILE_PATTERN = re.compile(r'\bpixelFile_\s*=\s*ShaderPath::Engine\s*\(\s*"([^"]+)"\s*\)')
SHADER_PIXEL_ENTRY_PATTERN = re.compile(r'\bpixelEntryPoint_\s*=\s*"([^"]+)"')
SHADER_MAIN_PATTERN = re.compile(r'\bmain\s*\(')

# エンジンが実行時に読むシェーダ = GraphicsEngine の C++ ソースに書かれた "GraphicsEngine/....hlsl"
# の文字列（ShaderPath::Engine(...) でも、先頭に "../" の付いた直接のパスでも）。
# 使われていない古いシェーダや開発中で未接続のシェーダは事前コンパイルしない。
SHADER_REFERENCE_PATTERN = re.compile(r'"(?:\.\./)?(GraphicsEngine/[^"]+\.hlsl)"')


def seed(context):
    """
    GraphicsEngine 以下の .hlsl のうち、エンジンの C++ ソースから参照されているもの
    (SHADER_REFERENCE_PATTERN)と Zephyr の Generated を集める。
    コンパイルは collect が1ファイルずつ、Release の ShaderCompiler と同じ
    プロファイル/オプションで External/DXC/dxc.exe により行う。
    """
    project_root = context.project_root
    dxc_path = os.path.join(project_root, "External", "DXC", "dxc.exe")
    if not os.path.exists(dxc_path):
        print(f"dxc.exe が見つかりません: {dxc_path}")
        return None

    pixel_entries = {}
    referenced_shaders = set()
    for root, _, files in os.walk(os.path.join(project_root, "GraphicsEngine")):
        for file in files:
            if not (file.endswith(".h") or file.endswith(".cpp")):
                continue
            with open(os.path.join(root, file), 'r', encoding='utf-8', errors='ignore') as f:
                header = context.strip_comments(f.read())
            for reference in SHADER_REFERENCE_PATTERN.findall(header):
                referenced_shaders.add(os.path.normcase(os.path.normpath(os.path.join(project_root, reference))))
            for description in re.split(r'\bstruct\b', header):
                pixel_file = SHADER_PIXEL_FILE_PATTERN.search(description)
                pixel_entry = SHADER_PIXEL_ENTRY_PATTERN.search(description)
                if pixel_file and pixel_entry:
                    pixel_path = os.path.normcase(os.path.normpath(os.path.join(project_root, pixel_file.group(1))))
                    pixel_entries.setdefault(pixel_path, set()).add(pixel_entry.group(1))

    generated_dir = os.path.join(project_root, "GraphicsEngine", "Effect", "Zephyr", "Generated")
    if os.path.isdir(generated_dir):
        for file in os.listdir(generated_dir):
            if file.lower().endswith(".hlsl"):
                referenced_shaders.add(os.path.normcase(os.path.normpath(os.path.join(generated_dir, file))))

    paths = []
    unreferenced_count = 0
    for root, _, files in os.walk(os.path.join(project_root, "GraphicsEngine")):
        for file in files:
            if not file.lower().endswith(".hlsl"):
                continue
            path = os.path.join(root, file)
            if os.path.normcase(os.path.normpath(path)) not in referenced_shaders:
                unreferenced_count += 1
                continue
            paths.append(path)

    print(f"エンジンから参照されていないシェーダ {unreferenced_count} 件は事前コンパイルしません")
    context.store["shader"] = (dxc_path, pixel_entries, os.path.normcase(os.path.normpath(generated_dir)))
    return paths


def collect(path, context):
    """
    1ファイルの全エントリをコンパイルし、エンジンが読むキャッシュ形式
    (CompiledShaderObject/Application/<ファイル名>.dx.cso、Zephyr の Generated は
    その下の Zephyr/) にする。
    キャッシュは BinaryOutputArchive の "entries" フィールドに
    std::unordered_map<String, DynamicArray<Uint8>>（キー "プロファイル:エントリ"、
    DXR ライブラリは "lib_6_6:main"）を入れ、AES-256-CBC で暗号化したもの。
    実行時の機能レベル分岐(MS/VS、RT の有無)に関係なく全シェーダを含めるため、
    どの GPU でもソース無しで動く。失敗したら None を返す。
    """
    project_root = context.project_root
    dxc_path, pixel_entries, generated_dir = context.store["shader"]
    file = os.path.basename(path)

    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        source = context.strip_comments(f.read())

    jobs = []
    profile = SHADER_PROFILE_BY_SUFFIX.get(file[:-5][-2:])
    if SHADER_LIBRARY_PATTERN.search(source):
        jobs.append(("lib_6_6", "main"))
    elif profile == "cs_6_6" or (profile is None and SHADER_NUMTHREADS_PATTERN.search(source)):
        for entry in SHADER_COMPUTE_ENTRY_PATTERN.findall(source) or ["main"]:
            jobs.append(("cs_6_6", entry))
    elif profile == "ps_6_6":
        entries = set(pixel_entries.get(os.path.normcase(os.path.normpath(path)), ()))
        if SHADER_MAIN_PATTERN.search(source) or not entries:
            entries.add("main")
        for entry in sorted(entries):
            jobs.append((profile, entry))
    elif profile:
        jobs.append((profile, "main"))
    else:
        print(f"警告: シェーダ種別を判定できないため事前コンパイルしません: {os.path.relpath(path, project_root)}")
        return [], set()

    include_options = ["-I", project_root, "-I", context.user_project_root]
    compiled = {}
    for profile, entry in jobs:
        handle, object_path = tempfile.mkstemp(suffix=".cso", dir=context.staging_dir)
        os.close(handle)
        arguments = [dxc_path, "-T", profile] + (["-enable-payload-qualifiers"] if profile == "lib_6_6" else ["-E", entry]) + SHADER_COMPILE_OPTIONS + include_options + [path, "-Fo", object_path]
        result = subprocess.run(arguments, capture_output=True)
        if result.returncode != 0 or os.path.getsize(object_path) == 0:
            os.remove(object_path)
            message = result.stderr.decode("utf-8", "replace") + result.stdout.decode("utf-8", "replace")
            print(f"シェーダのコンパイルに失敗しました: {os.path.relpath(path, project_root)} ({profile}:{entry})\n{message}")
            return None
        with open(object_path, 'rb') as f:
            compiled[f"{profile}:{entry}"] = f.read()
        os.remove(object_path)

    map_payload = struct.pack('<I', len(compiled))
    for cache_key, data in compiled.items():
        entry_body = context.record("key", cache_key.encode("utf-8")) + context.record("value", struct.pack('<I', len(data)) + data)
        map_payload += struct.pack('<I', len(entry_body)) + entry_body

    directory = os.path.join("CompiledShaderObject", "Application")
    if os.path.normcase(os.path.dirname(os.path.normpath(path))) == generated_dir:
        directory = os.path.join(directory, "Zephyr")
    destination = os.path.join(directory, f"{file[:-5]}.dx.cso")

    staged_path = os.path.join(context.staging_dir, "Shader", f"{context.fnv1a(os.path.normcase(os.path.normpath(path))):08x}.dx.cso")
    os.makedirs(os.path.dirname(staged_path), exist_ok=True)
    with open(staged_path, 'wb') as f:
        f.write(context.encrypt(context.record("entries", map_payload)))

    return [(destination, staged_path)], set()

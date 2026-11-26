import os

LABEL = "Plugins コピー"

EXTENSIONS = set()


def seed(context):
    if not os.path.isdir(context.build_dir):
        print(f"ビルド出力が見つかりません: {context.build_dir}")
        return None

    paths = [os.path.join(context.build_dir, file) for file in os.listdir(context.build_dir) if file.lower().endswith(".cplusplus.dll")]
    if not any(os.path.basename(path) == "UserProject.Cplusplus.dll" for path in paths):
        print(f"UserProject.Cplusplus.dll が見つかりません: {os.path.join(context.build_dir, 'UserProject.Cplusplus.dll')}")
        return None

    external_dir = os.path.join(context.project_root, "External")
    external_dll_count = 0
    for library in sorted(os.listdir(external_dir)):
        dll_dir = os.path.join(external_dir, library, "Dll")
        if not os.path.isdir(dll_dir):
            continue
        release_dll_dir = os.path.join(dll_dir, "Release")
        if os.path.isdir(release_dll_dir):
            dll_dir = release_dll_dir
        for file in os.listdir(dll_dir):
            if not file.lower().endswith(".dll"):
                continue
            paths.append(os.path.join(dll_dir, file))
            external_dll_count += 1
    if external_dll_count == 0:
        print(f"サードパーティ DLL が見つかりません: {external_dir}")
        return None

    return paths


def collect(path, context):
    return [(os.path.join("Plugins", os.path.basename(path)), path)], set()

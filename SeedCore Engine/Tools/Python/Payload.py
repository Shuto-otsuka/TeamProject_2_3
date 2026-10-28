import re
import os
import glob
import xml.etree.ElementTree as ET

from FieldType import PAYLOAD_FIELD_TYPES
from FieldType.Field import EmitContext, FieldDecl

STRUCT_PATTERN = re.compile(r'(struct|class)\s+(?:SEEDCORE_API\s+)?(\w+)')

PAYLOAD_FIELD_PATTERN = re.compile(
    r'SC_PAYLOAD_FIELD\(\s*(\w+)\s*\)\s*([\w:<>\s]+)\s+(\w+)\s*(?:=[^;]*)?\s*;'
)

PAYLOAD_FIELD_EX_PATTERN = re.compile(
    r'SC_PAYLOAD_FIELD_EX\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)\s*([\w:<>\s]+)\s+(\w+)\s*(?:=[^;]*)?\s*;'
)

REFLECTION_FIELD_CONDITION_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD_CONDITION\((.+?)\)\s*'
)

ENUM_CLASS_PATTERN = re.compile(
    r'enum\s+class\s+(\w+)\s*\{([^}]*)\}'
)

ENGINE_NAMESPACE_PATTERN = re.compile(r'(?:::)?\bSeedCore::')

DYNAMIC_ARRAY_PREFIXES = ['std::vector', 'DynamicArray']

REGISTRY_BEGIN_MARKER = '// [PAYLOAD_AUTO_BEGIN]'

REGISTRY_END_MARKER = '// [PAYLOAD_AUTO_END]'

REGISTRY_HEADER = 'PayloadRegistry.h'

SKIP_DIRECTORIES = {'External', 'Package', 'CompiledShaderObject', 'Build', 'Intermediate', '.git', '.vs'}

MSBUILD_NAMESPACE = 'http://schemas.microsoft.com/developer/msbuild/2003'

FILTER_NAME = 'Payload'


# ---------------------------
# 解析 (parse_)
# ---------------------------
def parse_code(code):
    result = []
    index = 0
    length = len(code)

    in_string = False
    in_char = False
    in_line_comment = False
    in_block_comment = False

    while index < length:
        character = code[index]
        next_character = code[index + 1] if index + 1 < length else ''

        if not in_string and not in_char and not in_block_comment:
            if character == '/' and next_character == '/':
                in_line_comment = True
                index += 2
                continue

        if not in_string and not in_char and not in_line_comment:
            if character == '/' and next_character == '*':
                in_block_comment = True
                index += 2
                continue

        if in_block_comment:
            if character == '*' and next_character == '/':
                in_block_comment = False
                index += 2
                continue
            index += 1
            continue

        if in_line_comment:
            if character == '\n':
                in_line_comment = False
                result.append(character)
            index += 1
            continue

        if not in_char and character == '"':
            if not in_string:
                prefix = ''.join(result)
                if prefix.rstrip().endswith('SC_PAYLOAD_FIELD_EX('):
                    end_quote = code.find('"', index + 1)
                    if end_quote != -1:
                        result.append(code[index:end_quote + 1])
                        index = end_quote + 1
                        continue
            in_string = not in_string
            result.append(' ')
            index += 1
            continue

        if not in_string and character == '\'':
            in_char = not in_char
            result.append(' ')
            index += 1
            continue

        result.append(character)
        index += 1

    return ''.join(result)


def parse_block(content, start):
    depth = 0

    for index in range(start, len(content)):
        character = content[index]

        if character == '{':
            depth += 1
        elif character == '}':
            depth -= 1
            if depth == 0:
                return content[start:index]

    return None


def parse_type_name(type_text):
    return ENGINE_NAMESPACE_PATTERN.sub('', type_text).strip()


def parse_array_type(type_text):
    """
    Payload fields are never fixed-size C arrays, so only DynamicArray/
    std::vector need detecting here. Returns the element type string, or
    None if type_text isn't a dynamic array.
    """
    type_text = type_text.strip()

    for prefix in DYNAMIC_ARRAY_PREFIXES:
        if type_text.startswith(prefix + '<'):
            depth = 0
            start = type_text.index('<')
            for index in range(start, len(type_text)):
                if type_text[index] == '<': depth += 1
                elif type_text[index] == '>': depth -= 1
                if depth == 0:
                    return type_text[start + 1:index].strip()

    return None


def parse_enums(content):
    enums = {}
    for enum_match in ENUM_CLASS_PATTERN.finditer(content):
        enum_name = enum_match.group(1)
        body = enum_match.group(2)
        entries = []
        for entry in body.split(','):
            entry = entry.strip()
            if not entry:
                continue
            name = entry.split('=')[0].strip()
            if name:
                entries.append(name)
        if entries:
            enums[enum_name] = entries
    return enums


def parse_enum_type(type_name, struct_name, enums, enum_headers, header_path):
    """
    struct_name のメンバとして書かれた型名 type_name を、collect_enums のキー(所有者付きの名前)に解決する。
    enum でなければ None を返す。C++ の名前探索と同じく、所有者付きで書かれていればその名前、
    次に struct_name 自身の中の enum、次に名前空間直下の enum の順に探す。
    どれにも無いとき(基底クラスの enum など)は同じ名前の enum を探し、
    複数あれば header_path と同じヘッダで宣言されたものを優先する。
    """
    if '::' in type_name:
        return type_name if type_name in enums else None

    own_enum_type = f'{struct_name}::{type_name}'
    if own_enum_type in enums:
        return own_enum_type
    if type_name in enums:
        return type_name

    candidates = [enum_type for enum_type in enums if enum_type.endswith('::' + type_name)]
    if not candidates:
        return None
    same_header_candidates = [enum_type for enum_type in candidates if enum_headers[enum_type] == header_path]
    return (same_header_candidates or candidates)[-1]


def parse_structs(content):
    """
    コメント・文字列除去済みの content から、SC_PAYLOAD_FIELD(_EX) が
    付与された struct/class ごとの FieldDecl を集め、[(struct_name, fields), ...] を返す。
    condition_ はまだ解決前の式のまま。emit_header() と
    collect_payload_field_names() の両方から使われる。
    """
    results = []

    for struct_match in STRUCT_PATTERN.finditer(content):
        struct_name = struct_match.group(2)

        brace_position = content.find('{', struct_match.end())
        if brace_position == -1:
            continue
        if is_forward_declaration(content, struct_match.end(), brace_position):
            continue

        body = parse_block(content, brace_position)
        if not body:
            continue

        conditions = {}
        for condition_match in REFLECTION_FIELD_CONDITION_PATTERN.finditer(body):
            conditions[condition_match.end()] = condition_match.group(1).strip()

        fields = []
        for field_match in PAYLOAD_FIELD_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(2), field_match.group(3), field_match.group(3), field_match.group(1)))
        for field_match in PAYLOAD_FIELD_EX_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(3), field_match.group(4), field_match.group(1), field_match.group(2)))
        fields.sort(key=lambda entry: entry[0])

        resolved_fields = []
        used_conditions = set()
        for position, field_type_text, field_name, display_name, asset_type in fields:
            condition = None
            for condition_end, condition_text in conditions.items():
                if condition_end in used_conditions:
                    continue
                if condition_end <= position and position - condition_end < 200:
                    between = body[condition_end:position].strip()
                    if between == '' or between.startswith('SC_'):
                        condition = condition_text
                        used_conditions.add(condition_end)
                        break
            field_type = parse_type_name(field_type_text)
            element_type = parse_array_type(field_type)
            array_type = (element_type, None, True) if element_type is not None else None
            resolved_fields.append(FieldDecl(field_type, field_name, display_name, None, array_type, None, condition, None, None, False, asset_type))
        fields = resolved_fields
        if fields:
            results.append((struct_name, fields))

    return results


# ---------------------------
# 判定 (is_)
# ---------------------------
def is_forward_declaration(content, match_end, brace_position):
    """
    STRUCT_PATTERN が拾った struct/class 名が前方宣言(本体を持たない
    `struct X;`)かどうかを判定する。match_end から brace_position までの間に
    ';' があれば、brace_position の '{' はこの宣言の本体ではなく、後方にある
    無関係な別の struct/class/namespace の開き括弧を誤って拾っている
    (例: 前方宣言の直後に別のクラス本体が続く場合)。
    """
    semicolon_position = content.find(';', match_end)
    return semicolon_position != -1 and semicolon_position < brace_position


def is_user_project_file(full_path, project_root):
    """
    full_path が UserProject/ 配下にあるかどうかを判定する。
    UserProject で定義された型の Payload コードは UserProject.Cplusplus.dll 側の
    出力ファイルへ、それ以外(エンジン側)は FoundationEngine 側の出力ファイル
    へ振り分けるために使う — UserProject.Cplusplus.dll だけを再ビルドしてもPayload
    フィールドのオフセットが古いまま SeedCore.Cplusplus.dll に取り残される事故を防ぐ。
    """
    relative_path = os.path.relpath(full_path, project_root)
    return relative_path.split(os.sep)[0].split('/')[0] == 'UserProject'


# ---------------------------
# 走査 (collect_)
# ---------------------------
def collect_directories(directories):
    """
    走査対象から外すディレクトリを directories から取り除く
    (詳細は Reflection.py の同名関数のコメントを参照)。
    """
    directories[:] = [directory for directory in directories if directory not in SKIP_DIRECTORIES]


def collect_enums(project_root):
    """
    プロジェクト全体の enum class を、所有者付きの名前をキーにして集める。
    struct/class の中で宣言された enum は "Owner::Name"、名前空間直下の enum は "Name" になる。
    struct/class が入れ子になっているときは、一番内側の struct/class を所有者とする。
    (キー -> 値の一覧, キー -> 宣言されているヘッダの相対パス) を返す。
    """
    enums = {}
    enum_headers = {}
    for root, directories, files in os.walk(project_root):
        collect_directories(directories)
        for file in files:
            if not file.endswith(".h"):
                continue
            full_path = os.path.join(root, file)
            try:
                with open(full_path, 'r', encoding='utf-8', errors='ignore') as file_handle:
                    content = file_handle.read()
                code = parse_code(content)
                relative_path = os.path.relpath(full_path, start=project_root).replace('\\', '/')

                struct_ranges = []
                for struct_match in STRUCT_PATTERN.finditer(code):
                    brace_position = code.find('{', struct_match.end())
                    if brace_position == -1:
                        continue
                    if is_forward_declaration(code, struct_match.end(), brace_position):
                        continue
                    body = parse_block(code, brace_position)
                    if not body:
                        continue
                    struct_ranges.append((brace_position, brace_position + len(body), struct_match.group(2)))

                for enum_match in ENUM_CLASS_PATTERN.finditer(code):
                    owner_ranges = [struct_range for struct_range in struct_ranges if struct_range[0] < enum_match.start() < struct_range[1]]
                    owner_name = max(owner_ranges, key=lambda struct_range: struct_range[0])[2] if owner_ranges else None
                    for enum_name, entries in parse_enums(enum_match.group(0)).items():
                        enum_type = f'{owner_name}::{enum_name}' if owner_name else enum_name
                        if owner_name is None and enum_type in enums:
                            continue
                        enums[enum_type] = entries
                        enum_headers[enum_type] = relative_path
            except Exception:
                continue
    return enums, enum_headers


def collect_payload_field_names(project_root):
    """
    project_root 以下の全 .h を走査し、payload フィールドを持つ struct/class
    ごとの表示名一覧を {struct_name: set(display_name)} で返す。
    main() と同じ走査対象（PayloadRegistry.h は除外）。
    RuntimePackager.py がシーン/プレハブの参照 Asset を解決するために使う。
    """
    field_names = {}

    for root, directories, files in os.walk(project_root):
        collect_directories(directories)
        for file in files:
            if not file.endswith(".h"):
                continue
            if file == REGISTRY_HEADER:
                continue

            full_path = os.path.join(root, file)
            try:
                with open(full_path, 'r', encoding='utf-8', errors='ignore') as file_handle:
                    content = file_handle.read()
            except Exception:
                continue

            content = parse_code(content)

            results = parse_structs(content)
            for struct_name, fields in results:
                names = field_names.setdefault(struct_name, set())
                for field in fields:
                    names.add(field.display_name_)

    return field_names


# ---------------------------
# コード生成 (emit_)
# ---------------------------
def emit_condition(condition, struct_name, enums, enum_headers, header_path):
    """
    condition の式を enableIf_ のラムダ内で使える形にする。
    メンバ (末尾が _ の名前) には o. を付け、Name:: の形で書かれた enum は parse_enum_type で所有者付きの名前にする。
    """
    result = re.sub(r'\b(\w+_)\b', r'o.\1', condition)
    for type_name in sorted(set(re.findall(r'(?<!::)\b(\w+)::', result))):
        enum_type = parse_enum_type(type_name, struct_name, enums, enum_headers, header_path)
        if enum_type and enum_type != type_name:
            result = re.sub(r'(?<!::)\b' + type_name + r'::', f'{enum_type}::', result)
    return result


def emit_header(file_path, project_root, enums, enum_headers):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as file_handle:
            content = file_handle.read()
    except Exception:
        return None

    content = parse_code(content)
    relative_path = os.path.relpath(file_path, start=project_root).replace('\\', '/')

    # -----------------------
    # 解析
    # -----------------------
    results = parse_structs(content)

    if not results:
        return None

    # -----------------------
    # 出力生成
    # -----------------------
    includes = [relative_path]

    force_lines = []
    for struct_name, fields in results:
        force_lines.append(f'extern "C" int _force_payload_{struct_name} = 0;')

    lines = []
    lines.append(f'\t\t// ---- {relative_path} ----')

    for struct_name, fields in results:
        lines.append(f'\t\tstruct Register_{struct_name}')
        lines.append('\t\t{')
        lines.append(f'\t\t\tRegister_{struct_name}()')
        lines.append('\t\t\t{')
        lines.append(
            f'\t\t\t\tPayloadRegistry::Register('
            f'String("{struct_name}"), '
            '[](void* ptr, DynamicArray<FieldInfo>& outInfo) {'
        )

        lines.append(f'\t\t\t\t\t{struct_name}& obj = *static_cast<{struct_name}*>(ptr);')

        context = EmitContext(struct_name, {}, set())
        for field in fields:
            if field.condition_:
                field.condition_ = emit_condition(field.condition_, struct_name, enums, enum_headers, relative_path)
            for field_type in PAYLOAD_FIELD_TYPES:
                if field_type.matches(field, context):
                    field_type.emit(field, context, lines)
                    break

        lines.append('\t\t\t\t});')
        lines.append('\t\t\t}')
        lines.append('\t\t};')
        lines.append(f'\t\tstatic Register_{struct_name} global_{struct_name}_register;')
        lines.append('')

    struct_names = [struct_name for struct_name, _ in results]
    return includes, force_lines, lines, struct_names


def emit_source(includes, force_lines, body_lines):
    """
    1モジュール分の includes/force_lines/body_lines から、
    Payload.generated.cpp の完全なソースコード文字列を組み立てる。
    """
    lines = []
    lines.append('#include <FoundationEngine/Prelude.h>')
    lines.append('#include <FoundationEngine/Payload/PayloadRegistry.h>')
    for include in sorted(set(includes)):
        lines.append(f'#include <{include}>')
    lines.append('')
    lines.extend(force_lines)
    lines.append('')
    lines.append('namespace SeedCore')
    lines.append('{')
    lines.append('\t namespace ScPayload')
    lines.append('\t {')
    lines.extend(body_lines)
    lines.append('\t}')
    lines.append('}')

    return '\r\n'.join(lines)


# ---------------------------
# ファイル更新 (update_)
# ---------------------------
def update_source(output_path, code, label):
    """
    code が output_path の現在の中身と異なる場合のみ書き込み、変化したかどうかを返す。
    """
    old_code = None
    if os.path.exists(output_path):
        with open(output_path, 'r', encoding='utf-8', newline='') as file_handle:
            old_code = file_handle.read()

    if old_code == code:
        return False

    with open(output_path, 'w', encoding='utf-8', newline='') as file_handle:
        file_handle.write(code)
    print(f"生成完了: {label}")
    return True


def update_objects(project_root, module_name):
    """
    生成ファイルが書き換わったときに、対応する obj を消しておく
    (詳細は Reflection.py の同名関数のコメントを参照)。
    """
    object_root = os.path.join(project_root, 'Runtime', 'Intermediate', module_name)
    for object_path in glob.glob(os.path.join(object_root, '**', 'Payload.generated.obj'), recursive=True):
        try:
            os.remove(object_path)
            print(f"obj削除: {object_path}")
        except Exception:
            pass


def update_registry(registry_cpp_path, struct_names):
    with open(registry_cpp_path, 'r', encoding='utf-8', newline='') as file_handle:
        content = file_handle.read()

    pragma_lines = []
    for struct_name in struct_names:
        pragma_lines.append(f'#pragma comment(linker, "/include:_force_payload_{struct_name}")')

    new_block = REGISTRY_BEGIN_MARKER + '\r\n' + '\r\n'.join(pragma_lines) + '\r\n' + REGISTRY_END_MARKER

    if REGISTRY_BEGIN_MARKER in content and REGISTRY_END_MARKER in content:
        start = content.index(REGISTRY_BEGIN_MARKER)
        end = content.index(REGISTRY_END_MARKER) + len(REGISTRY_END_MARKER)
        updated = content[:start] + new_block + content[end:]
    else:
        include_end = content.rfind('#include')
        insert_position = content.index('\r\n', include_end) + 2
        updated = content[:insert_position] + '\r\n' + new_block + '\r\n' + content[insert_position:]

    if updated != content:
        with open(registry_cpp_path, 'w', encoding='utf-8', newline='') as file_handle:
            file_handle.write(updated)
        print(f"更新完了: {os.path.basename(registry_cpp_path)}")


def update_vcxproj(vcxproj_path, cpp_paths):
    ET.register_namespace('', MSBUILD_NAMESPACE)
    tree = ET.parse(vcxproj_path)
    root = tree.getroot()

    project_directory = os.path.dirname(vcxproj_path)

    existing = set()
    for item in root.iter(f'{{{MSBUILD_NAMESPACE}}}ClCompile'):
        include = item.get('Include', '')
        existing.add(os.path.normpath(include).lower())

    changed = False

    for cpp_path in cpp_paths:
        relative_path = os.path.relpath(cpp_path, project_directory)
        if os.path.normpath(relative_path).lower() in existing:
            continue

        item_groups = [item_group for item_group in root.iter(f'{{{MSBUILD_NAMESPACE}}}ItemGroup') if item_group.find(f'{{{MSBUILD_NAMESPACE}}}ClCompile') is not None]
        if not item_groups:
            continue
        item_group = item_groups[0]

        compile_element = ET.SubElement(item_group, f'{{{MSBUILD_NAMESPACE}}}ClCompile')
        compile_element.set('Include', relative_path)

        print(f"vcxproj追加: {relative_path}")
        changed = True

    if changed:
        tree.write(vcxproj_path, xml_declaration=True, encoding='utf-8')

    filters_path = vcxproj_path + '.filters'
    if os.path.exists(filters_path):
        update_filters(filters_path, cpp_paths, project_directory)


def update_filters(filters_path, cpp_paths, project_directory):
    ET.register_namespace('', MSBUILD_NAMESPACE)
    tree = ET.parse(filters_path)
    root = tree.getroot()

    filter_exists = False
    for filter_node in root.iter(f'{{{MSBUILD_NAMESPACE}}}Filter'):
        if filter_node.get('Include', '') == FILTER_NAME:
            filter_exists = True
            break

    if not filter_exists:
        filter_groups = [item_group for item_group in root.iter(f'{{{MSBUILD_NAMESPACE}}}ItemGroup') if item_group.find(f'{{{MSBUILD_NAMESPACE}}}Filter') is not None]
        if filter_groups:
            filter_group = filter_groups[0]
            filter_element = ET.SubElement(filter_group, f'{{{MSBUILD_NAMESPACE}}}Filter')
            filter_element.set('Include', FILTER_NAME)
            unique_identifier = ET.SubElement(filter_element, f'{{{MSBUILD_NAMESPACE}}}UniqueIdentifier')
            unique_identifier.text = '{a1b2c3d4-payl-0000-0000-000000000001}'

    existing = set()
    for item in root.iter(f'{{{MSBUILD_NAMESPACE}}}ClCompile'):
        include = item.get('Include', '')
        existing.add(os.path.normpath(include).lower())

    changed = False

    for cpp_path in cpp_paths:
        relative_path = os.path.relpath(cpp_path, project_directory)
        if os.path.normpath(relative_path).lower() in existing:
            continue

        compile_groups = [item_group for item_group in root.iter(f'{{{MSBUILD_NAMESPACE}}}ItemGroup') if item_group.find(f'{{{MSBUILD_NAMESPACE}}}ClCompile') is not None]
        if not compile_groups:
            continue
        item_group = compile_groups[0]

        compile_element = ET.SubElement(item_group, f'{{{MSBUILD_NAMESPACE}}}ClCompile')
        compile_element.set('Include', relative_path)
        filter_child = ET.SubElement(compile_element, f'{{{MSBUILD_NAMESPACE}}}Filter')
        filter_child.text = FILTER_NAME

        print(f"filters追加: {relative_path} -> {FILTER_NAME}")
        changed = True

    if changed:
        tree.write(filters_path, xml_declaration=True, encoding='utf-8')


# ---------------------------
# エントリポイント
# ---------------------------
def main():
    """
    ヘッダーごとに新しい.payload.cppを作る方式はやめて、既にvcxprojに登録
    済みの1ファイル(Payload.generated.cpp)の中身だけを毎回まるごと書き換える。
    理由はReflection.pyと同じ(PreBuildEvent中の新規ファイル追加は同じ
    ビルドに反映されない)。

    UserProject/ 配下で定義された型は、FoundationEngine 側の集約ファイルへ
    混ぜず、UserProject/Payload/Payload.generated.cpp という別ファイルへ
    出力し UserProject.Cplusplus.vcxproj 側でコンパイルする(Reflection.py と同じ理由)。
    """
    script_directory = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_directory))

    foundation_output_directory = os.path.join(project_root, 'FoundationEngine', 'Payload')
    os.makedirs(foundation_output_directory, exist_ok=True)
    foundation_output_path = os.path.join(foundation_output_directory, 'Payload.generated.cpp')

    user_project_output_directory = os.path.join(project_root, 'UserProject', 'Payload')
    os.makedirs(user_project_output_directory, exist_ok=True)
    user_project_output_path = os.path.join(user_project_output_directory, 'Payload.generated.cpp')

    enums, enum_headers = collect_enums(project_root)

    buckets = {
        'foundation': {'struct_names': [], 'includes': [], 'force_lines': [], 'body_lines': []},
        'user_project': {'struct_names': [], 'includes': [], 'force_lines': [], 'body_lines': []},
    }

    for root, directories, files in os.walk(project_root):
        collect_directories(directories)
        for file in files:
            if not file.endswith(".h"):
                continue

            if file == REGISTRY_HEADER:
                continue

            full_path = os.path.join(root, file)
            result = emit_header(full_path, project_root, enums, enum_headers)

            if result:
                includes, force_lines, body_lines, struct_names = result
                bucket = buckets['user_project'] if is_user_project_file(full_path, project_root) else buckets['foundation']
                bucket['includes'].extend(includes)
                bucket['force_lines'].extend(force_lines)
                bucket['body_lines'].extend(body_lines)
                bucket['struct_names'].extend(struct_names)

    foundation_bucket = buckets['foundation']
    user_project_bucket = buckets['user_project']

    foundation_code = emit_source(foundation_bucket['includes'], foundation_bucket['force_lines'], foundation_bucket['body_lines'])
    user_project_code = emit_source(user_project_bucket['includes'], user_project_bucket['force_lines'], user_project_bucket['body_lines'])

    foundation_changed = update_source(foundation_output_path, foundation_code, 'Payload.generated.cpp (FoundationEngine)')
    user_project_changed = update_source(user_project_output_path, user_project_code, 'Payload.generated.cpp (UserProject)')

    if foundation_changed:
        update_objects(project_root, 'FoundationEngine.Cplusplus')
    if user_project_changed:
        update_objects(project_root, 'UserProject.Cplusplus')

    registry_cpp = os.path.join(project_root, 'FoundationEngine', 'ECS', 'PayloadRegistry.cpp')
    if os.path.exists(registry_cpp):
        update_registry(registry_cpp, foundation_bucket['struct_names'])

    foundation_vcxproj = os.path.join(project_root, 'FoundationEngine', 'FoundationEngine.Cplusplus.vcxproj')
    if os.path.exists(foundation_vcxproj):
        update_vcxproj(foundation_vcxproj, [foundation_output_path])

    user_project_vcxproj = os.path.join(project_root, 'UserProject', 'UserProject.Cplusplus.vcxproj')
    if os.path.exists(user_project_vcxproj):
        update_vcxproj(user_project_vcxproj, [user_project_output_path])


if __name__ == "__main__":
    main()

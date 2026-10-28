import re
import os
import glob
import xml.etree.ElementTree as ET

from FieldType import REFLECTION_FIELD_TYPES, Enum
from FieldType.Field import EmitContext, FieldDecl

STRUCT_PATTERN = re.compile(r'(struct|class)\s+(?:SEEDCORE_API\s+)?(\w+)')

REFLECTION_FIELD_CONDITION_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD_CONDITION\((.+?)\)\s*'
)

REFLECTION_FIELD_REVISION_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD_REVISION\(\)\s*'
)

REFLECTION_FIELD_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD\(\)\s*([\w:<>,\s]+)\s+(\w+)\s*(\[\d+\])?\s*(?:=[^;]*)?\s*;'
)

REFLECTION_FIELD_EX_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD_EX\(\s*"([^"]+)"\s*\)\s*([\w:<>,\s]+)\s+(\w+)\s*(\[\d+\])?\s*(?:=[^;]*)?\s*;'
)

REFLECTION_CLAMPED_PATTERN = re.compile(
    r'SC_REFLECTION_CLAMPED\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*([\w:<>,\s]+)\s+(\w+)\s*(\[\d+\])?\s*(?:=[^;]*)?\s*;'
)

REFLECTION_CLAMPED_EX_PATTERN = re.compile(
    r'SC_REFLECTION_CLAMPED_EX\(\s*"([^"]+)"\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*([\w:<>,\s]+)\s+(\w+)\s*(\[\d+\])?\s*(?:=[^;]*)?\s*;'
)

SERIALIZE_FIELD_PATTERN = re.compile(
    r'SC_SERIALIZE_FIELD\(\)\s*([\w:<>,\s]+)\s+(\w+)\s*(\[\d+\])?\s*(?:=[^;]*)?\s*;'
)

FUNCTION_PATTERN = re.compile(
    r'SC_FUNCTION\(\)\s*(?:virtual\s+|inline\s+)*([\w:<>\s&]+?)\s+(\w+)\s*\(([^)]*)\)'
)

KNOWN_STRUCT_MARKER_PATTERN = re.compile(
    r'SC_REFLECTION_FIELD\(\)|SC_REFLECTION_FIELD_EX\(|SC_REFLECTION_CLAMPED\(|SC_REFLECTION_CLAMPED_EX\(|SC_SERIALIZE_FIELD\b'
)

FUNCTION_ATTRIBUTE_TYPES = {
    'Int',
    'Float',
    'Bool',
    'Vector2',
    'Vector3',
    'String',
    'Color'
}

ENGINE_NAMESPACE_PATTERN = re.compile(r'(?:::)?\bSeedCore::')

DYNAMIC_ARRAY_PREFIXES = ['std::vector', 'DynamicArray']

STATIC_ARRAY_PREFIXES = ['std::array', 'StaticArray']

ENUM_CLASS_PATTERN = re.compile(
    r'enum\s+class\s+(\w+)\s*\{([^}]*)\}'
)

REGISTRY_BEGIN_MARKER = '// [REFLECTION_AUTO_BEGIN]'

REGISTRY_END_MARKER = '// [REFLECTION_AUTO_END]'

REGISTRY_HEADER = 'ReflectionRegistry.h'

SKIP_DIRECTORIES = {'External', 'Package', 'CompiledShaderObject', 'Build', 'Intermediate', '.git', '.vs'}

MSBUILD_NAMESPACE = 'http://schemas.microsoft.com/developer/msbuild/2003'

FILTER_NAME = 'Reflection'


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
                if prefix.rstrip().endswith('SC_REFLECTION_FIELD_EX(') or prefix.rstrip().endswith('SC_REFLECTION_CLAMPED_EX('):
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


def parse_array_type(type_text, bracket_text=None):
    """
    Returns (element_type, count_or_none, is_dynamic)
    count_or_none: int for static, None for dynamic
    Returns None if not an array type
    """
    type_text = type_text.strip()

    if bracket_text:
        count = int(bracket_text.strip('[]'))
        return (type_text, count, False)

    for prefix in DYNAMIC_ARRAY_PREFIXES:
        if type_text.startswith(prefix + '<'):
            depth = 0
            start = type_text.index('<')
            for index in range(start, len(type_text)):
                if type_text[index] == '<': depth += 1
                elif type_text[index] == '>': depth -= 1
                if depth == 0:
                    inner = type_text[start+1:index].strip()
                    return (inner, None, True)

    for prefix in STATIC_ARRAY_PREFIXES:
        if type_text.startswith(prefix + '<'):
            depth = 0
            start = type_text.index('<')
            for index in range(start, len(type_text)):
                if type_text[index] == '<': depth += 1
                elif type_text[index] == '>': depth -= 1
                if depth == 0:
                    inner = type_text[start+1:index].strip()
                    parts = inner.rsplit(',', 1)
                    element_type = parts[0].strip()
                    count = int(parts[1].strip())
                    return (element_type, count, False)

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


def parse_function_type(type_text):
    """'const String&' → 'String'。対応していない型なら None を返す。"""
    cleaned = parse_type_name(re.sub(r'\bconst\b', ' ', type_text).replace('&', ' '))
    if cleaned in FUNCTION_ATTRIBUTE_TYPES:
        return cleaned
    return None


def parse_parameter_types(parameters_text):
    """'Int damage, const Vector3& point' → ['Int', 'Vector3']。1つでも未対応なら None。"""
    types = []
    for parameter in parameters_text.split(','):
        parameter = parameter.split('=')[0].strip()
        if not parameter or parameter == 'void':
            continue
        type_text = parameter.rsplit(None, 1)[0]
        parameter_type = parse_function_type(type_text)
        if parameter_type is None:
            return None
        types.append(parameter_type)
    return types


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
    UserProject で定義された型のリフレクションコードは UserProject.Cplusplus.dll 側の
    出力ファイルへ、それ以外(エンジン側)は FoundationEngine 側の出力ファイル
    へ振り分けるために使う — UserProject.Cplusplus.dll だけを再ビルドしてもリフレクション
    のフィールドオフセットが古いまま SeedCore.Cplusplus.dll に取り残される事故を防ぐため。
    """
    relative_path = os.path.relpath(full_path, project_root)
    return relative_path.split(os.sep)[0].split('/')[0] == 'UserProject'


# ---------------------------
# 走査 (collect_)
# ---------------------------
def collect_directories(directories):
    """
    走査対象から外すディレクトリを directories から取り除く。
    サードパーティ(External)やビルド生成物には SC_ アノテーションが
    存在しないため、読み込むだけ時間の無駄になる。os.walk は directories を
    破壊的に書き換えるとその配下ごと走査をスキップするので、
    リポジトリの .h の約7割をここで除外できる。
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


def collect_structs(project_root):
    """
    struct/class 名 -> その定義があるヘッダの相対パス。
    本体(直下のネストも含む)に SC_REFLECTION_FIELD 系か SC_SERIALIZE_FIELD が
    1つでもあれば「既知の構造体」とし、他フィールドの型としてこの名前が
    出てきたら nested_type_name_ で再帰させる対象になる。
    """
    known_structs = {}
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

            code = parse_code(content)
            relative_path = os.path.relpath(full_path, start=project_root).replace('\\', '/')

            for struct_match in STRUCT_PATTERN.finditer(code):
                struct_name = struct_match.group(2)
                brace_position = code.find('{', struct_match.end())
                if brace_position == -1:
                    continue
                if is_forward_declaration(code, struct_match.end(), brace_position):
                    continue
                body = parse_block(code, brace_position)
                if not body:
                    continue
                if KNOWN_STRUCT_MARKER_PATTERN.search(body):
                    known_structs[struct_name] = relative_path

    return known_structs


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


def emit_header(file_path, project_root, enums, enum_headers, known_structs=None):
    known_structs = known_structs or {}

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

        revisions = {}
        for revision_match in REFLECTION_FIELD_REVISION_PATTERN.finditer(body):
            revisions[revision_match.end()] = True

        fields = []
        for field_match in REFLECTION_FIELD_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(1), field_match.group(2), field_match.group(2), field_match.group(3), None, None, False))
        for field_match in REFLECTION_FIELD_EX_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(2), field_match.group(3), field_match.group(1), field_match.group(4), None, None, False))
        for field_match in REFLECTION_CLAMPED_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(3), field_match.group(4), field_match.group(4), field_match.group(5), field_match.group(1).strip(), field_match.group(2).strip(), False))
        for field_match in REFLECTION_CLAMPED_EX_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(4), field_match.group(5), field_match.group(1), field_match.group(6), field_match.group(2).strip(), field_match.group(3).strip(), False))
        for field_match in SERIALIZE_FIELD_PATTERN.finditer(body):
            fields.append((field_match.start(), field_match.group(1), field_match.group(2), field_match.group(2), field_match.group(3), None, None, True))
        fields.sort(key=lambda entry: entry[0])

        resolved_fields = []
        used_conditions = set()
        used_revisions = set()
        for position, field_type_text, field_name, display_name, bracket, clamp_min, clamp_max, serialize_only in fields:
            condition = None
            for condition_end, condition_text in conditions.items():
                if condition_end in used_conditions:
                    continue
                if condition_end <= position and position - condition_end < 200:
                    between = body[condition_end:position].strip()
                    if between == '' or between.startswith('SC_REFLECTION'):
                        condition = condition_text
                        used_conditions.add(condition_end)
                        break

            revision = False
            for revision_end in revisions:
                if revision_end in used_revisions:
                    continue
                if revision_end <= position and position - revision_end < 200:
                    between = body[revision_end:position].strip()
                    if between == '' or between.startswith('SC_REFLECTION'):
                        revision = True
                        used_revisions.add(revision_end)
                        break
            field_type = parse_type_name(field_type_text)
            array_type = parse_array_type(field_type, bracket)
            enum_type = parse_enum_type(field_type, struct_name, enums, enum_headers, relative_path) if array_type is None else None
            resolved_fields.append(FieldDecl(field_type, field_name, display_name, bracket, array_type, enum_type, condition, clamp_min, clamp_max, serialize_only, revision_=revision))
        fields = resolved_fields

        functions = []
        for function_match in FUNCTION_PATTERN.finditer(body):
            return_text = function_match.group(1).strip()
            function_name = function_match.group(2)
            parameter_types = parse_parameter_types(function_match.group(3))

            return_type = None if return_text == 'void' else parse_function_type(return_text)
            if parameter_types is None or (return_text != 'void' and return_type is None):
                print(f"警告: {struct_name}::{function_name} は SC_FUNCTION で使えない型があるため登録しません")
                continue
            functions.append((return_type, function_name, parameter_types))

        if fields or functions:
            results.append((struct_name, fields, functions))

    if not results:
        return None

    # -----------------------
    # 出力生成
    # -----------------------
    used_enum_headers = set()
    for struct_name, fields, functions in results:
        for field in fields:
            if field.enum_type_ is not None:
                used_enum_headers.add(enum_headers[field.enum_type_])

    includes = [relative_path]
    for enum_header in sorted(used_enum_headers):
        if enum_header != relative_path:
            includes.append(enum_header)

    force_lines = []
    for struct_name, fields, functions in results:
        force_lines.append(f'extern "C" int _force_reflection_{struct_name} = 0;')

    lines = []
    lines.append(f'\t\t// ---- {relative_path} ----')

    generated_enums = set()

    for struct_name, fields, functions in results:
        if not fields:
            continue
        lines.append(f'\t\tstruct Register_{struct_name}')
        lines.append('\t\t{')
        lines.append(f'\t\t\tRegister_{struct_name}()')
        lines.append('\t\t\t{')
        lines.append(
            f'\t\t\t\tReflectionRegistry::Register('
            f'String("{struct_name}"), '
            '[](void* ptr, DynamicArray<FieldInfo>& outInfo) {'
        )

        lines.append(f'\t\t\t\t\t{struct_name}& obj = *static_cast<{struct_name}*>(ptr);')

        context = EmitContext(struct_name, known_structs, generated_enums)
        for field in fields:
            if field.condition_:
                field.condition_ = emit_condition(field.condition_, struct_name, enums, enum_headers, relative_path)
            for field_type in REFLECTION_FIELD_TYPES:
                if field_type.matches(field, context):
                    field_type.emit(field, context, lines)
                    break

        lines.append('\t\t\t\t});')
        lines.append('\t\t\t}')
        lines.append('\t\t};')
        lines.append(f'\t\tstatic Register_{struct_name} global_{struct_name}_register;')
        lines.append('')

    for struct_name, fields, functions in results:
        if not functions:
            continue
        lines.append(f'\t\tstruct RegisterFunction_{struct_name}')
        lines.append('\t\t{')
        lines.append(f'\t\t\tRegisterFunction_{struct_name}()')
        lines.append('\t\t\t{')

        for return_type, function_name, parameter_types in functions:
            parameter_kinds = ', '.join(f'AttributeType::{parameter_type}' for parameter_type in parameter_types)
            arguments = ', '.join(f'*static_cast<const {parameter_type}*>(arguments[{index}])' for index, parameter_type in enumerate(parameter_types))
            call = f'obj.{function_name}({arguments})'

            lines.append('\t\t\t\t{')
            lines.append('\t\t\t\t\tFunctionInfo info;')
            lines.append(f'\t\t\t\t\tinfo.name_ = String("{function_name}");')
            if parameter_types:
                lines.append(f'\t\t\t\t\tinfo.parameters_ = {{ {parameter_kinds} }};')
            if return_type is not None:
                lines.append(f'\t\t\t\t\tinfo.result_ = AttributeType::{return_type};')
            lines.append('\t\t\t\t\tinfo.invoke_ = [](void* component, void* const* arguments, void* result)')
            lines.append('\t\t\t\t\t{')
            lines.append(f'\t\t\t\t\t\t{struct_name}& obj = *static_cast<{struct_name}*>(component);')
            if return_type is None:
                lines.append(f'\t\t\t\t\t\t{call};')
            else:
                lines.append(f'\t\t\t\t\t\t*static_cast<{return_type}*>(result) = {call};')
            lines.append('\t\t\t\t\t};')
            lines.append(f'\t\t\t\t\tFunctionRegistry::Register(String("{struct_name}"), std::move(info));')
            lines.append('\t\t\t\t}')

        lines.append('\t\t\t}')
        lines.append('\t\t};')
        lines.append(f'\t\tstatic RegisterFunction_{struct_name} global_{struct_name}_function_register;')
        lines.append('')

    struct_names = [struct_name for struct_name, _, _ in results]
    return includes, force_lines, lines, struct_names, generated_enums


def emit_source(includes, force_lines, body_lines, generated_enums, enums):
    """
    1モジュール分の includes/force_lines/body_lines/generated_enums から、
    Reflection.generated.cpp の完全なソースコード文字列を組み立てる。
    """
    body_lines = list(body_lines)

    for enum_type in sorted(generated_enums):
        Enum.emit_registration(enum_type, enums[enum_type], body_lines)

    lines = []
    lines.append('#include <FoundationEngine/Prelude.h>')
    lines.append('#include <FoundationEngine/Reflection/ReflectionRegistry.h>')
    for include in sorted(set(includes)):
        lines.append(f'#include <{include}>')
    lines.append('')
    lines.extend(force_lines)
    lines.append('')
    lines.append('namespace SeedCore')
    lines.append('{')
    lines.append('\t namespace ScReflection')
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
    生成ファイルが書き換わったときに、対応する obj を消しておく。
    生成ファイル自体もタイムスタンプが更新されるので MSBuild の通常の
    増分判定だけで再コンパイルされるが、念のための保険として残している。

    かつてはここで .tlog(MSBuild の増分ビルド情報)も消していた。これは
    ヘッダごとに .reflection.cpp を生成し、毎ビルドでファイルが増減して
    いた頃の名残で、当時はプロジェクト構成が変わるため増分情報を捨てる
    必要があった。現在は vcxproj 登録済みの1ファイルの中身を書き換える
    だけでファイルの増減が起きないので不要。むしろ消すと全ファイルが
    再コンパイル対象になり、巨大な Prelude.h の PCH まで作り直しになる。
    """
    object_root = os.path.join(project_root, 'Runtime', 'Intermediate', module_name)
    for object_path in glob.glob(os.path.join(object_root, '**', 'Reflection.generated.obj'), recursive=True):
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
        pragma_lines.append(f'#pragma comment(linker, "/include:_force_reflection_{struct_name}")')

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
            unique_identifier.text = '{a1b2c3d4-refl-0000-0000-000000000001}'

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
    ヘッダーごとに新しい.reflection.cppを作る方式はやめて、既に vcxproj に
    登録済みの1ファイル(Reflection.generated.cpp)の中身だけを毎回まるごと
    書き換える。ファイルの追加/削除が発生しないので、PreBuildEvent内で
    Pythonが生成した内容がその回のビルドでそのままコンパイルされる
    (ファイルを新規追加すると、MSBuildは実行中のビルドで構成を再読み込み
    しないため、次のビルドまで反映されなかった)。

    UserProject/ 配下で定義された型は、FoundationEngine 側の集約ファイルへ
    混ぜず、UserProject/Reflection/Reflection.generated.cpp という別ファイルへ
    出力し UserProject.Cplusplus.vcxproj 側でコンパイルする。UserProject.Cplusplus.dll だけを
    再ビルドしてリフレクションフィールドを追加/削除/並べ替えても、
    そのオフセット情報は同じ UserProject.Cplusplus.dll 内で更新される — SeedCore.Cplusplus.dll
    (FoundationEngineの集約ファイル)側に古いオフセットが取り残されて
    ホットリロード時にメモリ破壊を起こす事故を防ぐ。
    """
    script_directory = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_directory))

    foundation_output_directory = os.path.join(project_root, 'FoundationEngine', 'Reflection')
    os.makedirs(foundation_output_directory, exist_ok=True)
    foundation_output_path = os.path.join(foundation_output_directory, 'Reflection.generated.cpp')

    user_project_output_directory = os.path.join(project_root, 'UserProject', 'Reflection')
    os.makedirs(user_project_output_directory, exist_ok=True)
    user_project_output_path = os.path.join(user_project_output_directory, 'Reflection.generated.cpp')

    enums, enum_headers = collect_enums(project_root)
    known_structs = collect_structs(project_root)

    buckets = {
        'foundation': {'struct_names': [], 'includes': [], 'force_lines': [], 'body_lines': [], 'generated_enums': set()},
        'user_project': {'struct_names': [], 'includes': [], 'force_lines': [], 'body_lines': [], 'generated_enums': set()},
    }

    for root, directories, files in os.walk(project_root):
        collect_directories(directories)
        for file in files:
            if not file.endswith(".h"):
                continue

            if file == REGISTRY_HEADER:
                continue

            full_path = os.path.join(root, file)
            result = emit_header(full_path, project_root, enums, enum_headers, known_structs)

            if result:
                includes, force_lines, body_lines, struct_names, generated_enums = result
                bucket = buckets['user_project'] if is_user_project_file(full_path, project_root) else buckets['foundation']
                bucket['includes'].extend(includes)
                bucket['force_lines'].extend(force_lines)
                bucket['body_lines'].extend(body_lines)
                bucket['struct_names'].extend(struct_names)
                bucket['generated_enums'].update(generated_enums)

    foundation_bucket = buckets['foundation']
    user_project_bucket = buckets['user_project']

    foundation_code = emit_source(
        foundation_bucket['includes'], foundation_bucket['force_lines'], foundation_bucket['body_lines'],
        foundation_bucket['generated_enums'], enums
    )
    user_project_code = emit_source(
        user_project_bucket['includes'], user_project_bucket['force_lines'], user_project_bucket['body_lines'],
        user_project_bucket['generated_enums'], enums
    )

    foundation_changed = update_source(foundation_output_path, foundation_code, 'Reflection.generated.cpp (FoundationEngine)')
    user_project_changed = update_source(user_project_output_path, user_project_code, 'Reflection.generated.cpp (UserProject)')

    if foundation_changed:
        update_objects(project_root, 'FoundationEngine.Cplusplus')
    if user_project_changed:
        update_objects(project_root, 'UserProject.Cplusplus')

    registry_cpp = os.path.join(project_root, 'FoundationEngine', 'Reflection', 'ReflectionRegistry.cpp')
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

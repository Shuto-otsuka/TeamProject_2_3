import os
import re

LABEL = "スクリプト参照"

EXTENSIONS = set()

# ゲームプレイコードが名前/パス/ID で Asset に辿り着く入口と、そのうち Asset を指す引数の位置。
# スクリプトから ResourceCache には直接触れないため、名前解決(ResourceCache::Find)を
# 経由する公開 API はこの3つだけ（Scene.h/.cpp, Prefab.h/.cpp 参照）。
# Scene::Change の第2引数はローディングシーン版ではパス/ID、フェード版では秒数なので、
# 解決できなかった場合も警告しない。
ASSET_ENTRY_ARGUMENTS = {
    "Scene::AssetID": (0,),
    "Prefab::Spawn": (0,),
    "Scene::Change": (0, 1),
}
ASSET_ENTRY_CALL_PATTERN = re.compile(r'\b(Scene::AssetID|Scene::Change|Prefab::Spawn)\s*\(')

# 引数が文字列リテラル1つ、またはそれを String(...) / String::intern(...) /
# std::filesystem::path(...) のような変換で包んだだけのものかを判定するパターン。
WRAPPED_STRING_LITERAL_PATTERN = re.compile(r'^(?:[A-Za-z_][\w:]*\s*[({]\s*)*(?:u8|L|u|U)?"((?:[^"\\]|\\.)*)"\s*(?:[)}]\s*)*$')

# Spawn(Uint32) / Change(Uint32) に Asset ID を数値リテラルで直接渡す呼び方。
INTEGER_LITERAL_PATTERN = re.compile(r'^(0[xX][0-9a-fA-F]+|\d+)[uUlL]*$')

# 引数が変数/メンバ/定数名だけのもの（a / a_ / this->a_ / obj.a / Ns::a）。
IDENTIFIER_ARGUMENT_PATTERN = re.compile(r'^(?:this\s*->\s*)?[A-Za-z_]\w*(?:\s*(?:::|\.|->)\s*[A-Za-z_]\w*)*$')

# 識別子が文字列リテラルで初期化/代入される箇所（name = "..." / name{"..."} / name("...")、
# 右辺が String(...) 等で包まれていてもよい）と、#define NAME "..."。
STRING_BINDING_PATTERN = re.compile(r'\b([A-Za-z_]\w*)\s*(?:=|\{|\()\s*(?:[A-Za-z_][\w:]*\s*[({]\s*)*(?:u8|L|u|U)?"((?:[^"\\]|\\.)*)"')
STRING_DEFINE_PATTERN = re.compile(r'#\s*define\s+([A-Za-z_]\w*)\s+(?:u8|L|u|U)?"((?:[^"\\]|\\.)*)"')


def split_call_arguments(content, open_index):
    arguments = []
    current = []
    depth = 0
    index = open_index + 1
    length = len(content)

    while index < length:
        c = content[index]

        if c == '"' or c == "'":
            current.append(c)
            index += 1
            while index < length and content[index] != c:
                if content[index] == '\\' and index + 1 < length:
                    current.append(content[index])
                    index += 1
                current.append(content[index])
                index += 1
            if index < length:
                current.append(content[index])
            index += 1
            continue

        if c in '([{':
            depth += 1
        elif c in ')]}':
            if depth == 0:
                arguments.append(''.join(current).strip())
                return arguments
            depth -= 1
        elif c == ',' and depth == 0:
            arguments.append(''.join(current).strip())
            current = []
            index += 1
            continue

        current.append(c)
        index += 1

    return None


def seed(context):
    """
    UserProject 以下の .cpp/.h を読み、コメントを除いた中身と、
    文字列リテラルで初期化/代入される変数・メンバ・定数、#define 定数の束縛
    （UserProject 全体から同名の束縛を集め、その全ての値を候補にする）を作っておく。
    """
    sources = {}
    bindings = {}

    for root, _, files in os.walk(context.user_project_root):
        for file in files:
            if not (file.endswith(".cpp") or file.endswith(".h")):
                continue

            full_path = os.path.join(root, file)
            try:
                with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
            except Exception:
                continue

            content = context.strip_comments(content)
            sources[full_path] = content

            for pattern in (STRING_BINDING_PATTERN, STRING_DEFINE_PATTERN):
                for name, literal in pattern.findall(content):
                    bindings.setdefault(name, set()).add(literal)

    context.store["script"] = (sources, bindings)
    return list(sources)


def collect(path, context):
    """
    ASSET_ENTRY_ARGUMENTS の入口 (Scene::AssetID / Scene::Change / Prefab::Spawn) に
    渡される Asset 引数を静的に解決する。引数は括弧の対応を取って切り出し、次の形を解決できる:
      - 文字列リテラル（String(...) 等の変換で包まれていてもよい）
      - 文字列リテラルで初期化/代入される変数・メンバ・定数、#define 定数
      - Asset ID の数値リテラル（Spawn(Uint32) / Change(Uint32) 版）
    インスペクタで設定される String フィールドの値は、ここではなく
    SceneCollect / PrefabCollect がシーン/プレハブの保存値から拾う。
    どれにも当てはまらない式や、解決先の Asset が無い引数は警告する。
    """
    sources, bindings = context.store["script"]
    content = sources[path]
    rel_source_path = os.path.relpath(path, context.user_project_root)
    referenced_ids = set()

    for match in ASSET_ENTRY_CALL_PATTERN.finditer(content):
        entry = match.group(1)
        line_number = content.count('\n', 0, match.start()) + 1

        arguments = split_call_arguments(content, match.end() - 1)
        if arguments is None:
            continue

        for argument_index in ASSET_ENTRY_ARGUMENTS[entry]:
            if argument_index >= len(arguments) or not arguments[argument_index]:
                continue

            argument = arguments[argument_index]
            warn = argument_index == 0

            literal_match = WRAPPED_STRING_LITERAL_PATTERN.match(argument)
            integer_match = INTEGER_LITERAL_PATTERN.match(argument)

            if literal_match:
                names = {literal_match.group(1)}
            elif integer_match:
                text = integer_match.group(1)
                asset_id = (int(text, 16) if text[:2].lower() == "0x" else int(text, 10)) & 0xFFFFFFFF
                if asset_id in context.guid_to_path:
                    referenced_ids.add(asset_id)
                elif warn:
                    print(f"警告: {entry}() に渡された Asset ID {text} が既知の Asset と一致しません ({rel_source_path}:{line_number})")
                continue
            elif IDENTIFIER_ARGUMENT_PATTERN.match(argument):
                names = bindings.get(re.split(r'::|\.|->', argument)[-1].strip(), set())
                if not names:
                    if warn:
                        print(f"警告: {entry}() の引数 {argument} の値を静的に解決できません ({rel_source_path}:{line_number}) — インスペクタで設定される値ならシーン/プレハブの保存値から拾います")
                    continue
            else:
                if warn:
                    print(f"警告: {entry}() の引数 {argument} は静的に解決できない式です ({rel_source_path}:{line_number}) — RuntimePackager はこの Asset を自動検出できません")
                continue

            resolved = set()
            for name in names:
                resolved |= context.resolve(name)

            if resolved:
                referenced_ids |= resolved
            elif warn:
                print(f"警告: {entry}() の引数 {argument} が既知の Asset パスと一致しません ({rel_source_path}:{line_number})")

    return [], referenced_ids

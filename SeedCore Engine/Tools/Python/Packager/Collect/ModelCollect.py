import os
import json
import urllib.parse

LABEL = "Model"

EXTENSIONS = {".gltf", ".glb", ".fbx", ".crister"}


def seed(context):
    return []


def collect(path, context):
    files = context.asset(path)
    if os.path.splitext(path)[1].lower() != ".gltf":
        return files, set()

    try:
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception:
        print(f"警告: .gltf を読み込めませんでした: {path}")
        return files, set()

    directory = os.path.dirname(path)
    for entry in data.get("buffers", []) + data.get("images", []):
        uri = entry.get("uri")
        if not uri or uri.startswith("data:"):
            continue
        dependency = os.path.normpath(os.path.join(directory, urllib.parse.unquote(uri)))
        if not os.path.isfile(dependency):
            print(f"警告: .gltf の依存ファイルが見つかりません: {dependency}")
            continue
        files.extend(context.asset(dependency))

    return files, set()

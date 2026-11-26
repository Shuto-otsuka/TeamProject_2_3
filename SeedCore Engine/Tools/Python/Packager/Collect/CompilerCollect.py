import os

LABEL = "DXC コピー"

EXTENSIONS = set()


def seed(context):
    paths = [os.path.join(context.project_root, "External", "DXC", file) for file in ("dxcompiler.dll", "dxil.dll")]
    for path in paths:
        if not os.path.exists(path):
            print(f"{os.path.basename(path)} が見つかりません: {path}")
            return None
    return paths


def collect(path, context):
    return [(os.path.join("Plugins", os.path.basename(path)), path)], set()

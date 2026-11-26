import os

LABEL = "ロゴ コピー"

EXTENSIONS = set()


def seed(context):
    logo_dir = os.path.join(context.project_root, "Runtime", "Logo")
    paths = [os.path.join(logo_dir, file) for file in os.listdir(logo_dir) if file.lower().endswith(".logo")] if os.path.isdir(logo_dir) else []
    if not paths:
        print(f"ロゴ (.logo) が見つかりません: {logo_dir}")
        return None
    return paths


def collect(path, context):
    return [context.place(path)], set()

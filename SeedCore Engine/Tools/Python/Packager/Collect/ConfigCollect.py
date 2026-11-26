import os

LABEL = "Config コピー"

EXTENSIONS = set()


def seed(context):
    paths = []
    config_dir = os.path.join(context.user_project_root, "Assets", "Config")
    if os.path.isdir(config_dir):
        for root, _, files in os.walk(config_dir):
            for file in files:
                if not file.lower().endswith(".scg") or file.lower() == "iconbindings.scg":
                    continue
                paths.append(os.path.join(root, file))
    return paths


def collect(path, context):
    return [context.place(path)], set()

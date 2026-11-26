import os
from Packager.Collect import SceneCollect

LABEL = "Prefab コピー"

EXTENSIONS = {".prefab"}


def seed(context):
    paths = []
    prefab_dir = os.path.join(context.user_project_root, "Assets", "Prefab")
    if os.path.isdir(prefab_dir):
        for root, _, files in os.walk(prefab_dir):
            for file in files:
                paths.append(os.path.join(root, file))
    return paths


def collect(path, context):
    referenced_ids = SceneCollect.walk(path, context) if os.path.splitext(path)[1].lower() in EXTENSIONS else set()
    return context.asset(path), referenced_ids

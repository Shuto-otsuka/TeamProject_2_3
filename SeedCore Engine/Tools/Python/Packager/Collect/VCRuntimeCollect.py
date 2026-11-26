import os

LABEL = "VC++ ランタイム コピー"

EXTENSIONS = set()


def seed(context):
    vc_runtime_dir = os.path.join(context.project_root, "Platform", "VCRuntime")
    paths = [os.path.join(vc_runtime_dir, file) for file in os.listdir(vc_runtime_dir) if file.lower().endswith(".dll")] if os.path.isdir(vc_runtime_dir) else []
    if not paths:
        print(f"VC++ ランタイムが見つかりません: {vc_runtime_dir}")
        return None
    return paths


def collect(path, context):
    return [(os.path.join("Plugins", os.path.basename(path)), path)], set()

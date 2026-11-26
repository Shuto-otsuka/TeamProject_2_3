import os
import re

LABEL = "Effekseer"

EXTENSIONS = {".efk", ".efkefc", ".effekseer"}

DEPENDENCY_PATTERN = re.compile(r'[^\x00-\x1f"*<>?|�]+?\.(?:png|jpg|jpeg|tga|dds|efkmodel|efkmat|efkcurve)(?=\x00)', re.IGNORECASE)


def seed(context):
    return []


def collect(path, context):
    files = context.asset(path)
    if os.path.splitext(path)[1].lower() == ".effekseer":
        return files, set()

    with open(path, 'rb') as f:
        raw = f.read()

    directory = os.path.dirname(path)
    dependencies = set()
    for start in (0, 1):
        text = raw[start:].decode("utf-16-le", errors="replace")
        for match in DEPENDENCY_PATTERN.finditer(text):
            candidate = match.group(0)
            for offset in range(len(candidate)):
                relative_path = candidate[offset:]
                if relative_path[0] in "/\\":
                    continue
                dependency = os.path.normpath(os.path.join(directory, relative_path))
                if os.path.isfile(dependency):
                    dependencies.add(dependency)
                    break

    for dependency in sorted(dependencies):
        if os.path.relpath(dependency, context.user_project_root).startswith(".."):
            print(f"警告: UserProject の外にある Effekseer の依存ファイルは同梱しません: {dependency}")
            continue
        files.extend(context.asset(dependency))

    return files, set()

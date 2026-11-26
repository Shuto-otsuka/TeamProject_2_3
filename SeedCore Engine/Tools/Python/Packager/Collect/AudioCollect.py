import os

LABEL = "Audio"

EXTENSIONS = {".mp3", ".wav", ".acb", ".awb", ".audio"}


def seed(context):
    return []


def collect(path, context):
    files = context.asset(path)
    stem, extension = os.path.splitext(path)
    if extension.lower() == ".acb" and os.path.isfile(stem + ".awb"):
        files.extend(context.asset(stem + ".awb"))
    return files, set()

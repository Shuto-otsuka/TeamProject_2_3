LABEL = "Font"

EXTENSIONS = {".ttf", ".otf", ".ttc"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

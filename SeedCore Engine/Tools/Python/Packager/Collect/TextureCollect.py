LABEL = "Texture"

EXTENSIONS = {".png", ".jpg", ".jpeg", ".dds", ".texture"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

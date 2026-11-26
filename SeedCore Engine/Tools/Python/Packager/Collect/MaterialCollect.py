LABEL = "Material"

EXTENSIONS = {".material"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

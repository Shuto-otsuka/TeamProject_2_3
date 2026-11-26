LABEL = "Animation"

EXTENSIONS = {".animation"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

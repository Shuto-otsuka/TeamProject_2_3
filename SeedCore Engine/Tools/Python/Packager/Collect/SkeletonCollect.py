LABEL = "Skeleton"

EXTENSIONS = {".skeleton"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

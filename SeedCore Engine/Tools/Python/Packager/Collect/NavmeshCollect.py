LABEL = "Navmesh"

EXTENSIONS = {".navmesh"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

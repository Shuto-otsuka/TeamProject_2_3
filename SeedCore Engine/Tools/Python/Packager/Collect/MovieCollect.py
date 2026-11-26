LABEL = "Movie"

EXTENSIONS = {".mp4", ".movie"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

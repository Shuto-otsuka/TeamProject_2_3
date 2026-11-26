LABEL = "Skymap"

EXTENSIONS = {".hdr", ".skymap"}


def seed(context):
    return []


def collect(path, context):
    return context.asset(path), set()

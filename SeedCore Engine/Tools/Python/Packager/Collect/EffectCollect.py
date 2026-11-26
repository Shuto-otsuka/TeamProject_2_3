import struct

LABEL = "Effect"

EXTENSIONS = {".zephyr"}


def seed(context):
    return []


def collect(path, context):
    referenced_ids = set()
    fields = context.read_archive(path)
    if fields is None:
        print(f"警告: .zephyr を復号できませんでした: {path}")
        return context.asset(path), referenced_ids

    def elements(payload):
        if not payload or len(payload) < 4:
            return
        count = struct.unpack_from('<I', payload, 0)[0]
        offset = 4
        for _ in range(count):
            if offset + 4 > len(payload):
                return
            size = struct.unpack_from('<I', payload, offset)[0]
            offset += 4
            yield context.fields(payload[offset:offset + size])
            offset += size

    for emitter in elements(fields.get(context.fnv1a("emitters"))):
        for node in elements(emitter.get(context.fnv1a("nodes"))):
            for parameter in elements(node.get(context.fnv1a("parameters"))):
                asset = parameter.get(context.fnv1a("asset"))
                if asset and len(asset) == 4:
                    asset_id = struct.unpack('<I', asset)[0]
                    if asset_id:
                        referenced_ids.add(asset_id)

    return context.asset(path), referenced_ids

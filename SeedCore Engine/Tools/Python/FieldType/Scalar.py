ATTRIBUTE_TYPE_MAP = {
    "Int": "AttributeType::Int",
    "int": "AttributeType::Int",
    "Uint": "AttributeType::Int",
    "unsigned int": "AttributeType::Int",

    "Int8": "AttributeType::Int",
    "int8_t": "AttributeType::Int",
    "Uint8": "AttributeType::Int",
    "uint8_t": "AttributeType::Int",

    "Int16": "AttributeType::Int",
    "int16_t": "AttributeType::Int",
    "Uint16": "AttributeType::Int",
    "uint16_t": "AttributeType::Int",

    "Int32": "AttributeType::Int",
    "int32_t": "AttributeType::Int",
    "Uint32": "AttributeType::Int",
    "uint32_t": "AttributeType::Int",

    "Int64": "AttributeType::Int",
    "int64_t": "AttributeType::Int",
    "Uint64": "AttributeType::Int",
    "uint64_t": "AttributeType::Int",

    "Short": "AttributeType::Int",
    "short": "AttributeType::Int",
    "Long": "AttributeType::Int",
    "long": "AttributeType::Int",

    "Float": "AttributeType::Float",
    "float": "AttributeType::Float",
    "Double": "AttributeType::Float",
    "double": "AttributeType::Float",

    "Bool": "AttributeType::Bool",
    "bool": "AttributeType::Bool",

    "Vector2": "AttributeType::Vector2",

    "Vector3": "AttributeType::Vector3",
    "Vector4": "AttributeType::Vector4",

    "String": "AttributeType::String",

    "Color": "AttributeType::Color",
}


def matches(field, context):
    """配列でも enum でも既知の構造体でもないフィールド。"""
    return field.array_type_ is None and field.enum_type_ is None and field.type_ not in context.known_structs_


def emit(field, context, lines):
    """condition / clamp / serialize_only / revision のどれかがあれば FieldInfo ブロック、無ければ1行で出力する。"""
    attribute_type = ATTRIBUTE_TYPE_MAP.get(field.type_, "AttributeType::Unknown")
    if field.condition_ or field.clamp_min_ is not None or field.serialize_only_ or field.revision_:
        emit_detailed(field, context, attribute_type, lines)
    else:
        emit_inline(field, context, attribute_type, lines)


def emit_inline(field, context, attribute_type, lines):
    """Payload のフィールドなら、4つ目の要素に PayloadType を付ける。"""
    asset_type = f', PayloadType::{field.asset_type_}' if field.asset_type_ else ''
    lines.append(
        f'\t\t\t\t\toutInfo.push_back({{ '
        f'String("{field.display_name_}"), '
        f'offsetof({context.struct_name_}, {field.name_}), '
        f'{attribute_type}{asset_type} '
        f'}});'
    )


def emit_detailed(field, context, attribute_type, lines):
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tFieldInfo fi;')
    lines.append(f'\t\t\t\t\t\tfi.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\tfi.offset_ = offsetof({context.struct_name_}, {field.name_});')
    lines.append(f'\t\t\t\t\t\tfi.type_ = {attribute_type};')
    if field.asset_type_:
        lines.append(f'\t\t\t\t\t\tfi.assetType_ = PayloadType::{field.asset_type_};')
    if field.condition_:
        lines.append(f'\t\t\t\t\t\tfi.enableIf_ = [](void* p) -> Bool {{ auto& o = *static_cast<{context.struct_name_}*>(p); return {field.condition_}; }};')
    if field.clamp_min_ is not None:
        lines.append(f'\t\t\t\t\t\tfi.clampMin_ = {field.clamp_min_};')
        lines.append(f'\t\t\t\t\t\tfi.clampMax_ = {field.clamp_max_};')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\tfi.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\tfi.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(fi));')
    lines.append(f'\t\t\t\t\t}}')

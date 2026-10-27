def matches(field, context):
    """配列ではなく、型がリフレクション対象の struct/class (known_structs_) のフィールド。"""
    return field.array_type_ is None and field.enum_type_ is None and field.type_ in context.known_structs_


def emit(field, context, lines):
    """AttributeType::Struct + nestedTypeName_ として出力する。clamp は付かない。"""
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tFieldInfo fi;')
    lines.append(f'\t\t\t\t\t\tfi.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\tfi.offset_ = offsetof({context.struct_name_}, {field.name_});')
    lines.append(f'\t\t\t\t\t\tfi.type_ = AttributeType::Struct;')
    lines.append(f'\t\t\t\t\t\tfi.nestedTypeName_ = String("{field.type_}");')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\tfi.editorVisible_ = false;')
    if field.condition_:
        lines.append(f'\t\t\t\t\t\tfi.enableIf_ = [](void* p) -> Bool {{ auto& o = *static_cast<{context.struct_name_}*>(p); return {field.condition_}; }};')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\tfi.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(fi));')
    lines.append(f'\t\t\t\t\t}}')

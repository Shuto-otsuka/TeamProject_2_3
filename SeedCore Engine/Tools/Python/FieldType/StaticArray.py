from FieldType.Scalar import ATTRIBUTE_TYPE_MAP


def matches(field, context):
    """T[N] / StaticArray<T, N> / std::array<T, N> のフィールド。"""
    return field.array_type_ is not None and not field.array_type_[2]


def emit(field, context, lines):
    """要素の型がリフレクション対象の構造体かどうかで出力を分ける。"""
    element_type, count, _is_dynamic = field.array_type_
    if element_type in context.known_structs_:
        emit_struct_elements(field, element_type, count, context, lines)
    else:
        emit_scalar_elements(field, element_type, count, context, lines)


def emit_struct_elements(field, element_type, count, context, lines):
    """
    header (Struct + nestedTypeName_ + array_.size_ = N) と、N 個の Struct の FieldInfo を出力する。
    要素数が固定なので add_/remove_ は持たない。要素の offset_ は header の offset_ から要素の大きさずつ進めた位置で、
    インスペクターが offset_ で並べ替えても header の直後に要素が並ぶ。
    """
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tFieldInfo header;')
    lines.append(f'\t\t\t\t\t\theader.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\theader.offset_ = offsetof({context.struct_name_}, {field.name_});')
    lines.append(f'\t\t\t\t\t\theader.type_ = AttributeType::Struct;')
    lines.append(f'\t\t\t\t\t\theader.nestedTypeName_ = String("{element_type}");')
    lines.append(f'\t\t\t\t\t\theader.array_.size_ = {count};')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\theader.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\theader.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(header));')
    lines.append(f'\t\t\t\t\t\tfor (Size i = 0; i < {count}; ++i)')
    lines.append(f'\t\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\t\tFieldInfo elementInfo;')
    lines.append(f'\t\t\t\t\t\t\telementInfo.offset_ = offsetof({context.struct_name_}, {field.name_}) + i * sizeof({element_type});')
    lines.append(f'\t\t\t\t\t\t\telementInfo.type_ = AttributeType::Struct;')
    lines.append(f'\t\t\t\t\t\t\telementInfo.nestedTypeName_ = String("{element_type}");')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\t\telementInfo.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\t\telementInfo.revision_ = true;')
    lines.append(f'\t\t\t\t\t\t\toutInfo.push_back(std::move(elementInfo));')
    lines.append(f'\t\t\t\t\t\t}}')
    lines.append(f'\t\t\t\t\t}}')


def emit_scalar_elements(field, element_type, count, context, lines):
    """header (array_.size_ = N) と、N 個の要素をループ展開した FieldInfo を出力する。"""
    attribute_type = ATTRIBUTE_TYPE_MAP.get(element_type, "AttributeType::Unknown")
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tFieldInfo header;')
    lines.append(f'\t\t\t\t\t\theader.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\theader.offset_ = offsetof({context.struct_name_}, {field.name_});')
    lines.append(f'\t\t\t\t\t\theader.type_ = {attribute_type};')
    lines.append(f'\t\t\t\t\t\theader.array_.size_ = {count};')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\theader.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\theader.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(header));')
    for element_index in range(count):
        if field.serialize_only_:
            revision_line = ' elem.revision_ = true;' if field.revision_ else ''
            lines.append(f'\t\t\t\t\t\t{{ FieldInfo elem{{ String("[{element_index}]"), offsetof({context.struct_name_}, {field.name_}) + {element_index} * sizeof({element_type}), {attribute_type} }}; elem.editorVisible_ = false;{revision_line} outInfo.push_back(std::move(elem)); }}')
        elif field.revision_:
            lines.append(f'\t\t\t\t\t\t{{ FieldInfo elem{{ String("[{element_index}]"), offsetof({context.struct_name_}, {field.name_}) + {element_index} * sizeof({element_type}), {attribute_type} }}; elem.revision_ = true; outInfo.push_back(std::move(elem)); }}')
        else:
            lines.append(
                f'\t\t\t\t\t\toutInfo.push_back({{ '
                f'String("[{element_index}]"), offsetof({context.struct_name_}, {field.name_}) + {element_index} * sizeof({element_type}), {attribute_type} '
                f'}});'
            )
    lines.append(f'\t\t\t\t\t}}')

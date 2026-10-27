from FieldType.Scalar import ATTRIBUTE_TYPE_MAP


def matches(field, context):
    """DynamicArray<T> / std::vector<T> のフィールド。"""
    return field.array_type_ is not None and field.array_type_[2]


def emit(field, context, lines):
    """要素の型がリフレクション対象の構造体かどうかで出力を分ける。"""
    element_type = field.array_type_[0]
    if element_type in context.known_structs_:
        emit_struct_elements(field, element_type, context, lines)
    else:
        emit_scalar_elements(field, element_type, context, lines)


def emit_struct_elements(field, element_type, context, lines):
    """header (Struct + add_/remove_) と、要素ごとに directPtr_ を持つ Struct の FieldInfo を出力する。"""
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tauto& arr = obj.{field.name_};')
    lines.append(f'\t\t\t\t\t\tFieldInfo header;')
    lines.append(f'\t\t\t\t\t\theader.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\theader.offset_ = 0;')
    lines.append(f'\t\t\t\t\t\theader.type_ = AttributeType::Struct;')
    lines.append(f'\t\t\t\t\t\theader.nestedTypeName_ = String("{element_type}");')
    lines.append(f'\t\t\t\t\t\theader.array_.size_ = arr.size();')
    lines.append(f'\t\t\t\t\t\theader.array_.add_ = [&obj]() {{ obj.{field.name_}.push_back({{}}); }};')
    lines.append(f'\t\t\t\t\t\theader.array_.remove_ = [&obj](Size idx) {{ if (idx < obj.{field.name_}.size()) obj.{field.name_}.erase(obj.{field.name_}.begin() + idx); }};')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\theader.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\theader.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(header));')
    lines.append(f'\t\t\t\t\t\tfor (Size i = 0; i < arr.size(); ++i)')
    lines.append(f'\t\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\t\tFieldInfo elementInfo;')
    lines.append(f'\t\t\t\t\t\t\telementInfo.offset_ = 0;')
    lines.append(f'\t\t\t\t\t\t\telementInfo.type_ = AttributeType::Struct;')
    lines.append(f'\t\t\t\t\t\t\telementInfo.directPtr_ = &arr[i];')
    lines.append(f'\t\t\t\t\t\t\telementInfo.nestedTypeName_ = String("{element_type}");')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\t\telementInfo.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\t\telementInfo.revision_ = true;')
    lines.append(f'\t\t\t\t\t\t\toutInfo.push_back(std::move(elementInfo));')
    lines.append(f'\t\t\t\t\t\t}}')
    lines.append(f'\t\t\t\t\t}}')


def emit_scalar_elements(field, element_type, context, lines):
    """
    header (基本型 + add_/remove_) と、要素ごとに "[i]" の名前で &arr[i] を指す FieldInfo を出力する。
    Payload のフィールドなら header に assetType_ と、ドロップで追加した要素を書き込むための lastPtr_ を付け、
    要素にも同じ PayloadType を持たせる。
    """
    attribute_type = ATTRIBUTE_TYPE_MAP.get(element_type, "AttributeType::Unknown")
    asset_type = field.asset_type_ or 'None'
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tauto& arr = obj.{field.name_};')
    lines.append(f'\t\t\t\t\t\tFieldInfo header;')
    lines.append(f'\t\t\t\t\t\theader.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\theader.offset_ = 0;')
    lines.append(f'\t\t\t\t\t\theader.type_ = {attribute_type};')
    if field.asset_type_:
        lines.append(f'\t\t\t\t\t\theader.assetType_ = PayloadType::{field.asset_type_};')
    lines.append(f'\t\t\t\t\t\theader.array_.size_ = arr.size();')
    lines.append(f'\t\t\t\t\t\theader.array_.add_ = [&obj]() {{ obj.{field.name_}.push_back({{}}); }};')
    lines.append(f'\t\t\t\t\t\theader.array_.remove_ = [&obj](Size idx) {{ if (idx < obj.{field.name_}.size()) obj.{field.name_}.erase(obj.{field.name_}.begin() + idx); }};')
    if field.asset_type_:
        lines.append(f'\t\t\t\t\t\theader.array_.lastPtr_ = [&obj]() -> void* {{ return &obj.{field.name_}.back(); }};')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\theader.editorVisible_ = false;')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\theader.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(header));')
    lines.append(f'\t\t\t\t\t\tfor (Size i = 0; i < arr.size(); ++i)')
    lines.append(f'\t\t\t\t\t\t{{')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\t\tFieldInfo elementInfo{{ String("[" + std::to_string(i) + "]"), 0, {attribute_type}, PayloadType::{asset_type}, &arr[i] }};')
        lines.append(f'\t\t\t\t\t\t\telementInfo.editorVisible_ = false;')
        if field.revision_:
            lines.append(f'\t\t\t\t\t\t\telementInfo.revision_ = true;')
        lines.append(f'\t\t\t\t\t\t\toutInfo.push_back(std::move(elementInfo));')
    else:
        if field.revision_:
            lines.append(f'\t\t\t\t\t\t\tFieldInfo elementInfo{{ String("[" + std::to_string(i) + "]"), 0, {attribute_type}, PayloadType::{asset_type}, &arr[i] }};')
            lines.append(f'\t\t\t\t\t\t\telementInfo.revision_ = true;')
            lines.append(f'\t\t\t\t\t\t\toutInfo.push_back(std::move(elementInfo));')
        else:
            lines.append(
                f'\t\t\t\t\t\t\toutInfo.push_back({{ '
                f'String("[" + std::to_string(i) + "]"), 0, {attribute_type}, '
                f'PayloadType::{asset_type}, &arr[i] '
                f'}});'
            )
    lines.append(f'\t\t\t\t\t\t}}')
    lines.append(f'\t\t\t\t\t}}')

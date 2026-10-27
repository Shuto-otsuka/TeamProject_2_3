def matches(field, context):
    """配列ではなく、型が enum class のフィールド。"""
    return field.array_type_ is None and field.enum_type_ is not None


def emit(field, context, lines):
    """AttributeType::Enum として出力し、使った所有者付きの enum 名を generated_enums_ に追加する。"""
    lines.append(f'\t\t\t\t\t{{')
    lines.append(f'\t\t\t\t\t\tFieldInfo fi;')
    lines.append(f'\t\t\t\t\t\tfi.name_ = String("{field.display_name_}");')
    lines.append(f'\t\t\t\t\t\tfi.offset_ = offsetof({context.struct_name_}, {field.name_});')
    lines.append(f'\t\t\t\t\t\tfi.type_ = AttributeType::Enum;')
    lines.append(f'\t\t\t\t\t\tfi.enum_.typeName_ = String("{field.enum_type_}");')
    if field.serialize_only_:
        lines.append(f'\t\t\t\t\t\tfi.editorVisible_ = false;')
    if field.condition_:
        lines.append(f'\t\t\t\t\t\tfi.enableIf_ = [](void* p) -> Bool {{ auto& o = *static_cast<{context.struct_name_}*>(p); return {field.condition_}; }};')
    if field.clamp_min_ is not None:
        lines.append(f'\t\t\t\t\t\tfi.clampMin_ = {field.clamp_min_};')
        lines.append(f'\t\t\t\t\t\tfi.clampMax_ = {field.clamp_max_};')
    if field.revision_:
        lines.append(f'\t\t\t\t\t\tfi.revision_ = true;')
    lines.append(f'\t\t\t\t\t\toutInfo.push_back(std::move(fi));')
    lines.append(f'\t\t\t\t\t}}')

    context.generated_enums_.add(field.enum_type_)


def emit_registration(enum_type, entries, lines):
    """
    enum の値と名前を、所有者付きの名前 enum_type をキーにして EnumRegistry へ登録する RegisterEnum_ 構造体を出力する。
    構造体名には :: が使えないので _ に置き換える。
    """
    identifier = enum_type.replace('::', '_')
    lines.append(f'\t\tstruct RegisterEnum_{identifier}')
    lines.append('\t\t{')
    lines.append(f'\t\t\tRegisterEnum_{identifier}()')
    lines.append('\t\t\t{')
    lines.append(f'\t\t\t\tEnumRegistry::Register(String("{enum_type}"), {{')
    for entry in entries:
        lines.append(f'\t\t\t\t\t{{ static_cast<Int>({enum_type}::{entry}), String("{entry}") }},')
    lines.append('\t\t\t\t});')
    lines.append('\t\t\t}')
    lines.append('\t\t};')
    lines.append(f'\t\tstatic RegisterEnum_{identifier} global_{identifier}_enum_register;')
    lines.append('')

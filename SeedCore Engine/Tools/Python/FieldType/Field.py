from dataclasses import dataclass


@dataclass
class FieldDecl:
    """
    SC_REFLECTION_FIELD 系 / SC_SERIALIZE_FIELD / SC_PAYLOAD_FIELD 系が付いた1フィールド分の情報。
    condition_ は出力時点で、そのまま enableIf_ に書ける形に解決済み。
    array_type_ は (要素の型, 個数 or None, 可変長かどうか) で、配列でなければ None。
    enum_type_ は Reflection.parse_enum_type で解決した所有者付きの enum 名で、enum でなければ None。
    asset_type_ は SC_PAYLOAD_FIELD に書いた PayloadType の名前で、Payload のフィールドでなければ None。
    """
    type_: str
    name_: str
    display_name_: str
    bracket_: str
    array_type_: tuple
    enum_type_: str
    condition_: str
    clamp_min_: str
    clamp_max_: str
    serialize_only_: bool
    asset_type_: str = None


@dataclass
class EmitContext:
    """
    1つの struct/class を出力するときに、各フィールド型の matches/emit が参照する情報。
    generated_enums_ には Enum 型のフィールドが使った所有者付きの enum 名が追加される。
    """
    struct_name_: str
    known_structs_: dict
    generated_enums_: set
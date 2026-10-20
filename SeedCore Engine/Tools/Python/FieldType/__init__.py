"""
フィールドの型ごとの出力処理。Reflection.py と Payload.py の両方が使う。
各モジュールは matches(field, context) と emit(field, context, lines) を持つ。
新しい型を足すときはモジュールを1つ追加し、下のリストに登録する。
"""
from FieldType import DynamicArray, Enum, Scalar, StaticArray, Struct

REFLECTION_FIELD_TYPES = [Enum, Struct, Scalar, DynamicArray, StaticArray]

PAYLOAD_FIELD_TYPES = [Scalar, DynamicArray]
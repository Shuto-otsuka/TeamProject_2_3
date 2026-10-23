#pragma once
#include <FoundationEngine/Reflection/ReflectionRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Serialized value of a single reflected field: a discriminated
	* union of every scalar AttributeType, plus (via isArray_/children_)
	* support for array-valued and nested-struct fields. Only the
	* member matching type_ holds meaningful data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* リフレクションされた単一フィールドのシリアライズ済みの値。
	* あらゆるスカラー AttributeType のタグ付き共用体であり、
	* （isArray_/children_ を通じて）配列値フィールドとネストされた
	* 構造体フィールドもサポートする。type_ に一致するメンバのみが
	* 意味のあるデータを持つ。
	*/
	struct BlueprintField
	{
		/// [EN] The field's name, used to match it back to a FieldInfo on restore.
		/// [JP] フィールドの名前。復元時に FieldInfo と対応付けるために使う。
		String name_;

		/// [EN] Which member below holds this field's actual value.
		/// [JP] 以下のどのメンバがこのフィールドの実際の値を保持するかを示す。
		AttributeType type_ = AttributeType::Unknown;

		/// [EN] Value when type_ is Float.
		/// [JP] type_ が Float の場合の値。
		Float floatValue_ = 0.0f;

		/// [EN] Value when type_ is Vector2.
		/// [JP] type_ が Vector2 の場合の値。
		Vector2 vector2Value_ = Vector2(0.0f, 0.0f);

		/// [EN] Value when type_ is Vector3.
		/// [JP] type_ が Vector3 の場合の値。
		Vector3 vector3Value_ = Vector3(0.0f, 0.0f, 0.0f);

		/// [EN] Value when type_ is Vector4.
		/// [JP] type_ が Vector4 の場合の値。
		Vector4 vector4Value_ = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

		/// [EN] Value when type_ is Color.
		/// [JP] type_ が Color の場合の値。
		Color colorValue_ = Color(1.0f, 1.0f, 1.0f, 1.0f);

		/// [EN] Value when type_ is Bool.
		/// [JP] type_ が Bool の場合の値。
		Bool boolValue_ = false;

		/// [EN] Value when type_ is Int or Enum.
		/// [JP] type_ が Int または Enum の場合の値。
		Int intValue_ = 0;

		/// [EN] Value when type_ is String.
		/// [JP] type_ が String の場合の値。
		String stringValue_ = String();

		/// [EN] Whether this field represents an array; if true, children_ holds one entry per array element instead of a scalar value.
		/// [JP] このフィールドが配列を表すかどうか。true の場合、children_ はスカラー値の代わりに配列の各要素を1エントリずつ保持する。
		Bool isArray_ = false;

		/// [EN] For array fields, one entry per element; for nested-struct fields, the struct's own captured field list. Empty for plain scalar fields.
		/// [JP] 配列フィールドの場合は要素ごとの1エントリ。ネストされた構造体フィールドの場合はその構造体自身の取得済みフィールド一覧。単純なスカラーフィールドの場合は空。
		DynamicArray<BlueprintField> children_;

		/**
		* [EN]
		* Serialization hook: reads/writes every member above. type_ is
		* round-tripped through a plain Int since the archive cannot
		* serialize an enum class directly.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック: 上記の全メンバを読み書きする。
		* アーカイブは enum class を直接シリアライズできないため、type_ は
		* 素の Int を経由して往復させる。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			Int typeValue = static_cast<Int>(type_);
			archive.Field("name", name_);
			archive.Field("type", typeValue);
			archive.Field("float", floatValue_);
			archive.Field("vector2", vector2Value_);
			archive.Field("vector3", vector3Value_);
			archive.Field("vector4", vector4Value_);
			archive.Field("color", colorValue_);
			archive.Field("bool", boolValue_);
			archive.Field("int", intValue_);
			archive.Field("string", stringValue_);
			archive.Field("is_array", isArray_);
			archive.Field("children", children_);
			type_ = static_cast<AttributeType>(typeValue);
		}
	};

	/**
	* [EN]
	* Serialized snapshot of a single non-builtin component attached to
	* an actor: its registered type name plus every one of its reflected fields.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor にアタッチされた単一の非組み込みコンポーネントのシリアライズ
	* 済みスナップショット。その登録済み型名と、リフレクションされた
	* 全フィールドを保持する。
	*/
	struct BlueprintComponent
	{
		/// [EN] The component's registered type name, used to look it up in ComponentRegistry on restore.
		/// [JP] コンポーネントの登録済み型名。復元時に ComponentRegistry から検索するために使う。
		String componentName_;

		/// [EN] Every reflected field captured for this component.
		/// [JP] このコンポーネントについて取得済みの、全リフレクションフィールド。
		DynamicArray<BlueprintField> fields_;

		/**
		* [EN]
		* Serialization hook: reads/writes componentName_ and fields_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック: componentName_ と fields_ を読み書きする。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("component", componentName_);
			archive.Field("fields", fields_);
		}

		/**
		* [EN]
		* Restores the captured field values onto componentData, a live
		* instance of the type named componentName_, matched by field name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 取得済みのフィールド値を、componentName_ という名前の型の生きた
		* インスタンスである componentData へ、フィールド名で対応付けて復元する。
		*/
		SEEDCORE_API void Apply(void* componentData)const;

		/**
		* [EN]
		* Captures every reflected field of componentData, a live instance
		* of the type named componentName, into a new BlueprintComponent.
		* Useful when a type's own storage is about to be destroyed (e.g.
		* before a hot-reload) and only its field values need to survive.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* componentName という名前の型の生きたインスタンスである
		* componentData の、リフレクションされた全フィールドを、新しい
		* BlueprintComponent へ取得する。型自身のストレージがこれから破棄
		* される場合（ホットリロード前など）に、フィールド値だけを残したい
		* ときに使う。
		*/
		SEEDCORE_API static BlueprintComponent Capture(const String& componentName, void* componentData);

	private:
		/**
		* [EN]
		* Recursively restores every reflected field of the type named
		* typeName (rooted at data) from savedFields, matching by field
		* name. A first pass grows every saved array field to its saved
		* element count before any element pointer is read; a second pass
		* applies each field's value.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* typeName という名前の型（data を起点とする）の、リフレクション
		* された全フィールドを savedFields から、フィールド名で対応付けて
		* 再帰的に復元する。第1パスで、要素へのポインタを読む前に保存済みの
		* 各配列フィールドをその要素数まで伸ばし、第2パスで各フィールドの値を
		* 適用する。
		*/
		static void ApplyFields(const String& typeName, void* data, const DynamicArray<BlueprintField>& savedFields);

		/**
		* [EN]
		* Writes match's saved value into ptr, based on field's
		* AttributeType; a saved value of a different type is skipped and
		* the field keeps its default.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* field の AttributeType に基づき、match の保存済みの値を ptr へ
		* 書き込む。型の違う保存値は飛ばし、フィールドは既定値のまま残す。
		*/
		static void ApplyScalar(const FieldInfo& field, void* ptr, const BlueprintField& match);

		/**
		* [EN]
		* Recursively captures every reflected field of the type named
		* typeName (rooted at data) into outFields: an array field becomes
		* one entry per element, a nested-struct field recurses, and
		* everything else is captured as a plain scalar.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* typeName という名前の型（data を起点とする）の、リフレクション
		* された全フィールドを outFields へ再帰的に取得する: 配列フィールドは
		* 要素ごとに1エントリになり、ネストされた構造体フィールドは再帰し、
		* それ以外は単純なスカラーとして取得する。
		*/
		static void CaptureFields(const String& typeName, void* data, DynamicArray<BlueprintField>& outFields);

		/**
		* [EN]
		* Captures a single non-array, non-nested field's current value
		* from ptr into a new BlueprintField, based on field's AttributeType.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* field の AttributeType に基づき、ptr から単一の非配列・非ネスト
		* フィールドの現在の値を、新しい BlueprintField へ取得する。
		*/
		static BlueprintField CaptureScalar(const FieldInfo& field, void* ptr);

		/**
		* [EN]
		* Fills fields with typeName's reflected fields (from
		* ReflectionRegistry) and, if it's a payload type, its
		* asset-reference fields too (from PayloadRegistry).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* typeName のリフレクションされたフィールド（ReflectionRegistry
		* から）を fields へ書き込む。それがペイロード型であれば、その
		* アセット参照フィールド（PayloadRegistry から）も書き込む。
		*/
		static void Fields(const String& typeName, void* data, DynamicArray<FieldInfo>& fields);
	};
}

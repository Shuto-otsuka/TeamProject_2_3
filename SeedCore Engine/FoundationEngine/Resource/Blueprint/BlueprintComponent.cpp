#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>

namespace SeedCore
{
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
	void BlueprintComponent::Apply(void* componentData)const
	{
		ApplyFields(componentName_, componentData, fields_);
	}

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
	BlueprintComponent BlueprintComponent::Capture(const String& componentName, void* componentData)
	{
		BlueprintComponent serializedComponent{};
		serializedComponent.componentName_ = componentName;
		CaptureFields(componentName, componentData, serializedComponent.fields_);
		return serializedComponent;
	}

	/**
	* [EN]
	* Recursively restores every reflected field of the type named
	* typeName (rooted at data) from savedFields, matching by field
	* name. Runs a first pass that grows every saved array field to
	* its saved element count before any per-element pointer is
	* read, then a second pass that actually applies each field's value.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* typeName という名前の型（data を起点とする）の、リフレクション
	* された全フィールドを savedFields から復元する。フィールド名で
	* 対応付ける。要素ごとのポインタを読む前に、保存済みの各配列
	* フィールドをその保存済み要素数まで伸ばす第1パスを実行し、その後
	* 実際に各フィールドの値を適用する第2パスを実行する。
	*/
	void BlueprintComponent::ApplyFields(const String& typeName, void* data, const DynamicArray<BlueprintField>& savedFields)
	{
		if (savedFields.empty())
		{
			return;
		}

		/// [EN] First pass: grow every saved array to its saved length before reading element pointers, since push_back may reallocate and invalidate earlier directPtr_ values.
		/// [JP] 第1パス: 要素へのポインタを読む前に、保存された配列を保存時の要素数まで伸ばし切る。push_back は再確保で先に取った directPtr_ を無効にしうる。
		{
			DynamicArray<FieldInfo> fields;
			Fields(typeName, data, fields);

			for (const FieldInfo& field : fields)
			{
				if (!field.array_.add_)
				{
					continue;
				}

				auto matchIt = std::ranges::find(savedFields, field.name_, &BlueprintField::name_);
				const BlueprintField* match = matchIt != savedFields.end() ? &*matchIt : nullptr;
				if (!match)
				{
					continue;
				}

				Size currentCount = field.array_.size_;
				while (currentCount < match->children_.size())
				{
					field.array_.add_();
					++currentCount;
				}
			}
		}

		/// [EN] Second pass: re-collect field infos (now with grown arrays and therefore stable pointers) and actually apply each saved value.
		/// [JP] 第2パス: フィールド情報を再収集し（配列は伸ばされ済みで、ポインタは安定している）、実際に各保存済みの値を適用する。
		DynamicArray<FieldInfo> fields;
		Fields(typeName, data, fields);

		for (Size index = 0; index < fields.size(); ++index)
		{
			const FieldInfo& field = fields[index];

			auto matchIt = std::ranges::find(savedFields, field.name_, &BlueprintField::name_);
			const BlueprintField* match = matchIt != savedFields.end() ? &*matchIt : nullptr;

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				Size count = field.array_.size_;

				if (match)
				{
					for (Size element = 0; element < count && element < match->children_.size() && (index + 1 + element) < fields.size(); ++element)
					{
						const FieldInfo& elementField = fields[index + 1 + element];
						void* ptr = elementField.directPtr_ ? elementField.directPtr_ : (static_cast<Uint8*>(data) + elementField.offset_);
						const BlueprintField& savedElement = match->children_[element];

						if (!elementField.nestedTypeName_.view().empty())
						{
							ApplyFields(elementField.nestedTypeName_, ptr, savedElement.children_);
						}
						else
						{
							ApplyScalar(elementField, ptr, savedElement);
						}
					}
				}

				index += count;
				continue;
			}

			if (!match)
			{
				continue;
			}

			void* ptr = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(data) + field.offset_);

			if (!field.nestedTypeName_.view().empty())
			{
				ApplyFields(field.nestedTypeName_, ptr, match->children_);
				continue;
			}

			ApplyScalar(field, ptr, *match);
		}
	}

	/**
	* [EN]
	* Writes match's saved value into ptr, based on field's
	* AttributeType (the restore-side counterpart of CaptureScalar).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* field の AttributeType に基づき、match の保存済みの値を ptr へ
	* 書き込む（CaptureScalar の復元側に対応する処理）。
	*/
	void BlueprintComponent::ApplyScalar(const FieldInfo& field, void* ptr, const BlueprintField& match)
	{
		/// [EN] Skip a saved value whose type differs from the field's: each value kind has its own member, so reading by the current type would pick one never written.
		///      Leaving the field untouched keeps its default.
		/// [JP] 保存時の型と今のフィールドの型が違う値は適用しない。値の種類ごとに別のメンバがあるので、今の型で読むと書かれていないメンバを拾ってしまう。
		///      触らなければ既定値のまま残る。
		if (match.type_ != field.type_)
		{
			return;
		}

		switch (field.type_)
		{
		case AttributeType::Int:
		case AttributeType::Enum:
			*static_cast<Int*>(ptr) = match.intValue_;
			break;
		case AttributeType::Float:
			*static_cast<Float*>(ptr) = match.floatValue_;
			break;
		case AttributeType::Bool:
			*static_cast<Bool*>(ptr) = match.boolValue_;
			break;
		case AttributeType::Vector2:
			*static_cast<Vector2*>(ptr) = match.vector2Value_;
			break;
		case AttributeType::Vector3:
			*static_cast<Vector3*>(ptr) = match.vector3Value_;
			break;
		case AttributeType::Vector4:
			*static_cast<Vector4*>(ptr) = match.vector4Value_;
			break;
		case AttributeType::String:
			*static_cast<String*>(ptr) = match.stringValue_;
			break;
		case AttributeType::Color:
			*static_cast<Color*>(ptr) = match.colorValue_;
			break;
		default:
			break;
		}
	}

	/**
	* [EN]
	* Recursively captures every reflected field of the type named
	* typeName (rooted at data) into outFields: array fields become
	* one BlueprintField per element, nested-struct fields recurse
	* via their own CaptureFields call, and everything else is
	* captured as a plain scalar.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* typeName という名前の型（data を起点とする）の、リフレクション
	* された全フィールドを outFields へ再帰的に取得する: 配列
	* フィールドは要素ごとに1つの BlueprintField になり、ネストされた
	* 構造体フィールドは自身の CaptureFields 呼び出しを介して再帰し、
	* それ以外は単純なスカラーとして取得される。
	*/
	void BlueprintComponent::CaptureFields(const String& typeName, void* data, DynamicArray<BlueprintField>& outFields)
	{
		DynamicArray<FieldInfo> fields;
		Fields(typeName, data, fields);

		for (Size index = 0; index < fields.size(); ++index)
		{
			const FieldInfo& field = fields[index];

			if (field.array_.size_ > 0 || field.array_.add_)
			{
				/// [EN] An array field: its elements are laid out as the following N FieldInfo entries in the flat list, so consume and skip them here.
				/// [JP] 配列フィールド: その要素はフラットなリスト内で続く N 個の FieldInfo エントリとして並んでいる。ここでそれらを消費し、スキップする。
				Size count = field.array_.size_;

				BlueprintField arrayField;
				arrayField.name_ = field.name_;
				arrayField.isArray_ = true;

				for (Size element = 0; element < count && (index + 1 + element) < fields.size(); ++element)
				{
					const FieldInfo& elementField = fields[index + 1 + element];
					void* ptr = elementField.directPtr_ ? elementField.directPtr_ : (static_cast<Uint8*>(data) + elementField.offset_);

					if (!elementField.nestedTypeName_.view().empty())
					{
						BlueprintField structField;
						CaptureFields(elementField.nestedTypeName_, ptr, structField.children_);
						arrayField.children_.push_back(std::move(structField));
					}
					else
					{
						arrayField.children_.push_back(CaptureScalar(elementField, ptr));
					}
				}

				outFields.push_back(std::move(arrayField));
				index += count;
				continue;
			}

			void* ptr = field.directPtr_ ? field.directPtr_ : (static_cast<Uint8*>(data) + field.offset_);

			if (!field.nestedTypeName_.view().empty())
			{
				/// [EN] Nested struct field: recurse, capturing its own fields as children rather than treating it as a scalar.
				/// [JP] ネストされた構造体フィールド: スカラーとして扱う代わりに、その自身のフィールドを子として再帰的に取得する。
				BlueprintField structField;
				structField.name_ = field.name_;
				CaptureFields(field.nestedTypeName_, ptr, structField.children_);
				outFields.push_back(std::move(structField));
				continue;
			}

			outFields.push_back(CaptureScalar(field, ptr));
		}
	}

	/**
	* [EN]
	* Captures a single non-array, non-nested field's current value
	* from ptr into a new BlueprintField, based on field's AttributeType.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* field の AttributeType に基づき、ptr から単一の非配列・
	* 非ネストフィールドの現在の値を、新しい BlueprintField へ
	* 取得する。
	*/
	BlueprintField BlueprintComponent::CaptureScalar(const FieldInfo& field, void* ptr)
	{
		BlueprintField capturedField;
		capturedField.name_ = field.name_;
		capturedField.type_ = field.type_;

		switch (field.type_)
		{
		case AttributeType::Int:
		case AttributeType::Enum:
			capturedField.intValue_ = *static_cast<Int*>(ptr);
			break;
		case AttributeType::Float:
			capturedField.floatValue_ = *static_cast<Float*>(ptr);
			break;
		case AttributeType::Bool:
			capturedField.boolValue_ = *static_cast<Bool*>(ptr);
			break;
		case AttributeType::Vector2:
			capturedField.vector2Value_ = *static_cast<Vector2*>(ptr);
			break;
		case AttributeType::Vector3:
			capturedField.vector3Value_ = *static_cast<Vector3*>(ptr);
			break;
		case AttributeType::Vector4:
			capturedField.vector4Value_ = *static_cast<Vector4*>(ptr);
			break;
		case AttributeType::String:
			capturedField.stringValue_ = *static_cast<String*>(ptr);
			break;
		case AttributeType::Color:
			capturedField.colorValue_ = *static_cast<Color*>(ptr);
			break;
		default:
			break;
		}

		return capturedField;
	}

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
	void BlueprintComponent::Fields(const String& typeName, void* data, DynamicArray<FieldInfo>& fields)
	{
		auto& reflectionRegistry = ReflectionRegistry::GetRegistry();
		auto reflectionIt = reflectionRegistry.find(typeName);
		if (reflectionIt != reflectionRegistry.end())
		{
			reflectionIt->second(data, fields);
		}

		auto& payloadRegistry = PayloadRegistry::GetRegistry();
		auto payloadIt = payloadRegistry.find(typeName);
		if (payloadIt != payloadRegistry.end())
		{
			payloadIt->second(data, fields);
		}
	}
}

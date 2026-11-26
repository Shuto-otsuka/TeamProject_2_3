#pragma once
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	enum class AttributeType
	{
		Unknown,

		Int,

		Float,

		Bool,

		Vector2,

		Vector3,

		Vector4,

		String,

		Color,

		Enum,

		Struct,
	};

	struct EnumEntry
	{
		Int value_;

		String name_;
	};

	enum class PayloadType
	{
		None,

		Texture,

		Model,

		Effect,

		Audio,

		Font,

		Movie,

		Animation,

		MeshCollision,

		Material,

		Skeleton,

		Sky,

		Prefab,

		Scene,

		Actor,
	};

	struct ArrayInfo
	{
		Size size_ = 0;

		std::function<void()> add_;

		std::function<void(Size)> remove_;

		std::function<void*()> lastPtr_;
	};

	struct EnumInfo
	{
		String typeName_;
	};

	struct FieldInfo
	{
		String name_;

		Size offset_;

		AttributeType type_;

		PayloadType assetType_ = PayloadType::None;

		void* directPtr_ = nullptr;

		ArrayInfo array_;

		EnumInfo enum_;

		std::function<Bool(void*)> enableIf_;

		Float clampMin_ = -FLT_MAX;

		Float clampMax_ = FLT_MAX;

		String nestedTypeName_;

		Bool editorVisible_ = true;

		Bool revision_ = false;
	};

	struct FunctionInfo
	{
		String name_;

		DynamicArray<AttributeType> parameters_;

		AttributeType result_ = AttributeType::Unknown;

		std::function<void(void* component, void* const* arguments, void* result)> invoke_;
	};

	class SEEDCORE_API ReflectionRegistry
	{
	public:
		using ReflectFunc = std::function<void(void*, DynamicArray<FieldInfo>&)>;

		static FlatMap<String, ReflectFunc>& GetRegistry();

		static void Register(String name, ReflectFunc function);
	};

	class SEEDCORE_API EnumRegistry
	{
	public:
		static FlatMap<String, DynamicArray<EnumEntry>>& GetRegistry();

		static void Register(String typeName, DynamicArray<EnumEntry> entries);

		static const DynamicArray<EnumEntry>* GetEntries(const String& typeName);
	};

	class SEEDCORE_API FunctionRegistry
	{
	public:
		static FlatMap<String, DynamicArray<FunctionInfo>>& GetRegistry();

		static void Register(String componentName, FunctionInfo function);
	};

	template<typename T>
	struct AttributeTraits
	{
		SC_CONSTEX AttributeType type = AttributeType::Unknown;
	};

	template<>
	struct AttributeTraits<Int>
	{
		SC_CONSTEX AttributeType type = AttributeType::Int;
	};

	template<>
	struct AttributeTraits<Float>
	{
		SC_CONSTEX AttributeType type = AttributeType::Float;
	};

	template<>
	struct AttributeTraits<Bool>
	{
		SC_CONSTEX AttributeType type = AttributeType::Bool;
	};

	template<>
	struct AttributeTraits<Vector2>
	{
		SC_CONSTEX AttributeType type = AttributeType::Vector2;
	};

	template<>
	struct AttributeTraits<Vector3>
	{
		SC_CONSTEX AttributeType type = AttributeType::Vector3;
	};

	template<>
	struct AttributeTraits<Vector4>
	{
		SC_CONSTEX AttributeType type = AttributeType::Vector4;
	};

	template<>
	struct AttributeTraits<String>
	{
		SC_CONSTEX AttributeType type = AttributeType::String;
	};

	template<>
	struct AttributeTraits<Color>
	{
		SC_CONSTEX AttributeType type = AttributeType::Color;
	};
}

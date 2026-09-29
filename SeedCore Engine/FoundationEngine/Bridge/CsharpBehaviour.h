#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Bridge/ManagedApi.h>

namespace SeedCore
{
	enum class ScriptFieldFlag :Uint8
	{
		None = 0,
		Reflection = 1 << 0,
		Condition = 1 << 1,
		Serialize = 1 << 2,
	};

	struct CsharpField
	{
		String name_;

		AttributeType type_ = AttributeType::Unknown;

		Int32 offset_ = 0;

		Double min_ = 0.0;

		Double max_ = 0.0;

		ScriptFieldFlag flag_ = ScriptFieldFlag::None;

		String enumName_;

		PayloadType assetType_ = PayloadType::None;
	};

	struct CsharpScriptType
	{
		ComponentID id_ = nullptr;

		Int32 typeIndex_ = 0;

		Uint32 hookMask_ = 0;

		Int32 blockSize_ = 0;

		DynamicArray<CsharpField> fields_;

		ManagedApi* managedApi_ = nullptr;
	};

	class SEEDCORE_API CsharpBehaviour :public ComponentBehaviour
	{
	public:
		static constexpr Size blockCapacity_ = 256;

		CsharpBehaviour() = default;

		~CsharpBehaviour()override;

		CsharpBehaviour(const CsharpBehaviour& other);

		CsharpBehaviour(CsharpBehaviour&& other)noexcept;

		CsharpBehaviour& operator=(const CsharpBehaviour&) = delete;
		CsharpBehaviour& operator=(CsharpBehaviour&&) = delete;

		static void Setup(void* component, void* world, Entity entity, ComponentID id);

		static void Reflect(void* component, DynamicArray<FieldInfo>& fields);

	private:
		template<ScriptHook Hook>
		static void Lifecycle(ComponentBehaviour* component);

		template<ScriptHook Hook>
		static void Lifecycle(ComponentBehaviour* component, Float elapsedTime);

		template<ScriptHook Hook>
		static void Contact(ComponentBehaviour* component, Entity other);

		void Synchronize(Bool pull);

	private:
		const CsharpScriptType* scriptType_ = nullptr;

		void* handle_ = nullptr;

		alignas(16) Uint8 currentBlock_[blockCapacity_]{};

		alignas(16) Uint8 syncedBlock_[blockCapacity_]{};
	};
}

#include <FoundationEngine/Bridge/CsharpBehaviour.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	CsharpBehaviour::~CsharpBehaviour()
	{
		if (handle_ != nullptr)
		{
			scriptType_->managedApi_->free_(handle_);
		}
	}

	CsharpBehaviour::CsharpBehaviour(const CsharpBehaviour& other) :ComponentBehaviour(other), scriptType_(other.scriptType_)
	{
		std::memcpy(currentBlock_, other.currentBlock_, blockCapacity_);
		std::memcpy(syncedBlock_, other.currentBlock_, blockCapacity_);

		if (other.handle_ != nullptr)
		{
			handle_ = scriptType_->managedApi_->create_(scriptType_->typeIndex_, entity_.GetID());
			if (handle_ != nullptr)
			{
				scriptType_->managedApi_->push_(handle_, currentBlock_);
			}
		}
	}

	CsharpBehaviour::CsharpBehaviour(CsharpBehaviour&& other)noexcept :ComponentBehaviour(other), scriptType_(other.scriptType_), handle_(other.handle_)
	{
		std::memcpy(currentBlock_, other.currentBlock_, blockCapacity_);
		std::memcpy(syncedBlock_, other.syncedBlock_, blockCapacity_);
		other.handle_ = nullptr;
	}

	void CsharpBehaviour::Setup(void* component, void* world, Entity entity, ComponentID id)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		behaviour->world_ = static_cast<World*>(world);
		behaviour->entity_ = entity;
		behaviour->scriptType_ = static_cast<const CsharpScriptType*>(ComponentRegistry::Get(id).context_);

		const CsharpScriptType* scriptType = behaviour->scriptType_;
		ManagedApi* managedApi = scriptType->managedApi_;
		Bool copied = behaviour->handle_ != nullptr;
		if (copied)
		{
			managedApi->free_(behaviour->handle_);
		}

		behaviour->handle_ = managedApi->create_(scriptType->typeIndex_, entity.GetID());
		if (behaviour->handle_ == nullptr)
		{
			return;
		}

		if (copied)
		{
			managedApi->push_(behaviour->handle_, behaviour->currentBlock_);
		}
		else
		{
			managedApi->pull_(behaviour->handle_, behaviour->currentBlock_);
		}
		std::memcpy(behaviour->syncedBlock_, behaviour->currentBlock_, blockCapacity_);

		auto has = [scriptType](ScriptHook hook)
		{
			return (scriptType->hookMask_ & (1u << static_cast<Uint32>(hook))) != 0;
		};

		if (has(ScriptHook::Awake))
		{
			behaviour->awake_ = &Lifecycle<ScriptHook::Awake>;
		}
		if (has(ScriptHook::Start))
		{
			behaviour->start_ = &Lifecycle<ScriptHook::Start>;
		}
		if (has(ScriptHook::Tick))
		{
			behaviour->tick_ = &Lifecycle<ScriptHook::Tick>;
		}
		if (has(ScriptHook::LateTick))
		{
			behaviour->lateTick_ = &Lifecycle<ScriptHook::LateTick>;
		}
		if (has(ScriptHook::FixedTick))
		{
			behaviour->fixedTick_ = &Lifecycle<ScriptHook::FixedTick>;
		}
		if (has(ScriptHook::EditorTick))
		{
			behaviour->editorTick_ = &Lifecycle<ScriptHook::EditorTick>;
		}
		if (has(ScriptHook::Destroy))
		{
			behaviour->destroy_ = &Lifecycle<ScriptHook::Destroy>;
		}
		if (has(ScriptHook::InspectorGUI))
		{
			behaviour->inspectorGUI_ = &Lifecycle<ScriptHook::InspectorGUI>;
		}
		if (has(ScriptHook::CollisionEnter))
		{
			behaviour->collisionEnter_ = &Contact<ScriptHook::CollisionEnter>;
		}
		if (has(ScriptHook::CollisionStay))
		{
			behaviour->collisionStay_ = &Contact<ScriptHook::CollisionStay>;
		}
		if (has(ScriptHook::CollisionExit))
		{
			behaviour->collisionExit_ = &Contact<ScriptHook::CollisionExit>;
		}
		if (has(ScriptHook::TriggerEnter))
		{
			behaviour->triggerEnter_ = &Contact<ScriptHook::TriggerEnter>;
		}
		if (has(ScriptHook::TriggerStay))
		{
			behaviour->triggerStay_ = &Contact<ScriptHook::TriggerStay>;
		}
		if (has(ScriptHook::TriggerExit))
		{
			behaviour->triggerExit_ = &Contact<ScriptHook::TriggerExit>;
		}
	}

	void CsharpBehaviour::Reflect(void* component, DynamicArray<FieldInfo>& fields)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		if (behaviour->scriptType_ == nullptr)
		{
			return;
		}

		behaviour->Synchronize(true);

		auto has = [](const CsharpField& field, ScriptFieldFlag flag)
			{
				return (static_cast<Uint8>(field.flag_) & static_cast<Uint8>(flag)) != 0;
			};

		Size blockOffset = static_cast<Size>(reinterpret_cast<Uint8*>(behaviour->currentBlock_) - reinterpret_cast<Uint8*>(behaviour));
		const DynamicArray<CsharpField>& scriptFields = behaviour->scriptType_->fields_;
		for (Size fieldIndex = 0; fieldIndex < scriptFields.size(); ++fieldIndex)
		{
			const CsharpField& scriptField = scriptFields[fieldIndex];

			FieldInfo field;
			field.name_ = scriptField.name_;
			field.offset_ = blockOffset + static_cast<Size>(scriptField.offset_);
			field.type_ = scriptField.type_;
			field.enum_.typeName_ = scriptField.enumName_;
			field.assetType_ = scriptField.assetType_;
			if (std::isfinite(scriptField.min_))
			{
				field.clampMin_ = static_cast<Float>(scriptField.min_);
			}
			if (std::isfinite(scriptField.max_))
			{
				field.clampMax_ = static_cast<Float>(scriptField.max_);
			}
			field.editorVisible_ = has(scriptField, ScriptFieldFlag::Reflection);

			if (has(scriptField, ScriptFieldFlag::Condition))
			{
				Int32 conditionIndex = static_cast<Int32>(fieldIndex);
				field.enableIf_ = [conditionIndex](void* data) -> Bool
				{
					CsharpBehaviour* owner = static_cast<CsharpBehaviour*>(data);
					if (owner->handle_ == nullptr)
					{
						return true;
					}
					return owner->scriptType_->managedApi_->condition_(owner->handle_, conditionIndex) != 0;
				};
			}

			fields.push_back(std::move(field));
		}
	}

	template<ScriptHook Hook>
	void CsharpBehaviour::Lifecycle(ComponentBehaviour* component)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		if (behaviour->handle_ == nullptr)
		{
			return;
		}
		behaviour->Synchronize(false);
		behaviour->scriptType_->managedApi_->invokeLifecycle_(behaviour->handle_, Hook, 0.0f);
	}

	template<ScriptHook Hook>
	void CsharpBehaviour::Lifecycle(ComponentBehaviour* component, Float elapsedTime)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		if (behaviour->handle_ == nullptr)
		{
			return;
		}
		behaviour->Synchronize(false);
		behaviour->scriptType_->managedApi_->invokeLifecycle_(behaviour->handle_, Hook, elapsedTime);
	}

	template<ScriptHook Hook>
	void CsharpBehaviour::Contact(ComponentBehaviour* component, Entity other)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		if (behaviour->handle_ == nullptr)
		{
			return;
		}
		behaviour->Synchronize(false);
		behaviour->scriptType_->managedApi_->invokeContact_(behaviour->handle_, Hook, other.GetID());
	}

	void CsharpBehaviour::Synchronize(Bool pull)
	{
		if (handle_ == nullptr)
		{
			return;
		}

		ManagedApi* managedApi = scriptType_->managedApi_;
		Size blockSize = static_cast<Size>(scriptType_->blockSize_);
		if (std::memcmp(currentBlock_, syncedBlock_, blockSize) != 0)
		{
			managedApi->push_(handle_, currentBlock_);
		}

		if (pull)
		{
			managedApi->pull_(handle_, currentBlock_);
		}
		std::memcpy(syncedBlock_, currentBlock_, blockSize);
	}
}

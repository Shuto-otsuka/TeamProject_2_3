#include <FoundationEngine/Bridge/CsharpBehaviour.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

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

	void CsharpBehaviour::Bind(CsharpScriptType& scriptType)
	{
		auto has = [&scriptType](ScriptHook hook)
			{
				return (scriptType.hookMask_ & (1u << static_cast<Uint32>(hook))) != 0;
			};

		scriptType.upcast_ = [](void* component) -> ComponentBehaviour* { return static_cast<CsharpBehaviour*>(component); };
		scriptType.attach_ = &Attach;

		if (has(ScriptHook::Detach))
		{
			scriptType.detach_ = &Lifecycle<ScriptHook::Detach>;
		}
		if (has(ScriptHook::Awake))
		{
			scriptType.awake_ = &Lifecycle<ScriptHook::Awake>;
		}
		if (has(ScriptHook::Start))
		{
			scriptType.start_ = &Lifecycle<ScriptHook::Start>;
		}
		if (has(ScriptHook::Tick))
		{
			scriptType.tick_ = &Lifecycle<ScriptHook::Tick>;
		}
		if (has(ScriptHook::LateTick))
		{
			scriptType.lateTick_ = &Lifecycle<ScriptHook::LateTick>;
		}
		if (has(ScriptHook::FixedTick))
		{
			scriptType.fixedTick_ = &Lifecycle<ScriptHook::FixedTick>;
		}
		if (has(ScriptHook::EditorTick))
		{
			scriptType.editorTick_ = &Lifecycle<ScriptHook::EditorTick>;
		}
		if (has(ScriptHook::Destroy))
		{
			scriptType.destroy_ = &Lifecycle<ScriptHook::Destroy>;
		}
		if (has(ScriptHook::InspectorGUI))
		{
			scriptType.inspectorGUI_ = &Lifecycle<ScriptHook::InspectorGUI>;
		}
		if (has(ScriptHook::CollisionEnter))
		{
			scriptType.collisionEnter_ = &Contact<ScriptHook::CollisionEnter>;
		}
		if (has(ScriptHook::CollisionStay))
		{
			scriptType.collisionStay_ = &Contact<ScriptHook::CollisionStay>;
		}
		if (has(ScriptHook::CollisionExit))
		{
			scriptType.collisionExit_ = &Contact<ScriptHook::CollisionExit>;
		}
		if (has(ScriptHook::TriggerEnter))
		{
			scriptType.triggerEnter_ = &Contact<ScriptHook::TriggerEnter>;
		}
		if (has(ScriptHook::TriggerStay))
		{
			scriptType.triggerStay_ = &Contact<ScriptHook::TriggerStay>;
		}
		if (has(ScriptHook::TriggerExit))
		{
			scriptType.triggerExit_ = &Contact<ScriptHook::TriggerExit>;
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

	void CsharpBehaviour::Attach(ComponentBehaviour* component)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		behaviour->scriptType_ = static_cast<const CsharpScriptType*>(behaviour->id_->hooks_);

		const CsharpScriptType* scriptType = behaviour->scriptType_;
		behaviour->handle_ = scriptType->managedApi_->create_(scriptType->typeIndex_, behaviour->entity_.GetID());
		if (behaviour->handle_ == nullptr)
		{
			return;
		}

		scriptType->managedApi_->pull_(behaviour->handle_, behaviour->currentBlock_);
		std::memcpy(behaviour->syncedBlock_, behaviour->currentBlock_, blockCapacity_);

		if ((scriptType->hookMask_ & (1u << static_cast<Uint32>(ScriptHook::Attach))) != 0)
		{
			Lifecycle<ScriptHook::Attach>(behaviour);
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
	void CsharpBehaviour::Contact(ComponentBehaviour* component, const Actor& other)
	{
		CsharpBehaviour* behaviour = static_cast<CsharpBehaviour*>(component);
		if (behaviour->handle_ == nullptr)
		{
			return;
		}
		behaviour->Synchronize(false);
		behaviour->scriptType_->managedApi_->invokeContact_(behaviour->handle_, Hook, ActorAccessor::GetEntity(other).GetID());
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

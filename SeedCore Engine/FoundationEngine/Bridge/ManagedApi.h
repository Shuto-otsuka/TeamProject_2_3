#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	enum class ScriptHook :Uint8
	{
		Awake,
		Start,
		Tick,
		LateTick,
		FixedTick,
		EditorTick,
		Destroy,
		InspectorGUI,
		CollisionEnter,
		CollisionStay,
		CollisionExit,
		TriggerEnter,
		TriggerStay,
		TriggerExit,
	};

	struct ManagedApi
	{
		Int32(*load_)(const Uint8* path, Int32 length) = nullptr;

		Int32(*unload_)() = nullptr;

		void* (*create_)(Int32 typeIndex, EntityID entity) = nullptr;

		void (*invokeLifecycle_)(void* handle, ScriptHook hook, Float deltaTime) = nullptr;

		void (*invokeContact_)(void* handle, ScriptHook hook, EntityID other) = nullptr;

		void (*push_)(void* handle, Uint8* block) = nullptr;

		void (*pull_)(void* handle, Uint8* block) = nullptr;

		Uint8 (*condition_)(void* handle, Int32 fieldIndex) = nullptr;

		void (*free_)(void* handle) = nullptr;
	};
}

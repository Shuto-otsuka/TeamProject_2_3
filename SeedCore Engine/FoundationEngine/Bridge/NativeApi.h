#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct NativeApi
	{
		void* context_ = nullptr;

		void (*log_)(Uint8 level, const Uint8* text, Int32 length) = nullptr;

		void (*registerScript_)(void* context, const Uint8* name, Int32 nameLength, const Uint8* category, Int32 categoryLength, Int32 typeIndex, Uint32 hookMask, Int32 blockSize) = nullptr;

		void (*registerField_)(void* context, Int32 typeIndex, const Uint8* name, Int32 nameLength, Uint8 type, Int32 offset, Double min, Double max, Uint8 flag, const Uint8* enumName, Int32 enumNameLength, Uint8 assetType) = nullptr;

		void (*registerEnum_)(void* context, const Uint8* enumName, Int32 enumNameLength, Int32 value, const Uint8* entryName, Int32 entryNameLength) = nullptr;

		void (*intern_)(const Uint8* text, Int32 length, Uint8* destination) = nullptr;

		void* (*field_)(void* context, EntityID entity, const Uint8* component, Int32 componentLength, const Uint8* field, Int32 fieldLength, Uint8 type) = nullptr;
	};
}

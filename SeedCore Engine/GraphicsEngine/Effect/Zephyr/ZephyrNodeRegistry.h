#pragma once
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>

namespace SeedCore
{
	struct ZephyrNodeInput
	{
		String name_;

		ZephyrType type_ = ZephyrType::Float;

		Vector4 default_ = Vector4::Zero;

		Bool pinned_ = true;

		Bool multiple_ = false;
	};

	struct ZephyrNodeOutput
	{
		String name_;

		ZephyrType type_ = ZephyrType::Float;
	};

	struct ZephyrNodeDefinition
	{
		String type_;

		String label_;

		String category_;

		String include_;

		Bool removable_ = true;

		DynamicArray<ZephyrNodeInput> inputs_;

		DynamicArray<ZephyrNodeOutput> outputs_;
	};

	class SEEDCORE_API ZephyrNodeRegistry
	{
	public:
		NODISCARD static const DynamicArray<ZephyrNodeDefinition>& Nodes();

		NODISCARD static const ZephyrNodeDefinition* Find(const String& type);

		NODISCARD static Bool Connectable(ZephyrType from, ZephyrType to);
	};
}
#pragma once

namespace SeedCore
{
	enum class ZephyrType :Uint8
	{
		Float,
		Float2,
		Float3,
		Float4,
		Color,
		Int,
		Bool,
		Numeric,
		Force,
		Position,
		Shape,
		Renderer,
		Effekseer,
	};

	struct ZephyrModuleInput
	{
		String name_;

		ZephyrType type_ = ZephyrType::Float;
	};

	struct ZephyrModuleDescriptor
	{
		String name_;

		String include_;

		ZephyrType output_ = ZephyrType::Force;

		DynamicArray<ZephyrModuleInput> inputs_;
	};

	class ZephyrModuleRegistry
	{
	public:
		NODISCARD static const DynamicArray<ZephyrModuleDescriptor>& Modules();

		NODISCARD static const ZephyrModuleDescriptor* Find(const String& name);
	};
}

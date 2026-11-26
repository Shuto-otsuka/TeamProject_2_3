#pragma once

namespace SeedCore
{
	class SEEDCORE_API LayerRegistry
	{
	public:
		SC_CONSTEX Size LayerCount = 16;

		SC_CONSTEX Size DefaultLayer = 0;

	public:
		static void Name(Size index, String name);

		static const String& Name(Size index);

		static Size Find(const String& name);

		static std::span<const String> Names();

		static void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/LayerBindings.scg");

		static void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/LayerBindings.scg");

	private:
		static DynamicArray<String>& Storage();
	};
}

#pragma once

namespace SeedCore
{
	struct SEEDCORE_API IconConfig
	{
		DynamicArray<Byte> icon_;

		void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/IconBindings.scg");

		void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/IconBindings.scg")const;

		Bool Import(const std::filesystem::path& imagePath);
	};
}

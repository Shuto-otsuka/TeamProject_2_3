#pragma once

namespace SeedCore
{
	class SEEDCORE_API FileLocator
	{
	public:
		NODISCARD static std::filesystem::path EngineRoot();

		NODISCARD static std::filesystem::path ProjectRoot();
	};
}
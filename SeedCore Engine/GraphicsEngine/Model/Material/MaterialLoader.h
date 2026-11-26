#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	class SEEDCORE_API MaterialLoader :public AssetLoader<BakedSurface>
	{
	private:
		friend class MaterialResource;

	public:
		MaterialLoader() = default;
		~MaterialLoader() = default;

		Handle<BakedSurface> Load(String filePath, LoadThread thread);

		void Clear(Handle<BakedSurface>& handle)noexcept;

	private:
		Bool Bake(const BakedSurface& bakedSurface, String filePath);
	};
}

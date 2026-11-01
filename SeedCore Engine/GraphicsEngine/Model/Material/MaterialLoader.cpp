#include <GraphicsEngine/Model/Material/MaterialLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	Handle<BakedSurface> MaterialLoader::Load(String filePath)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".material" || !std::filesystem::exists(path))
		{
			return Handle<BakedSurface>::null();
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			return Handle<BakedSurface>::null();
		}

		Handle<BakedSurface> handle = pool_.Create();
		BakedSurface* bakedSurface = pool_.Get(handle);
		if (!bakedSurface)
		{
			return Handle<BakedSurface>::null();
		}
		bakedSurface->Serialize(archive);

		return handle;
	}

	BakedSurface* MaterialLoader::Get(const Handle<BakedSurface>& handle)
	{
		return pool_.Get(handle);
	}

	void MaterialLoader::Clear(Handle<BakedSurface>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	Bool MaterialLoader::Save(const BakedSurface& bakedSurface, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<BakedSurface&>(bakedSurface).Serialize(archive);
		return archive.Write(filePath);
	}
}

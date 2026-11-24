#include <GraphicsEngine/Effect/EffectLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	Handle<EffectAsset> EffectLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".zephyr" || !std::filesystem::exists(path))
		{
			return Handle<EffectAsset>::null();
		}

		auto create = [this]()
			{
				Handle<EffectAsset> handle = pool_.Create();
				if (!pool_.Get(handle))
				{
					return Handle<EffectAsset>::null();
				}
				return handle;
			};

		std::error_code error;
		if (std::filesystem::file_size(path, error) == 0 && !error)
		{
			Handle<EffectAsset> handle = create();
			if (EffectAsset* effect = pool_.Get(handle))
			{
				effect->emitters_.push_back(EffectEmitter::Create(String("Emitter")));
			}
			return handle;
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			return Handle<EffectAsset>::null();
		}

		Handle<EffectAsset> handle = create();
		if (EffectAsset* effect = pool_.Get(handle))
		{
			effect->Serialize(archive);
		}
		return handle;
	}

	void EffectLoader::Clear(Handle<EffectAsset>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	Bool EffectLoader::Bake(const EffectAsset& effect, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<EffectAsset&>(effect).Serialize(archive);
		return archive.Write(filePath);
	}
}
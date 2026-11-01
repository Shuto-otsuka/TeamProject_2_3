#pragma once
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>

namespace SeedCore
{
	struct BakedSurface
	{
		Surface surface_;

		DynamicArray<Uint32> textureAssetIDs_;

		DynamicArray<Bitmap> textures_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			surface_.Serialize(archive);
			archive.TryField("baked_texture_asset_ids", textureAssetIDs_);
			archive.TryField("baked_textures", textures_);
		}
	};
}
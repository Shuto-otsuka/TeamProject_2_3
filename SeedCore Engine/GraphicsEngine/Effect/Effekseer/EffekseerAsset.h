#pragma once
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>

namespace SeedCore
{
	struct EffekseerTexture
	{
		Bitmap bitmap_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("bitmap", bitmap_);
		}
	};

	struct EffekseerModel
	{
		DynamicArray<Byte> data_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("data", data_);
		}
	};

	struct EffekseerMaterial
	{
		DynamicArray<Byte> data_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("data", data_);
		}
	};

	struct EffekseerCurve
	{
		DynamicArray<Byte> data_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("data", data_);
		}
	};

	struct EffekseerAsset
	{
		DynamicArray<Byte> data_;

		DynamicArray<EffekseerTexture> colorTextures_;

		DynamicArray<EffekseerTexture> normalTextures_;

		DynamicArray<EffekseerTexture> distortionTextures_;

		DynamicArray<EffekseerModel> models_;

		DynamicArray<EffekseerMaterial> materials_;

		DynamicArray<EffekseerCurve> curves_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("data", data_);
			archive.Field("color_textures", colorTextures_);
			archive.Field("normal_textures", normalTextures_);
			archive.Field("distortion_textures", distortionTextures_);
			archive.Field("models", models_);
			archive.Field("materials", materials_);
			archive.Field("curves", curves_);
		}
	};
}

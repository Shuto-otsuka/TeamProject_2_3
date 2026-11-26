#pragma once

namespace SeedCore
{
	struct SkymapAsset
	{
		Uint32 format_ = 0;

		Uint32 width_ = 0;

		Uint32 height_ = 0;

		Uint32 rowPitch_ = 0;

		DynamicArray<Uint8> pixels_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("format", format_);
			archive.Field("width", width_);
			archive.Field("height", height_);
			archive.Field("rowPitch", rowPitch_);
			archive.Field("pixels", pixels_);
		}
	};
}

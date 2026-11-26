#pragma once

namespace SeedCore
{
	enum class BootAnchor
	{
		TopLeft,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight,
	};

	enum class BootFillMethod
	{
		Horizontal,
		Vertical,
		Radial90,
		Radial180,
		Radial360,
	};

	struct SEEDCORE_API BootConfig
	{
		DynamicArray<Byte> backgroundImage_;

		DynamicArray<Byte> barImage_;

		DynamicArray<Byte> frameImage_;

		Bool useBackgroundImage_ = true;

		Bool useFrame_ = true;

		Color backgroundColor_ = Color(0.0f, 0.0f, 0.0f, 1.0f);

		Color backgroundTint_ = Color(1.0f, 1.0f, 1.0f, 1.0f);

		Color barTint_ = Color(1.0f, 1.0f, 1.0f, 1.0f);

		Color frameTint_ = Color(1.0f, 1.0f, 1.0f, 1.0f);

		BootAnchor anchor_ = BootAnchor::BottomRight;

		Vector2 offset_ = Vector2(-0.028125f, -0.05f);

		Float width_ = 0.4f;

		BootFillMethod fillMethod_ = BootFillMethod::Horizontal;

		Int32 fillOrigin_ = 0;

		Bool clockwise_ = true;

		Bool wave_ = true;

		void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/BootConfig.scg");

		void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/BootConfig.scg")const;

		static Bool Import(const std::filesystem::path& imagePath, DynamicArray<Byte>& image);
	};
}

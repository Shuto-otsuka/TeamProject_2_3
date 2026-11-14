#pragma once

namespace SeedCore
{
	class AvatarMesh;

	enum class PreviewSlot :Uint8
	{
		Timeline,
		ModelTransform,
		Material,
		Skeleton,
		Avatar,
		Boot,
	};

	inline constexpr Uint32 PreviewSlotCount = 6;

	struct PreviewRequest
	{
		Uint32 meshAssetID_ = 0;

		Uint32 surfaceAssetID_ = 0;

		Uint32 animationAssetID_ = 0;

		Float time_ = 0.0f;

		Matrix world_ = Matrix::Identity;

		Bool axes_ = false;

		Bool bones_ = false;

		Int joint_ = -1;

		const AvatarMesh* avatar_ = nullptr;

		StaticArray<Uint32, 4> regionTextures_ = { SC_INVALID, SC_INVALID, SC_INVALID, SC_INVALID };

		Uint32 regionCount_ = 0;
	};
}

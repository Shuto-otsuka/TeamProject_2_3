#pragma once

namespace SeedCore
{
	enum class SignedAxis :Uint32
	{
		PositiveX = 0,
		NegativeX = 1,
		PositiveY = 2,
		NegativeY = 3,
		PositiveZ = 4,
		NegativeZ = 5,
	};

	enum class WindingOverride :Uint32
	{
		Auto = 0,
		AsIs = 1,
		Flip = 2,
	};

	struct SEEDCORE_API AxisConvention
	{
		SignedAxis up_ = SignedAxis::PositiveY;
		SignedAxis right_ = SignedAxis::NegativeX;
		SignedAxis forward_ = SignedAxis::PositiveZ;
		WindingOverride windingOverride_ = WindingOverride::Auto;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("up", up_);
			archive.Field("right", right_);
			archive.Field("forward", forward_);
			archive.Field("winding_override", windingOverride_);
		}
	};

	struct SEEDCORE_API ResolvedAxisConvention
	{
		Matrix basis_ = Matrix::Identity;

		Bool valid_ = false;

		Bool isMirror_ = false;

		Bool isRightHanded_ = false;

		Bool flipWinding_ = false;

		NODISCARD static ResolvedAxisConvention Resolve(const AxisConvention& convention);
	};
}

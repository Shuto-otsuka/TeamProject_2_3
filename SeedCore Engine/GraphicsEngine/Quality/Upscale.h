#pragma once

namespace SeedCore
{
	enum class UpscaleMode : Int32
	{
		MaxPerformance = 0,
		Balanced = 1,
		MaxQuality = 2,
		UltraPerformance = 3,
		Dlaa = 4,
	};

	enum class UpscaleMethod : Int32
	{
		Taau = 0,
		DlssSuperResolution = 1,
		DlssRayReconstruction = 2,
	};

	struct GraphicsUpscale
	{
		UpscaleMethod method_ = UpscaleMethod::Taau;

		UpscaleMode mode_ = UpscaleMode::MaxQuality;
	};

	inline Float UpscaleRenderScale(UpscaleMode mode)
	{
		switch (mode)
		{
		case UpscaleMode::MaxPerformance:
			return 1.0f / 2.0f;
		case UpscaleMode::Balanced:
			return 1.0f / 1.7f;
		case UpscaleMode::MaxQuality:
			return 1.0f / 1.5f;
		case UpscaleMode::UltraPerformance:
			return 1.0f / 3.0f;
		case UpscaleMode::Dlaa:
			return 1.0f;
		}
		return 1.0f;
	}
}

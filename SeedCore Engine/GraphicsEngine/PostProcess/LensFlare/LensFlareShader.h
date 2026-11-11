#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct LensFlareConstantBuffer
	{
		Float threshold_ = 0.0f;

		Float intensity_ = 0.0f;

		Float streakLength_ = 0.0f;

		Float streakAttenuation_ = 0.0f;

		Float chromaticAberration_ = 0.0f;

		Float angleOffset_ = 0.0f;

		Uint32 ghostCount_ = 0;

		Float ghostDispersal_ = 0.0f;

		Float ghostIntensity_ = 0.0f;

		Float haloWidth_ = 0.0f;

		Uint32 axisCount_ = 0;

		Float spikeVariation_ = 0.0f;
	};
	SC_STATIC_ASSERT(LensFlareConstantBuffer, 48, "PostProcess/LensFlare/LensFlareCS.hlsl");

	struct LensFlareDownsampleDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Downsample";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareBlurPass1Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "BlurPass1";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareBlurPass2Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "BlurPass2";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareBlurPass3Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "BlurPass3";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareBlurPass4Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "BlurPass4";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareComposeDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Compose";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct LensFlareGhostDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/LensFlare/LensFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Ghost";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}

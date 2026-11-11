#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct AnamorphicFlareConstantBuffer
	{
		Float threshold_ = 0.0f;

		Float intensity_ = 0.0f;

		Float streakLength_ = 0.0f;

		Float attenuation_ = 0.0f;

		StaticArray<Float, 4> tint_ = { 0.0f, 0.0f, 0.0f, 0.0f };
	};
	SC_STATIC_ASSERT(AnamorphicFlareConstantBuffer, 32, "PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

	struct AnamorphicFlarePrefilterDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Prefilter";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct AnamorphicFlareBlur1Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Blur1";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct AnamorphicFlareBlur2Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Blur2";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct AnamorphicFlareBlur3Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Blur3";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct AnamorphicFlareBlur4Description
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Blur4";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};

	struct AnamorphicFlareComposeDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Compose";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}

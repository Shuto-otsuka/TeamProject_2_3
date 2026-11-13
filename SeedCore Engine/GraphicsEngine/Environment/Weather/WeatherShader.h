#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct WeatherConstantBuffer
	{
		Float wetness_ = 0.0f;

		Float snowCoverage_ = 0.0f;

		Float thunderFlash_ = 0.0f;

		Float thunderSeed_ = 0.0f;

		Float snowIntensity_ = 0.0f;

		Float weatherConstantBufferPadding0_ = 0.0f;

		Float weatherConstantBufferPadding1_ = 0.0f;

		Float weatherConstantBufferPadding2_ = 0.0f;
	};
	SC_STATIC_ASSERT(WeatherConstantBuffer, 32, "Environment/Weather/Weather.hlsli");

	struct WeatherParticleStructuredBuffer
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		Float life_ = 0.0f;

		Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };

		Float seed_ = 0.0f;
	};
	SC_STATIC_ASSERT(WeatherParticleStructuredBuffer, 32, "Environment/Weather/Weather.hlsli");

	struct WeatherEmitterConstantBuffer
	{
		Vector3 wind_ = { 0.0f, 0.0f, 0.0f };

		Uint rainCapacity_ = 0;

		Uint rainCount_ = 0;

		Float rainFallSpeed_ = 0.0f;

		Float rainSize_ = 0.0f;

		Float rainStreakLength_ = 0.0f;

		Float rainBrightness_ = 0.0f;

		Float rainVolumeRadius_ = 0.0f;

		Float rainVolumeHeight_ = 0.0f;

		Uint snowCount_ = 0;

		Vector3 rainColor_ = { 0.0f, 0.0f, 0.0f };

		Float snowFallSpeed_ = 0.0f;

		Float snowSwayAmount_ = 0.0f;

		Float snowSize_ = 0.0f;

		Float snowBrightness_ = 0.0f;

		Float snowVolumeRadius_ = 0.0f;

		Vector3 snowColor_ = { 0.0f, 0.0f, 0.0f };

		Float snowVolumeHeight_ = 0.0f;
	};
	SC_STATIC_ASSERT(WeatherEmitterConstantBuffer, 96, "Environment/Weather/Weather.hlsli");

	struct WeatherUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Environment/Weather/WeatherCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct WeatherDrawDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Environment/Weather/WeatherMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Environment/Weather/WeatherVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Environment/Weather/WeatherPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 32;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 6;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Additive;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}

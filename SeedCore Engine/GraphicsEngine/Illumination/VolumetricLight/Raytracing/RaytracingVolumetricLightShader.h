#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct RaytracingVolumetricLightConstantBuffer
	{
		Float density_ = 0.003f;

		Float absorption_ = 0.0005f;

		Float heightFalloff_ = 0.05f;

		Float heightReference_ = 0.0f;

		Vector3 fogAlbedo_ = { 1.0f, 1.0f, 1.0f };

		Float scatteringG_ = 0.7f;

		Vector3 jitter_ = { 0.0f, 0.0f, 0.0f };

		Float rayTMax_ = 2000.0f;

		Float godrayStrength_ = 1.0f;

		Uint32 frame_ = 0;

		Uint32 historyValid_ = 0;

		Uint32 raytracingVolumetricLightConstantBufferPadding0_ = 0;
	};
	SC_STATIC_ASSERT(RaytracingVolumetricLightConstantBuffer, 64, "Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLight.hlsli");

	struct RaytracingVolumetricLightInjectDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightInjectCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeZ_ = 4;
	};

	struct RaytracingVolumetricLightScatterDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightScatterCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeZ_ = 4;
	};

	struct RaytracingVolumetricLightIntegrateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightIntegrateCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "main";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}

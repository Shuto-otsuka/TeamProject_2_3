#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct VolumetricCloudConstantBuffer
	{
		Float cloudBottom_ = 500.0f;

		Float cloudTop_ = 1500.0f;

		Float coverage_ = 0.5f;

		Float densityScale_ = 0.02f;

		Vector3 cloudAlbedo_ = { 1.0f, 1.0f, 1.0f };

		Float scatteringG_ = 0.6f;

		Uint stepCount_ = 64;

		Uint lightStepCount_ = 6;

		Float noiseScale_ = 0.0008f;

		Float windSpeed_ = 0.01f;

		Uint frameIndex_ = 0;

		Float cloudType_ = 1.0f;

		Float rain_ = 0.0f;

		Float detailScale_ = 8.0f;

		Vector3 skyZenithColor_ = { 0.15f, 0.35f, 0.75f };

		Float sunSize_ = 0.03f;

		Vector3 skyHorizonColor_ = { 0.65f, 0.75f, 0.9f };

		Float skyBrightness_ = 1.0f;

		Vector3 groundColor_ = { 0.25f, 0.22f, 0.2f };

		Uint proceduralSkyEnabled_ = 0;

		Float planetRadius_ = 6360000.0f;

		Float maxMarchDistance_ = 120000.0f;

		Float lodDistance_ = 25000.0f;

		Float aerialDensity_ = 0.000012f;

		Float weatherScale_ = 0.06f;

		Float weatherAmount_ = 0.6f;

		Float detailStrength_ = 0.35f;

		Float detailWindSpeed_ = 0.3f;

		Float phaseBackward_ = 0.35f;

		Float phaseBlend_ = 0.4f;

		Float powderStrength_ = 1.0f;

		Float ambientStrength_ = 0.4f;

		Uint multiScatterOctaves_ = 5;

		Float multiScatterAttenuation_ = 0.5f;

		Float multiScatterContribution_ = 0.75f;

		Float multiScatterEccentricity_ = 0.5f;

		Float temporalJitter_ = 0.0f;

		Float iblIntensity_ = 1.0f;

		Float volumetricCloudConstantBufferPadding0_ = 0.0f;

		Float volumetricCloudConstantBufferPadding1_ = 0.0f;
	};
	SC_STATIC_ASSERT(VolumetricCloudConstantBuffer, 192, "Environment/VolumetricCloud/VolumetricCloud.hlsli");

	struct VolumetricCloudShapeDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Shape";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeZ_ = 4;
	};

	struct VolumetricCloudDetailDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Detail";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 4;

		SC_CONSTEX Uint32 threadGroupSizeZ_ = 4;
	};

	struct VolumetricCloudTraceDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Trace";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 8;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 8;
	};
}

#pragma once
#include <GraphicsEngine/System/SceneConstantSystem.h>

namespace SeedCore
{
	enum class RenderViewKind :Uint8
	{
		Editor,
		Game,
		Canvas,
	};

	inline constexpr Uint32 RenderViewCount = 3;

	enum class RenderViewMask :Uint8
	{
		Frame = 0,
		Editor = 1 << 0,
		Game = 1 << 1,
		Canvas = 1 << 2,
		Scene = Editor | Game,
		All = Editor | Game | Canvas,
	};

	enum class RenderViewOutput :Uint8
	{
		Texture,
		Screen,
	};

	enum class RenderViewMode :Uint32
	{
		Lit = 0,
		Unlit,
		Wireframe,
		DetailLighting,
		LightingOnly,
		Reflections,
		BufferBaseColor,
		BufferNormal,
		BufferRoughness,
		BufferMetallic,
		BufferEmissive,
		BufferDepth,
		BufferVelocity,
		BufferPosition,
		BufferMaterial,
		BufferOverview,
		MeshletId,
		MeshletTriangle,
		MeshletCluster,
		MeshletInstance,
		MeshletLevel,
		MeshletError,
		MeshletCulling,
		MeshletBounds,
		MeshletOverdraw,
		DirectDiffuse,
		DirectSpecular,
		IndirectDiffuse,
		IndirectSpecular,
		LightComplexity,
		LightCluster,
		RayDistance,
		RayHit,
		RayDirection,
		RayCount,
		RayInstance,
		RayGeometry,
		ReservoirCount,
		ReservoirWeight,
		ReservoirAge,
		ReservoirRadiance,
		ReservoirTemporal,
		ReservoirSpatial,
		ReservoirRejection,
		DenoiserRaw,
		DenoiserDenoised,
		DenoiserVariance,
		DenoiserLength,
		DenoiserValidity,
		DenoiserReprojection,
		ShaderComplexity,
		QuadOverdraw,
		TriangleDensity,
		MeshletDensity,
		TextureMip,
		LodColoration,
		PostColor,
		PostLuminance,
		PostBloom,
		PostExposure,
		PostTonemap,
		UpscaleInput,
		UpscaleOutput,
	};

	enum class RenderDebugSource :Uint32
	{
		Reflection = 0,
		GlobalIllumination,
		Shadow,
		AmbientOcclusion,
	};

	enum class RenderShowFlag :Uint64
	{
		None = 0,
		Model = 1ull << 0,
		Primitive = 1ull << 1,
		Transparent = 1ull << 2,
		Fur = 1ull << 3,
		Sky = 1ull << 4,
		VolumetricCloud = 1ull << 5,
		VolumetricStar = 1ull << 6,
		Weather = 1ull << 7,
		VolumetricLight = 1ull << 8,
		DirectLight = 1ull << 9,
		Shadow = 1ull << 10,
		Reflection = 1ull << 11,
		GlobalIllumination = 1ull << 12,
		AmbientOcclusion = 1ull << 13,
		SubsurfaceScattering = 1ull << 14,
		Refraction = 1ull << 15,
		PostProcess = 1ull << 16,
		Bloom = 1ull << 17,
		LensFlare = 1ull << 18,
		AnamorphicFlare = 1ull << 19,
		DepthOfField = 1ull << 20,
		Bokeh = 1ull << 21,
		ChromaticAberration = 1ull << 22,
		Vignette = 1ull << 23,
		FilmGrain = 1ull << 24,
		LensDistortion = 1ull << 25,
		ColorGrading = 1ull << 26,
		ToneMapping = 1ull << 27,
		AutoExposure = 1ull << 28,
		Sharpness = 1ull << 29,
		Outline = 1ull << 30,
		All = ~0ull,
	};

	struct RenderView
	{
		RenderViewKind kind_ = RenderViewKind::Game;

		RenderViewMode mode_ = RenderViewMode::Lit;

		RenderDebugSource debugSource_ = RenderDebugSource::Reflection;

		Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::All);

		RenderViewOutput output_ = RenderViewOutput::Texture;

		SceneConstantBuffer scene_;

		Uint32 renderWidth_ = 0;

		Uint32 renderHeight_ = 0;

		Uint32 displayWidth_ = 0;

		Uint32 displayHeight_ = 0;

		Vector4 outputRect_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		Bool hasActiveCamera_ = false;
	};

	struct ViewBindings
	{
		Uint32 scene_ = 0;

		Uint32 frame_ = 0;

		Uint32 view_ = 0;

		Uint32 sampler_ = 0;
	};
	SC_STATIC_ASSERT(ViewBindings, 16, "Shader/RootSignature.hlsli");
}
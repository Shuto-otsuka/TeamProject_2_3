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
		Depth,
		Meshlet,
		Normal,
		Roughness,
		Metallic,
		Emissive,
		Velocity,
		ReflectionRaw,
		ReflectionDenoised,
		GlobalIlluminationRaw,
		GlobalIlluminationDenoised,
		ShadowRaw,
		ShadowDenoised,
		AmbientOcclusionRaw,
		AmbientOcclusionDenoised,
	};

	struct RenderView
	{
		RenderViewKind kind_ = RenderViewKind::Game;

		RenderViewMode mode_ = RenderViewMode::Lit;

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
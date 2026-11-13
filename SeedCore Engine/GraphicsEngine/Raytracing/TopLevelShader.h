#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct TopLevelHitStructuredBuffer
	{
		Uint vertexBufferIndex_ = SC_INVALID;

		Uint indexBufferIndex_ = SC_INVALID;

		Uint surfaceBufferIndex_ = SC_INVALID;

		Uint materialBufferIndex_ = SC_INVALID;

		Uint materialOffset_ = SC_INVALID;

		Uint topLevelHitStructuredBufferPadding0_ = 0;

		Vector2 texcoordMin_ = { 0.0f, 0.0f };

		Vector2 texcoordExtent_ = { 0.0f, 0.0f };

		Uint topLevelHitStructuredBufferPadding1_ = 0;

		Uint topLevelHitStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(TopLevelHitStructuredBuffer, 48, "Raytracing/TopLevel.hlsli");

	struct TopLevelInstanceStructuredBuffer
	{
		StaticArray<Vector4, 3> transform_ = {};

		Uint64 bottomLevel_ = 0;

		Uint instanceMask_ = 0;

		Uint flags_ = 0;

		TopLevelHitStructuredBuffer hit_;
	};
	SC_STATIC_ASSERT(TopLevelInstanceStructuredBuffer, 112, "Raytracing/TopLevel.hlsli");

	struct TopLevelRequest
	{
		Uint32 count_ = 0;
	};

	struct TopLevelUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Raytracing/TopLevelCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 1;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct TopLevelResolveDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Raytracing/TopLevelCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Resolve";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}

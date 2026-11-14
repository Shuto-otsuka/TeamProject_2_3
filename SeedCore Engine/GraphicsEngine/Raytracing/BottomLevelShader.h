#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct BottomLevelTableStructuredBuffer
	{
		Uint64 key_ = 0;

		Uint64 address_ = 0;

		Uint vertexOffset_ = SC_INVALID;

		Uint bottomLevelTableStructuredBufferPadding0_ = 0;

		Uint bottomLevelTableStructuredBufferPadding1_ = 0;

		Uint bottomLevelTableStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(BottomLevelTableStructuredBuffer, 32, "Raytracing/BottomLevel.hlsli");

	struct BottomLevelTableUpdate
	{
		Uint slot_ = 0;

		Uint bottomLevelTableUpdatePadding0_ = 0;

		Uint bottomLevelTableUpdatePadding1_ = 0;

		Uint bottomLevelTableUpdatePadding2_ = 0;

		BottomLevelTableStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(BottomLevelTableUpdate, 48, "Raytracing/BottomLevelCS.hlsl");

	struct BottomLevelRequest
	{
		Uint64 key_ = 0;

		D3D12_RAYTRACING_GEOMETRY_DESC geometry_{};

		Bool dynamic_ = false;
	};

	struct BottomLevelClearDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Raytracing/BottomLevelCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Clear";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct BottomLevelUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Raytracing/BottomLevelCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}

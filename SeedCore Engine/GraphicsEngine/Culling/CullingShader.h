#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct CullingPrepareDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Culling/CullingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Prepare";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 1;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct CullingEarlyDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Culling/CullingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Early";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct CullingLateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Culling/CullingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Late";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct CullingTransparentDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Culling/CullingCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Transparent";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}

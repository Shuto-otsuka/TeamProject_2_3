#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>

namespace SeedCore
{
	struct ModelUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ModelClearDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Clear";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ModelPrepareDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Prepare";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ModelExpandDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Expand";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ModelDeformDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Deform";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct ModelRaytracingDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Model/ModelDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Raytracing";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};
}

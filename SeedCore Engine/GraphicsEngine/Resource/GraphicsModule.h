#pragma once

#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>

namespace SeedCore
{
	class ResourceCache;
	class BindlessHeap;
	class D3D12Context;
	class BC7CompressShader;

	class SEEDCORE_API GraphicsModule
	{
	public:
		GraphicsModule(D3D12Context& context, BindlessHeap& heap, BC7CompressShader& bc7Shader);

		void Install(ResourceCache& cache);

		NODISCARD GraphicsLoaderSystem& Loaders();

		NODISCARD GraphicsResourceSystem& Resources();

	private:
		GraphicsLoaderSystem loaders_;

		GraphicsResourceSystem resources_;
	};
}
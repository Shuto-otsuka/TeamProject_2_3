#pragma once

#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>

namespace SeedCore
{
	class ResourceCache;
	class BindlessHeap;

	class SEEDCORE_API GraphicsModule
	{
	public:
		GraphicsModule(BindlessHeap& heap);

		void Install(ResourceCache& cache);

		NODISCARD GraphicsLoaderSystem& Loaders();

		NODISCARD GraphicsResourceSystem& Resources();

	private:
		GraphicsLoaderSystem loaders_;

		GraphicsResourceSystem resources_;
	};
}
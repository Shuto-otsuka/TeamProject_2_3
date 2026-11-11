#pragma once

#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/Resource/GraphicsExporterSystem.h>

namespace SeedCore
{
	class ResourceCache;
	class BindlessHeap;
	class D3D12Context;
	class GraphicsProcessor;

	class SEEDCORE_API GraphicsModule
	{
	public:
		GraphicsModule(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);

		void Install(ResourceCache& cache);

		NODISCARD GraphicsLoaderSystem& Loaders();

		NODISCARD GraphicsResourceSystem& Resources();

		NODISCARD GraphicsExporterSystem& Exporters();

	private:
		GraphicsLoaderSystem loaders_;

		GraphicsResourceSystem resources_;

		GraphicsExporterSystem exporters_;
	};
}
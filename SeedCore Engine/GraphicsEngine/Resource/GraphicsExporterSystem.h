#pragma once
#include <GraphicsEngine/Model/ModelExporter.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	struct SEEDCORE_API GraphicsExporterSystem
	{
		ResourcePtr<ModelExporter> modelExporter_;

		GraphicsExporterSystem(GraphicsLoaderSystem& loaders);
	};
}

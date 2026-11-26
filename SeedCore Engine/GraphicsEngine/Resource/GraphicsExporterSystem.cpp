#include <GraphicsEngine/Resource/GraphicsExporterSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	GraphicsExporterSystem::GraphicsExporterSystem(GraphicsLoaderSystem& loaders)
	{
		modelExporter_ = MakePtr<ModelExporter>(loaders);
	}
}

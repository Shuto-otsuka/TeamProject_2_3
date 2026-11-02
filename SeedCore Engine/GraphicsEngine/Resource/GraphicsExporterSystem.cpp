#include <GraphicsEngine/Resource/GraphicsExporterSystem.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates every GraphicsEngine exporter, handing each the loaders
	* it reads its asset with.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GraphicsEngine の全エクスポーターを生成し、それぞれにアセットを
	* 読むためのローダーを渡す。
	*/
	GraphicsExporterSystem::GraphicsExporterSystem(GraphicsLoaderSystem& loaders)
	{
		modelExporter_ = MakePtr<ModelExporter>(loaders);
	}
}

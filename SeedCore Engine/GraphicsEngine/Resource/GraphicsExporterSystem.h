#pragma once
#include <GraphicsEngine/Model/ModelExporter.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	/**
	* [EN]
	* Bundles GraphicsEngine's exporters, beside the loaders and the
	* resources: each writes one asset type out in an external, standard
	* format through the same Export call.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GraphicsEngine のエクスポーターを、ローダーとリソースの横にまとめて
	* 持つ。それぞれが同じ Export の呼び出しで、1つのアセット種別を外部の
	* 標準的な形式へ書き出す。
	*/
	struct SEEDCORE_API GraphicsExporterSystem
	{
		/// [EN] Exports model assets to glTF/GLB/FBX.
		/// [JP] モデルのアセットを glTF/GLB/FBX へ書き出す。
		ResourcePtr<ModelExporter> modelExporter_;

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
		GraphicsExporterSystem(GraphicsLoaderSystem& loaders);
	};
}

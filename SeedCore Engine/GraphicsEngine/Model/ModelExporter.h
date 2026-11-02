#pragma once
#include <FoundationEngine/Resource/Asset/AssetExporter.h>
#include <GraphicsEngine/Model/ModelWriter.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	/**
	* [EN]
	* Exports a model asset to glTF/GLB/FBX. The model is read from its
	* source into a separate, unbaked Crister that is released afterwards,
	* so a copy already loaded for rendering is left untouched; the file
	* itself is written by ModelWriter. Which format, axis system and unit
	* scale is set with Preset before Export.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのアセットを glTF/GLB/FBX へ書き出す。モデルはソースから
	* 焼き込まない別の Crister へ読み込み、終わったら解放するので、描画用に
	* 読み込み済みの写しには触れない。ファイル自体は ModelWriter が書く。
	* 形式・軸系・単位の倍率は、Export の前に Preset で設定する。
	*/
	class SEEDCORE_API ModelExporter :public AssetExporter
	{
	public:
		/**
		* [EN]
		* Takes the loaders, whose model loader reads a model for export.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローダーを受け取る。書き出すモデルは、そのモデルのローダーで読む。
		*/
		ModelExporter(GraphicsLoaderSystem& loaders);
		~ModelExporter() = default;

		/**
		* [EN]
		* Chooses the format, axis system and unit scale the next Export
		* writes with.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次の Export が書き出す形式・軸系・単位の倍率を選ぶ。
		*/
		void Preset(ExportPreset preset);

		/**
		* [EN]
		* Exports the model asset assetID to outputPath with the current
		* preset. Returns false when outputPath is empty or the source itself,
		* the model cannot be read, or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルのアセット assetID を、今のプリセットで outputPath へ書き出す。
		* outputPath が空かソース自身、モデルを読めない、または書き込みに
		* 失敗した場合は false を返す。
		*/
		Bool Export(const AssetContext& context, Uint32 assetID, String outputPath)override;

	private:
		/// [EN] Loaders whose model loader reads a model for export.
		/// [JP] 書き出すモデルを読むモデルのローダーを持つローダー群。
		GraphicsLoaderSystem& loaders_;

		/// [EN] Format, axis system and unit scale the next Export writes with.
		/// [JP] 次の Export が書き出す形式・軸系・単位の倍率。
		ExportPreset preset_ = ExportPreset::Gltf;
	};
}

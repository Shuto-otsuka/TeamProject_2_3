#include <GraphicsEngine/Model/ModelExporter.h>
#include <GraphicsEngine/Model/ModelLoader.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the loaders, whose model loader reads a model for export.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ローダーを受け取る。書き出すモデルは、そのモデルのローダーで読む。
	*/
	ModelExporter::ModelExporter(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

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
	void ModelExporter::Preset(ExportPreset preset)
	{
		preset_ = preset;
	}

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
	Bool ModelExporter::Export(const AssetContext& context, Uint32 assetID, String outputPath)
	{
		AssetRecord* asset = context.cache_.GetAsset(assetID);
		if (!asset)
		{
			return false;
		}

		/// [EN] Writing over the source would destroy the very file being read, so that is refused.
		/// [JP] ソースへ上書きすると読んでいるファイルそのものを壊すため、断る。
		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path exportPath(outputPath.c_str());
		if (exportPath.empty() || modelPath.lexically_normal() == exportPath.lexically_normal())
		{
			return false;
		}

		/// [EN] A separate, unbaked Crister is read for the export, so the copy loaded for rendering is never decoded or released here.
		/// [JP] 書き出し用に、焼き込まない別の Crister を読み込む。描画用に読み込み済みの写しを、ここでデコードしたり解放したりしないようにするため。
		ModelLoader& modelLoader = *loaders_.modelLoader_;
		Handle<Crister> handle = modelLoader.Import(asset->fullpath_, context.thread_, true);
		Crister* crister = modelLoader.Get(handle);
		if (!crister)
		{
			return false;
		}

		Bool result = ModelWriter::Write(*crister, ModelWriter::Profile(preset_), outputPath);

		modelLoader.Clear(handle);
		return result;
	}
}

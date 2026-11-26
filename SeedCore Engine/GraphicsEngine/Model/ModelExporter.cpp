#include <GraphicsEngine/Model/ModelExporter.h>
#include <GraphicsEngine/Model/ModelLoader.h>
#include <GraphicsEngine/Resource/GraphicsLoaderSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	ModelExporter::ModelExporter(GraphicsLoaderSystem& loaders) :loaders_(loaders)
	{
		/// No Code
	}

	void ModelExporter::Preset(ExportPreset preset)
	{
		preset_ = preset;
	}

	Bool ModelExporter::Export(const AssetContext& context, Uint32 assetID, String outputPath)
	{
		AssetRecord* asset = context.cache_.Record(assetID);
		if (!asset)
		{
			return false;
		}

		std::filesystem::path modelPath(asset->fullpath_.c_str());
		std::filesystem::path exportPath(outputPath.c_str());
		if (exportPath.empty() || modelPath.lexically_normal() == exportPath.lexically_normal())
		{
			return false;
		}

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

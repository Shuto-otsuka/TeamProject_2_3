#pragma once
#include <FoundationEngine/Resource/Asset/AssetExporter.h>
#include <GraphicsEngine/Model/ModelWriter.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class SEEDCORE_API ModelExporter :public AssetExporter
	{
	public:
		ModelExporter(GraphicsLoaderSystem& loaders);
		~ModelExporter() = default;

		void Preset(ExportPreset preset);

		Bool Export(const AssetContext& context, Uint32 assetID, String outputPath)override;

	private:
		GraphicsLoaderSystem& loaders_;

		ExportPreset preset_ = ExportPreset::Gltf;
	};
}

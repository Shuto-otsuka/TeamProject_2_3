#pragma once
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	class SEEDCORE_API AssetExporter :public NonCopyable
	{
	public:
		virtual ~AssetExporter() = default;

		virtual Bool Export(const AssetContext& context, Uint32 assetID, String outputPath) = 0;
	};
}

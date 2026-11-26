#pragma once
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>

namespace SeedCore
{
	struct SharedFile
	{
		String path_;

		String hash_;

		String driveId_;

		Uint64 size_ = 0;
	};

	struct SharedAsset
	{
		String id_;

		String path_;

		Uint32 runtimeId_ = 0;

		Int32 type_ = 0;

		Uint64 revision_ = 0;

		DynamicArray<SharedFile> files_;

		DynamicArray<String> dependencies_;

		Bool scene_ = false;

		String sceneDocumentId_;
	};

	class SEEDCORE_API SharedCatalog
	{
	public:
		SharedCatalog(GoogleDocument& document, const String& documentId);

		~SharedCatalog();

		SharedCatalog(const SharedCatalog&) = delete;

		SharedCatalog& operator=(const SharedCatalog&) = delete;

		Bool Refresh();

		const DynamicArray<SharedAsset>& Assets()const;

		const SharedAsset* Find(const String& assetId)const;

		const SharedAsset* Find(Uint32 runtimeId)const;

		const SharedAsset* FindPath(const String& path)const;

		Bool Register(const SharedAsset& asset);

		Bool Publish(const String& assetId, const DynamicArray<SharedFile>& files, const DynamicArray<String>& dependencies, Uint64 baseRevision, Uint64& revision);

		Bool Remove(const String& assetId);

		const String& Error()const;

	private:
		GoogleDocument& document_;

		String documentId_;

		DynamicArray<SharedAsset> assets_;

		String error_;
	};
}

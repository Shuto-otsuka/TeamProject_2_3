#pragma once
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Resource/Sharing/SharingWorker.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API ResourceSync
	{
	public:
		explicit ResourceSync(const std::filesystem::path& projectRoot);

		~ResourceSync();

		ResourceSync(const ResourceSync&) = delete;

		ResourceSync& operator=(const ResourceSync&) = delete;

		void Update();

		void ConsumeChangedAsset(DynamicArray<Uint32>& assets);

		void ConsumeImportedAsset(DynamicArray<String>& paths);

		void FetchReferenced(World& world);

		Bool Configured()const;

		Bool Online()const;

		Uint64 Revision()const;

		void Gather(DynamicArray<AssetRecord>& assets)const;

		Bool RemoteOnly(Uint32 assetId)const;

		Bool Modified(Uint32 assetId)const;

		Bool Conflicted(Uint32 assetId)const;

		Bool Managed(const std::filesystem::path& path)const;

		Bool Shareable(const std::filesystem::path& path)const;

		void RequestGet(Uint32 assetId);

		void RequestAdopt(Uint32 assetId);

		void RequestPublish(Uint32 assetId);

		void RequestRegister(const AssetRecord& asset);

		void RequestUnshare(Uint32 assetId);

		void RequestRefresh();

		void RequestOpenScene(const std::filesystem::path& path);

		const SharedAsset* GetAsset(Uint32 assetId)const;

		const SharedAsset* GetAsset(const std::filesystem::path& path)const;

		const String& Operation()const;

		const String& Error()const;

	private:
		String Logical(const std::filesystem::path& path)const;

	private:
		std::filesystem::path projectRoot_;

		SharingConfig config_;

		Bool configured_ = false;

		ResourcePtr<SharingWorker> worker_;

		SharingSnapshot snapshot_;

		std::set<std::string_view> sourceExtensions_ =
		{
			".h", ".cpp", ".cs", ".hlsl", ".hlsli",
		};

		Bool fetchReferencedRequested_ = false;
	};
}

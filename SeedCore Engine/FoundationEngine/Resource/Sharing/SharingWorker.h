#pragma once
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>
#include <FoundationEngine/Resource/Sharing/GoogleDrive.h>
#include <FoundationEngine/Resource/Sharing/HttpClient.h>
#include <FoundationEngine/Resource/Sharing/SharedCatalog.h>

namespace SeedCore
{
	struct SharingConfig
	{
		String clientId_;
		String clientSecret_;

		String catalogDocumentId_;

		String blobFolderId_;

		String assetsFolderId_;

		String workspace_;
	};

	struct WorkspaceFile
	{
		String path_;

		String hash_;

		Uint64 size_ = 0;
		Int64 stamp_ = 0;
	};

	struct WorkspaceRecord
	{
		Uint64 revision_ = 0;

		DynamicArray<WorkspaceFile> files_;
	};

	struct ScenePartBase
	{
		Uint64 revision_ = 0;

		String hash_;
	};

	struct SceneBase
	{
		std::unordered_map<String, ScenePartBase> parts_;
	};

	enum class SharingAction
	{
		Refresh,

		Get,

		Adopt,

		Register,

		Publish,

		Unshare,

		OpenScene,
	};

	struct SharingRequest
	{
		SharingAction action_ = SharingAction::Refresh;

		String assetId_;

		String path_;

		Uint32 runtimeId_ = 0;
		Int32 type_ = 0;
	};

	struct SharedProgress
	{
		String assetId_;

		Bool modified_ = false;

		Bool conflicted_ = false;
	};

	struct SharingSnapshot
	{
		Bool online_ = false;

		String operation_;

		String error_;

		DynamicArray<SharedAsset> assets_;

		DynamicArray<SharedProgress> progress_;

		Uint64 revision_ = 0;
	};

	class SEEDCORE_API SharingWorker
	{
	public:
		SharingWorker(const std::filesystem::path& projectRoot, const SharingConfig& config);

		~SharingWorker();

		SharingWorker(const SharingWorker&) = delete;

		SharingWorker& operator=(const SharingWorker&) = delete;

		void Start();

		void Stop();

		void Enqueue(const SharingRequest& request);

		SharingSnapshot Snapshot()const;

		void ConsumeChanged(DynamicArray<Uint32>& assets);

		void ConsumeImported(DynamicArray<String>& paths);

	private:
		void Run();

		void Process(const SharingRequest& request);

		void Pull();

		void Follow(const std::unordered_map<std::string, DriveEntry>& placed);

		void Prune(const std::unordered_map<std::string, DriveEntry>& placed, Bool complete);

		Bool Get(const String& assetId);

		Bool Adopt(const String& assetId);

		Bool Publish(const String& assetId);

		Bool Register(const SharingRequest& request);

		Bool Unshare(const String& assetId);

		Bool Mirror(const std::filesystem::path& localFolder, const String& driveFolderId, const std::set<std::string>& sharedPaths, const std::set<std::string>& sharedIds, std::unordered_map<std::string, DriveEntry>& placed);

		Bool Store(const String& logicalPath, SharedFile& stored, DynamicArray<String>& uploaded, DynamicArray<String>& replaced);

		DynamicArray<String> Companions(const String& logicalPath)const;

		std::filesystem::path Local(const String& logicalPath)const;

		void Capture();

		void LoadWorkspace();

		void SaveWorkspace()const;

	private:
		std::filesystem::path projectRoot_;

		SharingConfig config_;

		HttpClient http_;
		GoogleAuth auth_;
		GoogleDocument document_;
		GoogleDrive drive_;
		SharedCatalog catalog_;

		std::thread thread_;
		std::atomic<Bool> running_ = false;

		std::unordered_map<String, WorkspaceRecord> workspace_;

		String openScene_;

		std::unordered_map<String, SceneBase> sceneBases_;

		std::queue<SharingRequest> requests_;

		mutable std::mutex requestMutex_;

		SharingSnapshot snapshot_;
		mutable std::mutex snapshotMutex_;

		DynamicArray<Uint32> changed_;

		DynamicArray<String> imported_;
	};
}

#pragma once
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Resource/Sharing/HttpClient.h>

namespace SeedCore
{
	struct DriveEntry
	{
		String id_;

		String name_;

		Bool folder_ = false;

		String hash_;

		Uint64 size_ = 0;

		String modified_;

		Bool canTrash_ = false;
	};

	class SEEDCORE_API GoogleDrive
	{
	public:
		GoogleDrive(HttpClient& http, GoogleAuth& auth);

		~GoogleDrive();

		GoogleDrive(const GoogleDrive&) = delete;

		GoogleDrive& operator=(const GoogleDrive&) = delete;

		String Find(const String& name, const String& parentId);

		Bool Exists(const String& fileId, Bool& exists);

		Bool List(const String& parentId, DynamicArray<DriveEntry>& entries);

		String Folder(const String& name, const String& parentId);

		String Upload(const std::filesystem::path& source, const String& name, const String& parentId);

		Bool Fetch(const String& fileId, const std::filesystem::path& destination);

		Bool Trash(const String& fileId);

		const String& Error()const;

	private:
		HttpClient& http_;

		GoogleAuth& auth_;

		String error_;
	};
}

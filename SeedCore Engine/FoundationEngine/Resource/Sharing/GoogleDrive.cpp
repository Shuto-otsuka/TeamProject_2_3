#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/GoogleDrive.h>

namespace SeedCore
{
	static Bool SendDrive(HttpClient& http, GoogleAuth& auth, String& error, const String& method, const String& url, const nlohmann::json& request, nlohmann::json& result);

	GoogleDrive::GoogleDrive(HttpClient& http, GoogleAuth& auth) : http_(http), auth_(auth)
	{
		/// No Code
	}

	GoogleDrive::~GoogleDrive()
	{
		/// No Code
	}

	String GoogleDrive::Find(const String& name, const String& parentId)
	{
		std::string escapedName;
		for (Char character : name.str())
		{
			if (character == '\\' || character == '\'')
			{
				escapedName += '\\';
			}
			escapedName += character;
		}
		std::string escapedParent;
		for (Char character : parentId.str())
		{
			if (character == '\\' || character == '\'')
			{
				escapedParent += '\\';
			}
			escapedParent += character;
		}

		String query = String(std::format("name = '{}' and '{}' in parents and trashed = false", escapedName, escapedParent));

		nlohmann::json result;
		if (!SendDrive(http_, auth_, error_, String("GET"), String(std::format("https://www.googleapis.com/drive/v3/files?q={}&fields=files(id)&pageSize=1", HttpClient::Escape(query).str())), nlohmann::json(), result))
		{
			return String();
		}

		if (!result.contains("files") || result["files"].empty())
		{
			return String();
		}
		return String(result["files"][0].value("id", ""));
	}

	Bool GoogleDrive::Exists(const String& fileId, Bool& exists)
	{
		String token = auth_.AccessToken();
		if (token.str().empty())
		{
			error_ = auth_.Error();
			return false;
		}

		DynamicArray<HttpHeader> headers;
		headers.push_back(HttpHeader{ String("Authorization"), String(std::format("Bearer {}", token.str())) });
		HttpResponse response = http_.Send(String("GET"), String(std::format("https://www.googleapis.com/drive/v3/files/{}?fields=trashed", fileId.str())), headers);

		if (response.status_ == 404)
		{
			error_ = String();
			exists = false;
			return true;
		}

		if (response.status_ == 200)
		{
			nlohmann::json result = nlohmann::json::parse(response.body_.begin(), response.body_.end(), nullptr, false);
			if (!result.is_object())
			{
				error_ = String("Drive の応答を解釈できませんでした。");
				return false;
			}
			error_ = String();
			exists = !result.value("trashed", false);
			return true;
		}

		error_ = response.status_ == 0 ? response.error_ : String(std::format("Drive へのアクセスに失敗しました ({})。", response.status_));
		return false;
	}

	Bool GoogleDrive::List(const String& parentId, DynamicArray<DriveEntry>& entries)
	{
		std::string escapedParent;
		for (Char character : parentId.str())
		{
			if (character == '\\' || character == '\'')
			{
				escapedParent += '\\';
			}
			escapedParent += character;
		}

		String query = String(std::format("'{}' in parents and trashed = false", escapedParent));

		std::string pageToken;
		do
		{
			std::string url = std::format("https://www.googleapis.com/drive/v3/files?q={}&fields=nextPageToken,files(id,name,mimeType,size,sha256Checksum,modifiedTime,capabilities/canTrash)&pageSize=1000", HttpClient::Escape(query).str());

			if (!pageToken.empty())
			{
				url += std::format("&pageToken={}", HttpClient::Escape(String(pageToken)).str());
			}

			nlohmann::json result;
			if (!SendDrive(http_, auth_, error_, String("GET"), String(url), nlohmann::json(), result))
			{
				return false;
			}

			for (const nlohmann::json& file : result["files"])
			{
				DriveEntry entry;
				entry.id_ = String(file.value("id", ""));
				entry.name_ = String(file.value("name", ""));

				entry.folder_ = file.value("mimeType", "") == "application/vnd.google-apps.folder";
				entry.hash_ = String(file.value("sha256Checksum", ""));

				entry.size_ = file.contains("size") ? std::strtoull(file["size"].get<std::string>().c_str(), nullptr, 10) : 0;
				entry.modified_ = String(file.value("modifiedTime", ""));
				entry.canTrash_ = file.contains("capabilities") && file["capabilities"].value("canTrash", false);
				entries.push_back(entry);
			}

			pageToken = result.value("nextPageToken", "");
		}
		while (!pageToken.empty());
		return true;
	}

	String GoogleDrive::Folder(const String& name, const String& parentId)
	{
		nlohmann::json request;
		request["name"] = name.str();
		request["mimeType"] = "application/vnd.google-apps.folder";

		request["parents"] = nlohmann::json::array({ parentId.str() });

		nlohmann::json result;
		if (!SendDrive(http_, auth_, error_, String("POST"), String("https://www.googleapis.com/drive/v3/files?fields=id"), request, result))
		{
			return String();
		}
		return String(result.value("id", ""));
	}

	String GoogleDrive::Upload(const std::filesystem::path& source, const String& name, const String& parentId)
	{
		std::error_code errorCode;
		Uint64 size = std::filesystem::file_size(source, errorCode);
		if (errorCode)
		{
			error_ = String(std::format("\"{}\" を読み取れませんでした。", source.string()));
			return String();
		}

		String token = auth_.AccessToken();
		if (token.str().empty())
		{
			error_ = auth_.Error();
			return String();
		}

		nlohmann::json request;
		request["name"] = name.str();
		request["parents"] = nlohmann::json::array({ parentId.str() });
		std::string body = request.dump();
		DynamicArray<Byte> payload(body.size());
		std::memcpy(payload.data(), body.data(), body.size());

		DynamicArray<HttpHeader> headers;
		headers.push_back(HttpHeader{ String("Authorization"), String(std::format("Bearer {}", token.str())) });
		headers.push_back(HttpHeader{ String("Content-Type"), String("application/json; charset=utf-8") });
		headers.push_back(HttpHeader{ String("X-Upload-Content-Type"), String("application/octet-stream") });
		headers.push_back(HttpHeader{ String("X-Upload-Content-Length"), String(std::format("{}", size)) });
		HttpResponse opened = http_.Send(String("POST"), String("https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable&fields=id"), headers, payload);

		if (opened.status_ != 200 || opened.location_.str().empty())
		{
			error_ = String(std::format("アップロードを開始できませんでした ({})。", opened.status_));
			return String();
		}
		String session = opened.location_;

		std::ifstream stream(source, std::ios::binary);
		if (!stream)
		{
			error_ = String(std::format("\"{}\" を開けませんでした。", source.string()));
			return String();
		}

		const Uint64 chunkSize = 8 * 1024 * 1024;
		DynamicArray<Byte> chunk;
		Uint64 offset = 0;
		while (offset < size)
		{
			Uint64 remaining = size - offset;
			Uint64 length = remaining < chunkSize ? remaining : chunkSize;
			chunk.resize(static_cast<Size>(length));
			stream.read(chunk.data(), static_cast<std::streamsize>(length));

			DynamicArray<HttpHeader> rangeHeaders;
			rangeHeaders.push_back(HttpHeader{ String("Content-Range"), String(std::format("bytes {}-{}/{}", offset, offset + length - 1, size)) });
			HttpResponse response = http_.Send(String("PUT"), session, rangeHeaders, chunk);

			if (response.status_ == 308)
			{
				offset += length;
				continue;
			}

			if (response.status_ == 200 || response.status_ == 201)
			{
				nlohmann::json result = nlohmann::json::parse(response.body_.begin(), response.body_.end(), nullptr, false);
				error_ = String();
				return String(result.is_object() ? result.value("id", "") : "");
			}

			error_ = String(std::format("アップロードの途中で失敗しました ({})。", response.status_));
			return String();
		}

		error_ = String("アップロードが完了しませんでした。");
		return String();
	}

	Bool GoogleDrive::Fetch(const String& fileId, const std::filesystem::path& destination)
	{
		String token = auth_.AccessToken();
		if (token.str().empty())
		{
			error_ = auth_.Error();
			return false;
		}

		DynamicArray<HttpHeader> headers;
		headers.push_back(HttpHeader{ String("Authorization"), String(std::format("Bearer {}", token.str())) });
		HttpResponse response = http_.Download(String("GET"), String(std::format("https://www.googleapis.com/drive/v3/files/{}?alt=media", fileId.str())), headers, destination);

		if (response.status_ == 200 && response.error_.str().empty())
		{
			error_ = String();
			return true;
		}

		std::error_code errorCode;
		std::filesystem::remove(destination, errorCode);
		error_ = response.error_.str().empty() ? String(std::format("ファイルの取得に失敗しました ({})。", response.status_)) : response.error_;
		return false;
	}

	Bool GoogleDrive::Trash(const String& fileId)
	{
		nlohmann::json request;
		request["trashed"] = true;
		nlohmann::json result;
		return SendDrive(http_, auth_, error_, String("PATCH"), String(std::format("https://www.googleapis.com/drive/v3/files/{}?fields=id", fileId.str())), request, result);
	}

	const String& GoogleDrive::Error()const
	{
		return error_;
	}

	static Bool SendDrive(HttpClient& http, GoogleAuth& auth, String& error, const String& method, const String& url, const nlohmann::json& request, nlohmann::json& result)
	{
		String token = auth.AccessToken();
		if (token.str().empty())
		{
			error = auth.Error();
			return false;
		}

		DynamicArray<HttpHeader> headers;
		headers.push_back(HttpHeader{ String("Authorization"), String(std::format("Bearer {}", token.str())) });

		DynamicArray<Byte> payload;
		if (!request.is_null())
		{
			headers.push_back(HttpHeader{ String("Content-Type"), String("application/json; charset=utf-8") });
			std::string body = request.dump();
			payload.resize(body.size());
			std::memcpy(payload.data(), body.data(), body.size());
		}

		HttpResponse response = http.Send(method, url, headers, payload);
		if (response.status_ == 0)
		{
			error = response.error_;
			return false;
		}

		result = nlohmann::json::parse(response.body_.begin(), response.body_.end(), nullptr, false);
		if (response.status_ == 200)
		{
			error = String();
			return result.is_object();
		}

		std::string message = result.is_object() && result.contains("error") ? result["error"].value("message", "") : "";
		error = String(std::format("Drive へのアクセスに失敗しました ({}): {}", response.status_, message));
		return false;
	}
}

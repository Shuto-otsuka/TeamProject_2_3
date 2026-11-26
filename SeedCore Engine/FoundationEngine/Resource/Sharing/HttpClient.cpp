#include <FoundationEngine/Resource/Sharing/HttpClient.h>

namespace SeedCore
{
	HttpClient::HttpClient()
	{
		session_ = WinHttpOpen(L"SeedCore/2.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (session_)
		{
			WinHttpSetTimeouts(session_, 15000, 15000, 30000, 120000);
		}
	}

	HttpClient::~HttpClient()
	{
		if (session_)
		{
			WinHttpCloseHandle(session_);
		}
	}

	HttpResponse HttpClient::Send(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const DynamicArray<Byte>& body)
	{
		return Exchange(method, url, headers, body, nullptr);
	}

	HttpResponse HttpClient::Send(const String& method, const String& url, const DynamicArray<HttpHeader>& headers)
	{
		return Send(method, url, headers, DynamicArray<Byte>());
	}

	HttpResponse HttpClient::Download(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const std::filesystem::path& destination)
	{
		HttpResponse response;
		std::error_code errorCode;
		std::filesystem::create_directories(destination.parent_path(), errorCode);

		std::ofstream stream(destination, std::ios::binary);
		if (!stream)
		{
			response.error_ = String(std::format("\"{}\" を書き込み用に開けませんでした。", destination.string()));
			return response;
		}
		return Exchange(method, url, headers, DynamicArray<Byte>(), &stream);
	}

	Bool HttpClient::Ready()const
	{
		return session_ != nullptr;
	}

	String HttpClient::Escape(const String& value)
	{
		std::string source = value.str();
		std::string result;
		result.reserve(source.size() * 3);
		for (Char character : source)
		{
			Bool unreserved = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '-' || character == '_' || character == '.' || character == '~';
			if (unreserved)
			{
				result += character;
			}
			else
			{
				result += std::format("%{:02X}", static_cast<Uint8>(character));
			}
		}
		return String(result);
	}

	String HttpClient::Unescape(const String& value)
	{
		std::string source = value.str();
		std::string result;
		result.reserve(source.size());
		for (Size index = 0; index < source.size(); ++index)
		{
			if (source[index] == '%' && index + 2 < source.size())
			{
				result += static_cast<Char>(std::strtoul(source.substr(index + 1, 2).c_str(), nullptr, 16));
				index += 2;
			}
			else if (source[index] == '+')
			{
				result += ' ';
			}
			else
			{
				result += source[index];
			}
		}
		return String(result);
	}

	HttpResponse HttpClient::Exchange(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const DynamicArray<Byte>& body, std::ofstream* destination)
	{
		HttpResponse response;
		if (!session_)
		{
			response.error_ = String("WinHTTP セッションを利用できません。");
			return response;
		}

		std::wstring wideUrl = url.w_str();
		std::wstring host(256, L'\0');
		std::wstring path(4096, L'\0');
		std::wstring extra(2048, L'\0');

		URL_COMPONENTS components{};
		components.dwStructSize = sizeof(components);

		components.lpszHostName = host.data();
		components.dwHostNameLength = static_cast<DWORD>(host.size());
		components.lpszUrlPath = path.data();
		components.dwUrlPathLength = static_cast<DWORD>(path.size());
		components.lpszExtraInfo = extra.data();
		components.dwExtraInfoLength = static_cast<DWORD>(extra.size());

		if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components))
		{
			response.error_ = String(std::format("URL を解釈できませんでした: {}", url.str()));
			return response;
		}

		HINTERNET connection = WinHttpConnect(session_, components.lpszHostName, components.nPort, 0);
		if (!connection)
		{
			response.error_ = String(std::format("{} へ接続できませんでした。", String(std::wstring(components.lpszHostName, components.dwHostNameLength)).str()));
			return response;
		}

		std::wstring target = std::wstring(components.lpszUrlPath, components.dwUrlPathLength) + std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);

		DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;

		HINTERNET request = WinHttpOpenRequest(connection, method.w_str().c_str(), target.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
		if (!request)
		{
			WinHttpCloseHandle(connection);
			response.error_ = String("HTTP リクエストを作成できませんでした。");
			return response;
		}

		std::wstring headerBlock;
		for (const HttpHeader& header : headers)
		{
			headerBlock += header.name_.w_str() + L": " + header.value_.w_str() + L"\r\n";
		}

		Bool sent = WinHttpSendRequest(request, headerBlock.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerBlock.c_str(), headerBlock.empty() ? 0 : static_cast<DWORD>(headerBlock.size()), body.empty() ? nullptr : const_cast<Byte*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) != FALSE;

		if (!sent || !WinHttpReceiveResponse(request, nullptr))
		{
			response.error_ = String(std::format("応答が届く前にリクエストが失敗しました (Windows エラー {})。", GetLastError()));
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connection);
			return response;
		}

		DWORD status = 0;
		DWORD statusSize = sizeof(status);
		WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
		response.status_ = status;

		DWORD locationSize = 0;
		WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &locationSize, WINHTTP_NO_HEADER_INDEX);
		if (locationSize > 0)
		{
			std::wstring location(locationSize / sizeof(wchar_t), L'\0');
			if (WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX, location.data(), &locationSize, WINHTTP_NO_HEADER_INDEX))
			{
				response.location_ = String(std::wstring(location.c_str()));
			}
		}

		Bool toFile = destination != nullptr && response.status_ == 200;

		DynamicArray<Byte> chunk;
		DWORD available = 0;
		while (WinHttpQueryDataAvailable(request, &available) && available > 0)
		{
			Size offset = toFile ? 0 : response.body_.size();
			DynamicArray<Byte>& buffer = toFile ? chunk : response.body_;
			buffer.resize(offset + available);
			DWORD read = 0;
			if (!WinHttpReadData(request, buffer.data() + offset, available, &read))
			{
				response.error_ = String("応答の本文を最後まで読み取れませんでした。");
				break;
			}

			buffer.resize(offset + read);
			if (toFile)
			{
				destination->write(buffer.data(), buffer.size());
			}
		}

		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connection);
		return response;
	}
}

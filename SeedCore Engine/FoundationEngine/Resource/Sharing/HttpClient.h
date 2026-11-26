#pragma once

namespace SeedCore
{
	struct HttpHeader
	{
		String name_;

		String value_;
	};

	struct HttpResponse
	{
		Uint32 status_ = 0;

		DynamicArray<Byte> body_;

		String location_;

		String error_;
	};

	class SEEDCORE_API HttpClient
	{
	public:
		HttpClient();

		~HttpClient();

		HttpClient(const HttpClient&) = delete;

		HttpClient& operator=(const HttpClient&) = delete;

		HttpResponse Send(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const DynamicArray<Byte>& body);

		HttpResponse Send(const String& method, const String& url, const DynamicArray<HttpHeader>& headers);

		HttpResponse Download(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const std::filesystem::path& destination);

		Bool Ready()const;

		static String Escape(const String& value);

		static String Unescape(const String& value);

	private:
		HttpResponse Exchange(const String& method, const String& url, const DynamicArray<HttpHeader>& headers, const DynamicArray<Byte>& body, std::ofstream* destination);

	private:
		HINTERNET session_ = nullptr;
	};
}

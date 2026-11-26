#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>

namespace SeedCore
{
	static Bool SendDocument(HttpClient& http, GoogleAuth& auth, Bool& conflicted, String& error, const String& method, const String& url, const nlohmann::json& request, nlohmann::json& result);
	static String ExtractDocument(const nlohmann::json& document);

	GoogleDocument::GoogleDocument(HttpClient& http, GoogleAuth& auth) : http_(http), auth_(auth)
	{
		/// No Code
	}

	GoogleDocument::~GoogleDocument()
	{
		/// No Code
	}

	GoogleDocumentSnapshot GoogleDocument::Read(const String& documentId)
	{
		GoogleDocumentSnapshot snapshot;

		nlohmann::json result;
		if (!SendDocument(http_, auth_, conflicted_, error_, String("GET"), String(std::format("https://docs.googleapis.com/v1/documents/{}?fields=revisionId,body", documentId.str())), nlohmann::json(), result))
		{
			return snapshot;
		}

		snapshot.revisionId_ = String(result.value("revisionId", ""));
		snapshot.text_ = ExtractDocument(result);

		snapshot.endIndex_ = 1;
		if (result.contains("body") && result["body"].contains("content"))
		{
			for (const nlohmann::json& element : result["body"]["content"])
			{
				snapshot.endIndex_ = std::max(snapshot.endIndex_, element.value("endIndex", Uint32(1)));
			}
		}

		snapshot.valid_ = !snapshot.revisionId_.str().empty();
		return snapshot;
	}

	Bool GoogleDocument::Write(const String& documentId, const GoogleDocumentSnapshot& base, const String& text)
	{
		conflicted_ = false;
		if (!base.valid_)
		{
			error_ = String("書き込みの前に、ドキュメントを読み取れていません。");
			return false;
		}

		nlohmann::json request;
		request["requests"] = nlohmann::json::array();

		if (base.endIndex_ > 2)
		{
			nlohmann::json remove;
			remove["deleteContentRange"]["range"]["startIndex"] = 1;
			remove["deleteContentRange"]["range"]["endIndex"] = base.endIndex_ - 1;
			request["requests"].push_back(remove);
		}

		nlohmann::json insert;
		insert["insertText"]["location"]["index"] = 1;
		insert["insertText"]["text"] = text.str();
		request["requests"].push_back(insert);

		request["writeControl"]["requiredRevisionId"] = base.revisionId_.str();

		nlohmann::json result;
		return SendDocument(http_, auth_, conflicted_, error_, String("POST"), String(std::format("https://docs.googleapis.com/v1/documents/{}:batchUpdate", documentId.str())), request, result);
	}

	Bool GoogleDocument::Conflicted()const
	{
		return conflicted_;
	}

	const String& GoogleDocument::Error()const
	{
		return error_;
	}

	static Bool SendDocument(HttpClient& http, GoogleAuth& auth, Bool& conflicted, String& error, const String& method, const String& url, const nlohmann::json& request, nlohmann::json& result)
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
		if (response.status_ == 400 && message.find("revision") != std::string::npos)
		{
			conflicted = true;
			error = String();
			return false;
		}

		error = String(std::format("共有ドキュメントへのアクセスに失敗しました ({}): {}", response.status_, message));
		return false;
	}

	static String ExtractDocument(const nlohmann::json& document)
	{
		std::string text;
		if (!document.contains("body") || !document["body"].contains("content"))
		{
			return String(text);
		}
		for (const nlohmann::json& element : document["body"]["content"])
		{
			if (!element.contains("paragraph") || !element["paragraph"].contains("elements"))
			{
				continue;
			}
			for (const nlohmann::json& run : element["paragraph"]["elements"])
			{
				if (run.contains("textRun"))
				{
					text += run["textRun"].value("content", "");
				}
			}
		}
		return String(text);
	}
}

#pragma once
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Resource/Sharing/HttpClient.h>

namespace SeedCore
{
	struct GoogleDocumentSnapshot
	{
		String revisionId_;

		String text_;

		Uint32 endIndex_ = 1;

		Bool valid_ = false;
	};

	class SEEDCORE_API GoogleDocument
	{
	public:
		GoogleDocument(HttpClient& http, GoogleAuth& auth);

		~GoogleDocument();

		GoogleDocument(const GoogleDocument&) = delete;

		GoogleDocument& operator=(const GoogleDocument&) = delete;

		GoogleDocumentSnapshot Read(const String& documentId);

		Bool Write(const String& documentId, const GoogleDocumentSnapshot& base, const String& text);

		Bool Conflicted()const;

		const String& Error()const;

	private:
		HttpClient& http_;

		GoogleAuth& auth_;

		Bool conflicted_ = false;

		String error_;
	};
}

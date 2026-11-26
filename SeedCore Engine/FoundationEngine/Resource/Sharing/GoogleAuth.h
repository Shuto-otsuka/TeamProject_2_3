#pragma once
#include <FoundationEngine/Resource/Sharing/HttpClient.h>

namespace SeedCore
{
	class SEEDCORE_API GoogleAuth
	{
	public:
		GoogleAuth(HttpClient& http, const std::filesystem::path& tokenPath, const String& clientId, const String& clientSecret);

		~GoogleAuth();

		GoogleAuth(const GoogleAuth&) = delete;

		GoogleAuth& operator=(const GoogleAuth&) = delete;

		Bool SignedIn()const;

		Bool SignIn();

		void SignOut();

		String AccessToken();

		const String& Error()const;

	private:
		Bool Refresh();

		Bool RequestToken(const String& form);

		String ReceiveCode(SOCKET listener);

		void LoadToken();

		void SaveToken()const;

	private:
		HttpClient& http_;

		std::filesystem::path tokenPath_;

		String clientId_;

		String clientSecret_;

		String refreshToken_;

		String accessToken_;

		Double expiresAt_ = 0.0;

		String error_;
	};
}

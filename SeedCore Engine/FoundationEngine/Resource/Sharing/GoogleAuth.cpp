#include <External/Json/json.hpp>
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>

namespace SeedCore
{
	GoogleAuth::GoogleAuth(HttpClient& http, const std::filesystem::path& tokenPath, const String& clientId, const String& clientSecret) : http_(http), tokenPath_(tokenPath), clientId_(clientId), clientSecret_(clientSecret)
	{
		LoadToken();
	}

	GoogleAuth::~GoogleAuth()
	{
		/// No Code
	}

	Bool GoogleAuth::SignedIn()const
	{
		return !refreshToken_.str().empty();
	}

	Bool GoogleAuth::SignIn()
	{
		WSADATA winsock{};
		if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0)
		{
			error_ = String("Windows のネットワーク機能を初期化できませんでした。");
			return false;
		}

		SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listener == INVALID_SOCKET)
		{
			WSACleanup();
			error_ = String("ログイン受け取り用のソケットを作成できませんでした。");
			return false;
		}

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = 0;
		Int addressSize = sizeof(address);

		if (bind(listener, reinterpret_cast<sockaddr*>(&address), addressSize) == SOCKET_ERROR || listen(listener, 1) == SOCKET_ERROR || getsockname(listener, reinterpret_cast<sockaddr*>(&address), &addressSize) == SOCKET_ERROR)
		{
			closesocket(listener);
			WSACleanup();
			error_ = String("ログインの戻り先となるポートを用意できませんでした。");
			return false;
		}

		Uint16 port = ntohs(address.sin_port);

		String redirect = String(std::format("http://127.0.0.1:{}/", port));

		String scope = String("openid email https://www.googleapis.com/auth/drive https://www.googleapis.com/auth/documents");

		String authorizeUrl = String(std::format("https://accounts.google.com/o/oauth2/v2/auth?client_id={}&redirect_uri={}&response_type=code&access_type=offline&prompt=select_account%20consent&scope={}", HttpClient::Escape(clientId_).str(), HttpClient::Escape(redirect).str(), HttpClient::Escape(scope).str()));

		ShellExecuteW(nullptr, L"open", authorizeUrl.w_str().c_str(), nullptr, nullptr, SW_SHOWNORMAL);

		String code = ReceiveCode(listener);

		closesocket(listener);
		WSACleanup();
		if (code.str().empty())
		{
			return false;
		}

		return RequestToken(String(std::format("code={}&client_id={}&client_secret={}&redirect_uri={}&grant_type=authorization_code", HttpClient::Escape(code).str(), HttpClient::Escape(clientId_).str(), HttpClient::Escape(clientSecret_).str(), HttpClient::Escape(redirect).str())));
	}

	void GoogleAuth::SignOut()
	{
		refreshToken_ = String();
		accessToken_ = String();
		expiresAt_ = 0.0;

		std::error_code errorCode;
		std::filesystem::remove(tokenPath_, errorCode);
	}

	String GoogleAuth::AccessToken()
	{
		if (!SignedIn())
		{
			error_ = String("Google へのログインが必要です。");
			return String();
		}

		Double now = std::chrono::duration<Double>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (accessToken_.str().empty() || now >= expiresAt_)
		{
			if (!Refresh())
			{
				return String();
			}
		}
		return accessToken_;
	}

	const String& GoogleAuth::Error()const
	{
		return error_;
	}

	Bool GoogleAuth::Refresh()
	{
		return RequestToken(String(std::format("client_id={}&client_secret={}&refresh_token={}&grant_type=refresh_token", HttpClient::Escape(clientId_).str(), HttpClient::Escape(clientSecret_).str(), HttpClient::Escape(refreshToken_).str())));
	}

	Bool GoogleAuth::RequestToken(const String& form)
	{
		DynamicArray<HttpHeader> headers;
		headers.push_back(HttpHeader{ String("Content-Type"), String("application/x-www-form-urlencoded") });

		std::string body = form.str();
		DynamicArray<Byte> payload(body.size());
		std::memcpy(payload.data(), body.data(), body.size());

		HttpResponse response = http_.Send(String("POST"), String("https://oauth2.googleapis.com/token"), headers, payload);
		if (response.status_ == 0)
		{
			error_ = response.error_;
			return false;
		}

		nlohmann::json result = nlohmann::json::parse(response.body_.begin(), response.body_.end(), nullptr, false);
		if (!result.is_object())
		{
			error_ = String("Google の応答を解釈できませんでした。");
			return false;
		}
		if (response.status_ != 200)
		{
			if (result.value("error", "") == "invalid_grant")
			{
				SignOut();
				error_ = String("Google のログインが期限切れです。もう一度ログインしてください。");
			}
			else
			{
				error_ = String(std::format("Google のログインに失敗しました: {}", result.value("error_description", result.value("error", "unknown"))));
			}
			return false;
		}

		accessToken_ = String(result.value("access_token", ""));

		Double now = std::chrono::duration<Double>(std::chrono::system_clock::now().time_since_epoch()).count();
		expiresAt_ = now + static_cast<Double>(result.value("expires_in", 3600)) - 60.0;

		if (result.contains("refresh_token"))
		{
			refreshToken_ = String(result.value("refresh_token", ""));
			SaveToken();
		}
		error_ = String();
		return true;
	}

	String GoogleAuth::ReceiveCode(SOCKET listener)
	{
		DWORD timeout = 300000;
		setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const Char*>(&timeout), sizeof(timeout));

		SOCKET browser = accept(listener, nullptr, nullptr);
		if (browser == INVALID_SOCKET)
		{
			error_ = String("ログインの完了を待っている間に時間切れになりました。");
			return String();
		}

		setsockopt(browser, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const Char*>(&timeout), sizeof(timeout));

		std::string request(4096, '\0');
		Int received = recv(browser, request.data(), static_cast<Int>(request.size()) - 1, 0);
		if (received > 0)
		{
			request.resize(static_cast<Size>(received));
		}
		else
		{
			request.clear();
		}

		std::string page = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>SeedCore へのログインが完了しました。このタブは閉じて構いません。</body></html>";

		std::string reply = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", page.size(), page);
		send(browser, reply.data(), static_cast<Int>(reply.size()), 0);
		closesocket(browser);

		if (request.find("error=") != std::string::npos)
		{
			error_ = String("Google へのアクセス許可が拒否されました。");
			return String();
		}

		Size start = request.find("code=");
		if (start == std::string::npos)
		{
			error_ = String("ブラウザからの戻りに認可コードが含まれていませんでした。");
			return String();
		}
		start += 5;
		Size end = request.find_first_of("& \r\n", start);

		return HttpClient::Unescape(String(request.substr(start, end == std::string::npos ? std::string::npos : end - start)));
	}

	void GoogleAuth::LoadToken()
	{
		std::ifstream stream(tokenPath_, std::ios::binary);
		if (!stream)
		{
			return;
		}
		DynamicArray<Byte> content((std::istreambuf_iterator<Char>(stream)), std::istreambuf_iterator<Char>());

		if (content.size() <= 16)
		{
			return;
		}
		DynamicArray<Byte> iv(content.begin(), content.begin() + 16);
		DynamicArray<Byte> ciphertext(content.begin() + 16, content.end());

		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));

		DynamicArray<Byte> plaintext = Aes256::Decrypt(key, iv, ciphertext);
		if (!plaintext.empty())
		{
			refreshToken_ = String(std::string(plaintext.data(), plaintext.size()));
		}
	}

	void GoogleAuth::SaveToken()const
	{
		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));

		DynamicArray<Byte> iv(16);
		std::random_device randomDevice;
		for (Byte& ivByte : iv)
		{
			ivByte = static_cast<Byte>(randomDevice());
		}

		std::string token = refreshToken_.str();
		DynamicArray<Byte> plaintext(token.size());
		std::memcpy(plaintext.data(), token.data(), token.size());
		DynamicArray<Byte> ciphertext = Aes256::Encrypt(key, iv, plaintext);

		std::error_code errorCode;
		std::filesystem::create_directories(tokenPath_.parent_path(), errorCode);
		std::ofstream stream(tokenPath_, std::ios::binary);
		if (stream)
		{
			stream.write(iv.data(), iv.size());
			stream.write(ciphertext.data(), ciphertext.size());
		}
	}
}

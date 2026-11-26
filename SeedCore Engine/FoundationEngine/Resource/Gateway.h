#pragma once

namespace SeedCore
{
	class JoltManager;
	class CriManager;
	class FontManager;
	class EffekseerManager;
	class DlssManager;

	class SEEDCORE_API Gateway :public NonTransferable
	{
	public:
		static void BindJoltManager(JoltManager* manager);

		static void BindCriManager(CriManager* manager);

		static void BindFontManager(FontManager* manager);

		static void BindEffekseerManager(EffekseerManager* manager);

		static void BindDlssManager(DlssManager* manager);

		static JoltManager& GetJoltManager();

		static CriManager& GetCriManager();

		static FontManager& GetFontManager();

		static EffekseerManager& GetEffekseerManager();

		static DlssManager& GetDlssManager();

	private:
		static JoltManager* joltManager_;

		static CriManager* criManager_;

		static FontManager* fontManager_;

		static EffekseerManager* effekseerManager_;

		static DlssManager* dlssManager_;
	};
}

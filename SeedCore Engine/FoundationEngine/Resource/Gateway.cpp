#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	JoltManager* Gateway::joltManager_ = nullptr;
	CriManager* Gateway::criManager_ = nullptr;
	FontManager* Gateway::fontManager_ = nullptr;
	EffekseerManager* Gateway::effekseerManager_ = nullptr;
	DlssManager* Gateway::dlssManager_ = nullptr;

	void Gateway::BindJoltManager(JoltManager* manager)
	{
		joltManager_ = manager;
	}

	void Gateway::BindCriManager(CriManager* manager)
	{
		criManager_ = manager;
	}

	void Gateway::BindFontManager(FontManager* manager)
	{
		fontManager_ = manager;
	}

	void Gateway::BindEffekseerManager(EffekseerManager* manager)
	{
		effekseerManager_ = manager;
	}

	void Gateway::BindDlssManager(DlssManager* manager)
	{
		dlssManager_ = manager;
	}

	JoltManager& Gateway::GetJoltManager()
	{
		return *joltManager_;
	}

	CriManager& Gateway::GetCriManager()
	{
		return *criManager_;
	}

	FontManager& Gateway::GetFontManager()
	{
		return *fontManager_;
	}

	EffekseerManager& Gateway::GetEffekseerManager()
	{
		return *effekseerManager_;
	}

	DlssManager& Gateway::GetDlssManager()
	{
		return *dlssManager_;
	}
}

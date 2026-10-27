#pragma once

namespace SeedCore
{
	struct GameConfig;
	struct EditorConfig;

	/**
	* [EN]
	* Owns and coordinates the CRI ADX runtime.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CRI ADX ランタイムを所有し、連携させる。
	*/
	class SEEDCORE_API CriManager final
	{
	public:
		CriManager();
		~CriManager();

		CriManager(const CriManager&) = delete;
		CriManager& operator=(const CriManager&) = delete;

	public:
		Bool Initialize(const GameConfig& gameConfig);

		Bool Initialize(const EditorConfig& editorConfig);

		void Execute();

		void Finalize();

	public:
		void MasterVolume(Float volume);

		NODISCARD Float MasterVolume()const;

		void DefaultMasterVolume(Float volume);

		NODISCARD Float DefaultMasterVolume()const;

		void CategoryVolume(const String& category, Float volume);

		NODISCARD Float CategoryVolume(const String& category)const;

		NODISCARD const DynamicArray<String>& CategoryNameList()const;

		void ResetVolume();

	public:
		Bool Load();

		Bool Save();

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}

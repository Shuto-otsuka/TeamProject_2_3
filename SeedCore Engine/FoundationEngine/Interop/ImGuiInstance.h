#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Shares one ImGui context across module boundaries. ImGui is linked as
	* a static library, so every module that uses it - Editor.exe,
	* SeedCore.Cplusplus.dll, UserProject.Cplusplus.dll - carries its own
	* copy of ImGui with its own current-context pointer. The editor creates
	* the context; this binds it inside SeedCore.Cplusplus.dll so the engine
	* components' inspector GUI draws into the same context.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モジュールの境界をまたいで、1つの ImGui コンテキストを共有する。
	* ImGui はスタティックライブラリとしてリンクしているため、ImGui を使う
	* モジュール - Editor.exe、SeedCore.Cplusplus.dll、
	* UserProject.Cplusplus.dll - はそれぞれ ImGui の実体と、カレント
	* コンテキストのポインタを別に持つ。コンテキストはエディタが作り、
	* これが SeedCore.Cplusplus.dll の中でそれをバインドすることで、
	* エンジンのコンポーネントのインスペクター GUI が同じコンテキストへ
	* 描画される。
	*/
	class SEEDCORE_API ImGuiInstance
	{
	public:
		/**
		* [EN]
		* Makes context the current ImGui context of SeedCore.Cplusplus.dll's
		* copy of ImGui.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* context を、SeedCore.Cplusplus.dll の ImGui のカレントコンテキストに
		* する。
		*/
		static void Bind(ImGuiContext* context);
	};
}

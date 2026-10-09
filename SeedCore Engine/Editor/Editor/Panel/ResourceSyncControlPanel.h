#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	struct EditorContext;

	/**
	* [EN]
	* The interface side of asset sharing: the pieces of the Editor that
	* show what the team has and offer the actions that reach the shared
	* library. All state lives in ResourceSync; this only draws it and
	* forwards what the member asks for.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット共有の画面側。チームが何を持っているかを表示し、共有
	* ライブラリへ届く操作を提供する部分。状態は全て ResourceSync にあり、
	* ここはそれを描いて、メンバーの要求を渡すだけ。
	*/
	class ResourceSyncControlPanel
	{
	public:
		/**
		* [EN]
		* Draws the one-line state of the shared library at the top of the
		* content browser.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コンテンツブラウザの上部に、共有ライブラリの状態を1行で描く。
		*/
		static void DrawStatus(EditorContext& context);

		/**
		* [EN]
		* Adds the shared-library entries to an asset's context menu.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの右クリックメニューへ、共有ライブラリ用の項目を足す。
		*/
		static void DrawActions(EditorContext& context, const AssetRecord& asset);

		/**
		* [EN]
		* Draws an asset's revision and how this copy stands against the
		* library.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの Revision と、手元の写しがライブラリに対してどういう
		* 状態かを描く。
		*/
		static void DrawState(EditorContext& context, const AssetRecord& asset);
	};
}

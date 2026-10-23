#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	struct EditorContext;

	class Actor;
	class ImGuiTexture;

	/**
	* [EN]
	* The "Add Component" button at the bottom of the Inspector and the
	* popup it opens: a search box and the list of components that can be
	* added to the selected actor. Adding one pushes an undoable command
	* onto the scene history.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* インスペクターの下にある「コンポーネントを追加」ボタンと、それが開く
	* ポップアップ。検索欄と、選択中の actor に追加できるコンポーネントの
	* 一覧を出す。追加するとシーンの履歴に元に戻せるコマンドを積む。
	*/
	class AddComponentPanel
	{
	public:
		/**
		* [EN]
		* Binds the editor context, whose world and scene history the panel
		* adds components through.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタのコンテキストを束縛する。パネルはその World とシーンの
		* 履歴を通してコンポーネントを追加する。
		*/
		AddComponentPanel(EditorContext& context);

		/**
		* [EN]
		* Draws the button centered in the current region and, while it is
		* open, the popup for actor. The popup opens below the button, or
		* above it when there is not enough room below, and its height is
		* capped to the room on that side.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今の領域の中央にボタンを描き、開いている間は actor 用のポップアップ
		* を描く。ポップアップはボタンの下に開き、下に十分な余白が無ければ上に
		* 開く。高さはその側の余白までに抑える。
		*/
		void Draw(Actor actor, ImGuiTexture& imguiTexture);

	private:
		/**
		* [EN]
		* Draws the search box with a magnifier icon inside its left padding,
		* writing what is typed into state_.searchBuffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 左の余白の中に虫眼鏡のアイコンを置いた検索欄を描き、入力された
		* 文字列を state_.searchBuffer に書く。
		*/
		void DrawSearchBar(ImGuiTexture& imguiTexture);

		/**
		* [EN]
		* Draws the list of components that can be added to actor. While the
		* search box is empty, components are grouped by category into nested
		* menus ("/" in a category opens a submenu), with "Custom" placed last
		* below a separator; while searching, the matching components are
		* listed flat in name order. matches_ is rebuilt here every frame, so
		* DrawCategory reads the same filtered list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor に追加できるコンポーネントの一覧を描く。検索欄が空の間は、
		* カテゴリごとに入れ子のメニューにまとめ（カテゴリの "/" でサブメニュー
		* が開く）、"Custom" は区切り線の下の最後に置く。検索中は、一致した
		* コンポーネントを名前順に平らに並べる。matches_ はここで毎フレーム
		* 作り直すので、DrawCategory も同じ絞り込み済みの一覧を読む。
		*/
		void DrawComponent(Actor actor, ImGuiTexture& imguiTexture);

		/**
		* [EN]
		* Draws the inside of the menu for the category path: a submenu for
		* each category one level below it, then the components whose category
		* is path itself. Reads only the range of matches_ that starts with
		* path, found by binary search.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カテゴリ path のメニューの中身を描く。1段下の各カテゴリをサブメニュー
		* にし、その後にカテゴリがちょうど path のコンポーネントを並べる。
		* matches_ のうち path で始まる範囲だけを二分探索で見つけて読む。
		*/
		void DrawCategory(Actor actor, const std::string& path, ImGuiTexture& imguiTexture);

		/**
		* [EN]
		* Draws one component as an icon and a selectable name. A component
		* actor already has is shown disabled; selecting one pushes a
		* ComponentAddCommand onto the scene history, adds it to actor and
		* closes the popup.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コンポーネント1つを、アイコンと選択できる名前で描く。actor が既に
		* 持っているものは無効の表示にする。選ぶと ComponentAddCommand を
		* シーンの履歴に積み、actor に追加して、ポップアップを閉じる。
		*/
		void DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID, ImGuiTexture& imguiTexture);

	private:
		/**
		* [EN]
		* Returns whether componentName is one every actor already has or
		* that the engine manages itself, which the list does not offer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* componentName が、全ての actor が最初から持つものか、エンジンが
		* 自分で管理するもので、一覧に出さないものかどうかを返す。
		*/
		Bool CheckBuiltin(const String& componentName)const;

		/**
		* [EN]
		* Returns whether componentName contains filterText, ignoring the case
		* of ASCII letters; an empty filterText matches everything.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* componentName が filterText を含むかどうかを、ASCII の英字の大文字・
		* 小文字を区別せずに返す。filterText が空なら全てに一致する。
		*/
		Bool CheckFilter(const String& componentName, const std::string& filterText)const;

	private:
		/**
		* [EN]
		* What the panel keeps between frames while the popup is open.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ポップアップを開いている間、パネルがフレームをまたいで持つもの。
		*/
		struct State
		{
			/// [EN] The text in the search box; cleared each time the popup is opened.
			/// [JP] 検索欄の文字列。ポップアップを開くたびに空にする。
			String searchBuffer;
		};

		/// [EN] The editor context: the world the components are added to and the scene history the commands go onto.
		/// [JP] エディタのコンテキスト。コンポーネントを追加する World と、コマンドを積むシーンの履歴を持つ。
		EditorContext& context_;

		/// [EN] The state kept between frames.
		/// [JP] フレームをまたいで持つ状態。
		State state_;

		/// [EN] The components that pass the search, with their names; rebuilt every frame by DrawComponent and sorted by category then name (name alone while searching), so DrawCategory can binary-search a category's range. Kept as a member so its capacity is reused across frames.
		/// [JP] 検索を通ったコンポーネントとその名前。DrawComponent が毎フレーム作り直し、カテゴリ → 名前（検索中は名前だけ）の順に並べるので、DrawCategory がカテゴリの範囲を二分探索できる。メンバーに持つのは、確保済みの容量をフレームをまたいで使い回すため。
		DynamicArray<std::pair<String, ComponentID>> matches_;
	};
}

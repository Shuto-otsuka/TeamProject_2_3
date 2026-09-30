#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/File/FilePath.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Utility/ArtMap.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture;

	enum class ExportPreset;
	enum class MeshCollisionDetail;

	/**
	* [EN]
	* The Editor's asset browser. A folder tree on the left and the selected
	* folder's contents on the right (as a list or a grid), with a search box
	* that switches to a flat list of matching assets. Folders and assets can
	* be created, cut/copied/pasted, renamed and deleted in place, models get
	* asset actions (collision/material/skeleton generation, export), assets
	* are dragged from here into other panels, and an actor dropped from the
	* Hierarchy is saved as a Prefab into the selected folder. The tree is
	* rebuilt whenever the ResourceCache or the shared library advances its
	* revision.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタのアセットブラウザ。左にフォルダのツリー、右に選択中のフォルダの
	* 中身（リストかグリッド）を出し、検索欄に入力すると一致するアセットの
	* 一覧に切り替わる。フォルダとアセットはその場で作成・切り取り/コピー/
	* 貼り付け・名前変更・削除ができ、モデルにはアセットアクション（コリジョン/
	* マテリアル/スケルトンの生成、エクスポート）がある。アセットはここから
	* 他のパネルへドラッグでき、Hierarchy からドロップしたアクターは選択中の
	* フォルダへ Prefab として保存される。ツリーは ResourceCache か共有
	* ライブラリの revision が進むたびに作り直す。
	*/
	class ContentsDrawerPanel
	{
	public:
		/**
		* [EN]
		* Binds the panel to the Editor context and icon set, and builds the
		* folder tree once so the first frame has something to show.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パネルをエディタのコンテキストとアイコンに結び付け、最初のフレームで
		* 表示できるようにフォルダのツリーを一度作る。
		*/
		ContentsDrawerPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ContentsDrawerPanel() = default;

		/**
		* [EN]
		* Drives the ResourceCache folder watch, rebuilds the tree when the
		* asset list has changed, then draws the whole panel.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResourceCache のフォルダ監視を進め、アセット一覧が変わっていれば
		* ツリーを作り直してから、パネル全体を描く。
		*/
		void Draw();

	private:
		struct FolderNode;
		enum class ScriptType;

		/**
		* [EN]
		* Draws one folder of the left-hand tree and, when it is open, its
		* child folders recursively.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 左のツリーのフォルダを 1 つ描き、開いていれば子フォルダを再帰的に描く。
		*/
		void DrawDirectoryTree(FolderNode& node);

		/**
		* [EN]
		* Draws the given folder's child folders and assets as rows of a
		* small icon followed by the name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定したフォルダの子フォルダとアセットを、小さなアイコンと名前の行として描く。
		*/
		void DrawAssetListMode(FolderNode& folder);

		/**
		* [EN]
		* Draws the given folder's child folders and assets as a grid of
		* icon buttons sized by gridIconSize_, with the name under each.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定したフォルダの子フォルダとアセットを、gridIconSize_ の大きさの
		* アイコンボタンの格子として描き、それぞれの下に名前を出す。
		*/
		void DrawAssetGridMode(FolderNode& folder);

		/**
		* [EN]
		* Draws the hover tooltip for an asset: its icon (a large preview for
		* a loaded texture), path, ID, sharing state, file size and, for a
		* texture, its pixel size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットにカーソルを重ねたときのツールチップを描く。アイコン（読み込み
		* 済みのテクスチャは大きなプレビュー）、パス、ID、共有の状態、ファイル
		* サイズ、テクスチャならピクセルサイズを出す。
		*/
		void DrawAssetTooltip(const AssetRecord& asset);

		/**
		* [EN]
		* Draws the right-click menu of a folder (new folder/script,
		* cut/copy/paste, rename, delete, open in Explorer). Attached to the
		* item drawn just before it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フォルダの右クリックメニュー（新規フォルダ/スクリプト、切り取り/コピー/
		* 貼り付け、名前変更、削除、エクスプローラーで開く）を描く。直前に描いた
		* 項目に付く。
		*/
		void DrawFolderMenu(const FilePath& folderPath);

		/**
		* [EN]
		* Draws the right-click menu of an asset (sharing actions, open, model
		* asset actions, cut/copy, rename, delete, show in Explorer). Attached
		* to the item drawn just before it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの右クリックメニュー（共有の操作、開く、モデルのアセット
		* アクション、切り取り/コピー、名前変更、削除、エクスプローラーで表示）
		* を描く。直前に描いた項目に付く。
		*/
		void DrawAssetMenu(const AssetRecord& asset, const FilePath& assetPath);

		/**
		* [EN]
		* Draws the right-click menu for empty space in the current child
		* window, acting on the selected folder. The tree and the contents
		* area share it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今の子ウィンドウの何も無いところを右クリックしたときのメニューを描き、
		* 選択中のフォルダを対象にする。ツリーと中身の領域で共通。
		*/
		void DrawBackgroundMenu();

		/**
		* [EN]
		* Draws the modal dialog that asks for a new script's name, opening it
		* when OpenScriptPopup() has requested it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいスクリプトの名前を入力するモーダルダイアログを描く。
		* OpenScriptPopup() が要求していれば開く。
		*/
		void DrawScriptMenu();

		/**
		* [EN]
		* Draws the inline name field in place of the item's label while that
		* item is being renamed. Returns whether it drew the field, so the
		* caller draws the normal label only when it returns false.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 名前変更中の項目では、ラベルの代わりにその場の入力欄を描く。入力欄を
		* 描いたかどうかを返すので、呼び出し側は false のときだけ普段のラベルを描く。
		*/
		Bool DrawRenameMenu(const FilePath& itemPath);

	private:
		/**
		* [EN]
		* Moves to a folder, dropping any forward history and appending the
		* folder as the newest entry. Selecting the current folder again does
		* nothing, so repeated clicks do not fill the history.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フォルダへ移動する。進む側の履歴を捨て、そのフォルダを最新の履歴として
		* 足す。今のフォルダをもう一度選んでも何もしないので、何度クリックしても
		* 履歴は増えない。
		*/
		void SelectDirectory(const FilePath& directoryPath);

		/**
		* [EN]
		* Returns the selected folder, which is always the history entry the
		* back/forward index points at.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 選択中のフォルダを返す。常に、戻る/進むの位置が指している履歴の要素。
		*/
		const FilePath& SelectedDirectory()const;

		/**
		* [EN]
		* Rebuilds the folder tree from the directories on disk and from the
		* asset list (local assets plus remote-only ones from the shared
		* library), so every asset lands in the folder of its path.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ディスク上のフォルダとアセット一覧（ローカルのアセットと、共有
		* ライブラリにしか無いアセット）からフォルダのツリーを作り直し、各アセットを
		* そのパスのフォルダへ入れる。
		*/
		void BuildDirectory();

	private:
		/**
		* [EN]
		* Returns the icon that stands for an asset type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの種類を表すアイコンを返す。
		*/
		ImTextureID GetAssetIcon(AssetType type)const;

		/**
		* [EN]
		* Returns the icon for an asset: a cached thumbnail for a loaded
		* texture, a source-file icon for .h/.cpp/.hlsl(i), otherwise the icon
		* of its type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットのアイコンを返す。読み込み済みのテクスチャはキャッシュした
		* サムネイル、.h/.cpp/.hlsl(i) はソースファイルのアイコン、それ以外は
		* 種類のアイコン。
		*/
		ImTextureID GetAssetIcon(const AssetRecord& asset)const;

		/**
		* [EN]
		* Returns the badge showing an asset's sharing state, or 0 when the
		* asset is not in the shared library.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの共有の状態を示すバッジを返す。共有ライブラリに無いアセットでは 0。
		*/
		ImTextureID GetSharingIcon(const AssetRecord& asset)const;

		/**
		* [EN]
		* Returns the folder icon, filled or empty depending on whether the
		* folder holds anything.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フォルダのアイコンを返す。中に何かあるかどうかで、中身ありと空を使い分ける。
		*/
		ImTextureID GetFolderIcon(const FolderNode& node)const;

	private:
		/**
		* [EN]
		* Returns the drag-and-drop payload name for an asset type; the
		* receiving panels accept a drop by this name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの種類に対応するドラッグ&ドロップのペイロード名を返す。受け取る側の
		* パネルはこの名前でドロップを受け付ける。
		*/
		const Char* GetPayloadType(AssetType type)const;

	private:
		/**
		* [EN]
		* Bakes a ".collision" file next to a model at the given detail level
		* and reloads the ResourceCache so it appears as an asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定した精度でモデルの隣に ".collision" ファイルを焼き、アセットとして
		* 出てくるよう ResourceCache を読み直す。
		*/
		void GenerateMeshCollision(const AssetRecord& asset, MeshCollisionDetail detail);

		/**
		* [EN]
		* Writes a ".material" file for each of a model's materials and
		* reloads the ResourceCache so they appear as assets.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルのマテリアルごとに ".material" ファイルを書き出し、アセットとして
		* 出てくるよう ResourceCache を読み直す。
		*/
		void GenerateMaterial(const AssetRecord& asset);

		/**
		* [EN]
		* Writes a ".skeleton" file from a skinned model and reloads the
		* ResourceCache so it appears as an asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スキン付きモデルから ".skeleton" ファイルを書き出し、アセットとして
		* 出てくるよう ResourceCache を読み直す。
		*/
		void GenerateSkeleton(const AssetRecord& asset);

	private:
		/**
		* [EN]
		* Deletes a file or folder together with its ".meta", and clears the
		* clipboard if it held that item. Shared content is refused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ファイルかフォルダを ".meta" ごと削除し、それがクリップボードに入って
		* いればクリップボードを空にする。共有コンテンツは断る。
		*/
		void ExecuteDelete(const FilePath& targetPath);

		/**
		* [EN]
		* Renames a file or folder together with its ".meta" to the name held
		* in targetPath's filename text, and keeps the clipboard pointing at
		* it. Shared content is refused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ファイルかフォルダを ".meta" ごと、targetPath のファイル名の文字列に
		* 入っている名前へ変え、クリップボードがそれを指し続けるようにする。
		* 共有コンテンツは断る。
		*/
		void ExecuteRename(const FilePath& targetPath);

		/**
		* [EN]
		* Pastes the clipboard item into a folder: a cut moves it (with its
		* ".meta") and empties the clipboard, a copy duplicates it. Shared
		* content is refused.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クリップボードの項目をフォルダへ貼り付ける。切り取りなら（".meta" ごと）
		* 移動してクリップボードを空にし、コピーなら複製する。共有コンテンツは断る。
		*/
		void ExecutePaste(const FilePath& directoryPath);

		/**
		* [EN]
		* Adds a new script's header and source to UserProject.Cplusplus,
		* through Visual Studio when it has the solution open, otherwise by
		* running CreateScript.py. Returns whether the files were added. Both
		* paths are relative to UserProject.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいスクリプトのヘッダとソースを UserProject.Cplusplus に追加する。
		* Visual Studio がソリューションを開いていればそちら経由で、開いていなければ
		* CreateScript.py を実行して追加する。追加できたかどうかを返す。どちらの
		* パスも UserProject を基準にする。
		*/
		Bool ExecuteRegister(const FilePath& headerPath, const FilePath& cppPath);

	private:
		/**
		* [EN]
		* Creates a folder at targetPath, numbering the name "(2)", "(3)", ...
		* while it is taken, and starts renaming the new folder at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* targetPath にフォルダを作る。名前が使われていれば "(2)"、"(3)"… と番号を
		* 付け、作ったフォルダの名前変更をすぐに始める。
		*/
		void CreateNewFolder(const FilePath& targetPath);

		/**
		* [EN]
		* Creates a C++ (header and source, registered in the project) or C#
		* script from targetPath's stem. The name is reduced to a valid
		* identifier and numbered while taken, and the file lands in its
		* folder when that is inside UserProject, otherwise in
		* UserProject/Script. The new files open in the Visual Studio that has
		* Runtime.sln open, or through their shell association when none does.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* targetPath の拡張子を除いた名前から、C++（ヘッダとソース。プロジェクトに
		* 登録する）か C# のスクリプトを作る。名前は有効な識別子に削り、使われて
		* いれば番号を付ける。置き場所は、そのフォルダが UserProject の中なら
		* そのフォルダ、外なら UserProject/Script。作ったファイルは、Runtime.sln を
		* 開いている Visual Studio で開き、無ければファイルの関連付けで開く。
		*/
		void CreateNewScript(const FilePath& targetPath);

	private:
		/**
		* [EN]
		* Opens an asset the way its type calls for: a remote-only asset is
		* requested from the shared library, a scene is loaded, a material
		* opens the material viewer, anything else opens in its associated
		* application.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットを種類に応じて開く。共有ライブラリにしか無いアセットは取得を
		* 依頼し、シーンは読み込み、マテリアルはマテリアルビューアを開き、それ
		* 以外は関連付けられたアプリケーションで開く。
		*/
		void OpenAssetPopup(const AssetRecord& asset);

		/**
		* [EN]
		* Requests the script-name dialog for a new script of the given
		* language in parentPath; DrawScriptMenu() opens it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* parentPath に指定した言語の新しいスクリプトを作るため、名前入力の
		* ダイアログを要求する。開くのは DrawScriptMenu()。
		*/
		void OpenScriptPopup(const FilePath& parentPath, ScriptType script);

	private:
		/**
		* [EN]
		* Asks for a destination with a save dialog, exports a model there in
		* the given preset's format, and reloads the ResourceCache so an
		* export inside the project appears as an asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 保存ダイアログで出力先を尋ね、指定したプリセットの形式でモデルを書き
		* 出す。プロジェクト内へ書き出した場合にアセットとして出てくるよう、
		* ResourceCache を読み直す。
		*/
		void ExportModel(const AssetRecord& asset, ExportPreset preset, const Wchar* extension);

	private:
		/**
		* [EN]
		* How the selected folder's contents are laid out.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 選択中のフォルダの中身の並べ方。
		*/
		enum class ViewType
		{
			/// [EN] Rows of a small icon followed by the name.
			/// [JP] 小さなアイコンと名前の行。
			List,

			/// [EN] A grid of icon buttons with the name under each.
			/// [JP] 名前を下に付けたアイコンボタンの格子。
			Grid,
		};

		/**
		* [EN]
		* The language of a script created from the panel.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パネルから作るスクリプトの言語。
		*/
		enum class ScriptType
		{
			/// [EN] A header and source registered in UserProject.Cplusplus.
			/// [JP] UserProject.Cplusplus に登録するヘッダとソース。
			Cpp,

			/// [EN] A single .cs file.
			/// [JP] .cs ファイル 1 つ。
			Csharp,
		};

		/**
		* [EN]
		* What the clipboard holds.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クリップボードに入っているもの。
		*/
		enum class ClipboardType
		{
			/// [EN] Nothing to paste.
			/// [JP] 貼り付けるものが無い。
			None,

			/// [EN] An item to be moved on paste.
			/// [JP] 貼り付け時に移動する項目。
			Cut,

			/// [EN] An item to be duplicated on paste.
			/// [JP] 貼り付け時に複製する項目。
			Copy,
		};

	private:
		/**
		* [EN]
		* One folder of the tree, rebuilt by BuildDirectory().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ツリーのフォルダ 1 つ。BuildDirectory() が作り直す。
		*/
		struct FolderNode
		{
			/// [EN] The folder's path, rooted at the project root.
			/// [JP] フォルダのパス。基準はプロジェクトルート。
			FilePath path_;

			/// [EN] Child folders, keyed by folder name so they draw in name order.
			/// [JP] 子フォルダ。フォルダ名をキーにするので名前順に描かれる。
			ArtMap<std::string, FolderNode> children_;

			/// [EN] Assets directly in this folder, pointing into assetList_.
			/// [JP] このフォルダ直下のアセット。assetList_ の要素を指す。
			DynamicArray<const AssetRecord*> assets_;
		};

	private:
		/**
		* [EN]
		* State of the new-script dialog between the request and the frame
		* that draws it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新規スクリプトのダイアログの状態。要求してから、それを描くフレームまで持つ。
		*/
		struct ScriptMenuState
		{
			/// [EN] Open the dialog on the next DrawScriptMenu().
			/// [JP] 次の DrawScriptMenu() でダイアログを開く。
			Bool openRequested_ = false;

			/// [EN] Put keyboard focus on the name field the next time it is drawn.
			/// [JP] 次に名前の入力欄を描くときにキーボードのフォーカスを置く。
			Bool focusRequested_ = false;

			/// [EN] Language of the script to create.
			/// [JP] 作るスクリプトの言語。
			ScriptType scriptType_ = ScriptType::Cpp;

			/// [EN] Path of the script to create; its filename text is the name field's buffer.
			/// [JP] 作るスクリプトのパス。ファイル名の文字列が名前の入力欄のバッファになる。
			std::optional<FilePath> targetPath_;
		};

		/**
		* [EN]
		* State of the inline rename field.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* その場の名前変更の入力欄の状態。
		*/
		struct RenameMenuState
		{
			/// [EN] Put keyboard focus on the field the next time it is drawn.
			/// [JP] 次に入力欄を描くときにキーボードのフォーカスを置く。
			Bool focusRequested_ = false;

			/// [EN] Item being renamed, empty when none is; its filename text is the field's buffer.
			/// [JP] 名前変更中の項目。無ければ空。ファイル名の文字列が入力欄のバッファになる。
			std::optional<FilePath> targetPath_;
		};

		/**
		* [EN]
		* The item on the panel's clipboard.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パネルのクリップボードに入っている項目。
		*/
		struct ClipboardState
		{
			/// [EN] Whether the item is to be moved or duplicated.
			/// [JP] その項目を移動するか複製するか。
			ClipboardType type_ = ClipboardType::None;

			/// [EN] The cut or copied file or folder.
			/// [JP] 切り取ったかコピーしたファイルかフォルダ。
			FilePath path_;
		};

		/**
		* [EN]
		* Folder navigation history for the mouse back/forward buttons.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マウスの戻る/進むボタン用の、フォルダの移動履歴。
		*/
		struct HistoryState
		{
			/// [EN] Visited folders, oldest first; never empty once BuildDirectory() has run.
			/// [JP] 訪れたフォルダ。古い順。BuildDirectory() の後は空にならない。
			DynamicArray<FilePath> directoryList_;

			/// [EN] Position of the selected folder in directoryList_.
			/// [JP] directoryList_ の中での選択中のフォルダの位置。
			Size index_ = 0;
		};

	private:
		/// [EN] Editor-wide context: the World's ResourceCache, graphics and the shared library.
		/// [JP] エディタ全体のコンテキスト。World の ResourceCache、グラフィックス、共有ライブラリ。
		EditorContext& context_;

		/// [EN] Editor icon set.
		/// [JP] エディタのアイコン。
		ImGuiTexture& imguiTexture_;

		/// [EN] Search box; while it holds a filter, the panel lists matching assets instead of the tree.
		/// [JP] 検索欄。条件が入っている間は、ツリーの代わりに一致するアセットを一覧で出す。
		ImGuiTextFilter searchFilter_;

		/// [EN] Layout of the selected folder's contents.
		/// [JP] 選択中のフォルダの中身の並べ方。
		ViewType viewType_ = ViewType::Grid;

		/// [EN] Root of the folder tree, the project root itself.
		/// [JP] フォルダのツリーの根。プロジェクトルートそのもの。
		FolderNode root_;

		/// [EN] Copies of every asset shown, local and remote-only; FolderNode::assets_ points into it.
		/// [JP] 表示する全アセットの写し。ローカルと共有ライブラリにしか無いもの。FolderNode::assets_ はここを指す。
		DynamicArray<AssetRecord> assetList_;

		/// [EN] New-script dialog state.
		/// [JP] 新規スクリプトのダイアログの状態。
		ScriptMenuState scriptMenuState_;

		/// [EN] Inline rename state.
		/// [JP] その場の名前変更の状態。
		RenameMenuState renameMenuState_;

		/// [EN] Clipboard for cut/copy/paste.
		/// [JP] 切り取り/コピー/貼り付けのクリップボード。
		ClipboardState clipboardState_;

		/// [EN] Folder navigation history.
		/// [JP] フォルダの移動履歴。
		HistoryState historyState_;

		/// [EN] Shared library revision the tree was last built from.
		/// [JP] ツリーを最後に作ったときの共有ライブラリの revision。
		Uint64 syncRevision_ = 0;

		/// [EN] ResourceCache revision the tree was last built from.
		/// [JP] ツリーを最後に作ったときの ResourceCache の revision。
		Uint64 resourceRevision_ = 0;

		/// [EN] Icon size in grid view, in pixels.
		/// [JP] グリッド表示のアイコンの大きさ（ピクセル）。
		Float gridIconSize_ = 64.0f;

		/// [EN] Thumbnail views of loaded textures, keyed by asset ID; mutable because GetAssetIcon() fills it on first use.
		/// [JP] 読み込み済みのテクスチャのサムネイル用ビュー。アセット ID がキー。GetAssetIcon() が初回に埋めるので mutable。
		mutable FlatMap<Uint32, ImTextureID> thumbnailCache_;
	};
}

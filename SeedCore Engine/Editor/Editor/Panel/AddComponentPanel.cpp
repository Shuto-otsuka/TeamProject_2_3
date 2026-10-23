#include <Editor/Editor/Panel/AddComponentPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Command/ComponentLifecycleCommand.h>

namespace SeedCore
{
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
	AddComponentPanel::AddComponentPanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

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
	void AddComponentPanel::Draw(Actor actor, ImGuiTexture& imguiTexture)
	{
		/// [EN] Centers the button horizontally in the space left in the current region.
		/// [JP] 今の領域に残っている幅の中で、ボタンを横方向の中央に置く。
		Float availableWidth = ImGui::GetContentRegionAvail().x;
		Float buttonWidth = 200.0f;
		ImGui::SetCursorPosX((availableWidth - buttonWidth) * 0.5f);

		/// [EN] Opening the popup starts with an empty search, so the full list shows first.
		/// [JP] ポップアップを開くときは検索を空から始め、最初は一覧の全部を出す。
		if (ImGui::Button("コンポーネントを追加", ImVec2(buttonWidth, 0)))
		{
			state_.searchBuffer = String("");
			ImGui::OpenPopup("AddComponentPopup");
		}

		/// [EN] The button's rectangle, which the popup is anchored to.
		/// [JP] ボタンの矩形。ポップアップはここを基準に置く。
		ImVec2 buttonMin = ImGui::GetItemRectMin();
		ImVec2 buttonMax = ImGui::GetItemRectMax();
		Float buttonCenterX = (buttonMin.x + buttonMax.x) * 0.5f;

		/// [EN] The room between the button and the bottom and top of the main viewport's work area.
		/// [JP] ボタンから、メインビューポートの作業領域の下端と上端までの余白。
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		Float spaceBelow = (viewport->WorkPos.y + viewport->WorkSize.y) - buttonMax.y;
		Float spaceAbove = buttonMin.y - viewport->WorkPos.y;

		/// [EN] The popup prefers to open downward; it opens upward only when the room below is both under this height and smaller than the room above.
		/// [JP] ポップアップは下向きに開くのを優先する。上向きに開くのは、下の余白がこの高さより小さく、かつ上の余白より小さいときだけ。
		constexpr Float minReasonableSpace = 150.0f;

		/// [EN] The gap kept between the popup and the edge of the viewport.
		/// [JP] ポップアップとビューポートの端の間に空けておく隙間。
		constexpr Float viewportEdgeMargin = 16.0f;
		Bool opensDownward = spaceBelow >= minReasonableSpace || spaceBelow >= spaceAbove;

		/// [EN] Capping the height to the room on the chosen side makes a long list scroll inside the popup instead of running off the screen.
		/// [JP] 高さを開く側の余白までに抑えるので、長い一覧は画面の外へはみ出さず、ポップアップの中でスクロールする。
		Float maxPopupHeight = (opensDownward ? spaceBelow : spaceAbove) - viewportEdgeMargin;
		ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxPopupHeight));

		/// [EN] The pivot puts the popup's top-center on the button's bottom edge when opening downward, and its bottom-center on the button's top edge when opening upward.
		/// [JP] ピボットにより、下向きならポップアップの上辺の中央をボタンの下辺に、上向きならポップアップの下辺の中央をボタンの上辺に合わせる。
		if (opensDownward)
		{
			ImGui::SetNextWindowPos(ImVec2(buttonCenterX, buttonMax.y), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
		}
		else
		{
			ImGui::SetNextWindowPos(ImVec2(buttonCenterX, buttonMin.y), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
		}

		/// [EN] The popup holds the search box above the component list.
		/// [JP] ポップアップの中は、検索欄とその下のコンポーネントの一覧。
		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			ImGui::SetNextItemWidth(240.0f);
			DrawSearchBar(imguiTexture);
			ImGui::Separator();
			DrawComponent(actor, imguiTexture);
			ImGui::EndPopup();
		}
	}

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
	void AddComponentPanel::DrawSearchBar(ImGuiTexture& imguiTexture)
	{
		/// [EN] InputText edits a fixed-size buffer in place, so the current text is copied into one of 256 bytes.
		/// [JP] InputText は固定長のバッファをその場で書き換えるので、今の文字列を 256 バイトのバッファへ写す。
		std::string searchText = state_.searchBuffer.str();
		searchText.resize(256);

		/// [EN] The left padding is widened by one icon plus the original padding on both sides of it, leaving room for the icon drawn afterwards.
		/// [JP] 左の余白を、アイコン1つ分とその両側の元の余白の分だけ広げ、後で描くアイコンの場所を空ける。
		Float iconSize = ImGui::GetTextLineHeight();
		Float originalPaddingX = ImGui::GetStyle().FramePadding.x;
		Float iconPadding = iconSize + originalPaddingX * 2.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(iconPadding, ImGui::GetStyle().FramePadding.y));

		/// [EN] Only an edit writes back; the buffer is read up to its terminator, since the padding bytes after it are zeros.
		/// [JP] 書き換えられたときだけ書き戻す。バッファは終端まで読む。その後ろの埋めたバイトは 0 のため。
		if (ImGui::InputTextWithHint("##Search", "検索...", searchText.data(), searchText.capacity()))
		{
			state_.searchBuffer = String(std::string_view(searchText.c_str()));
		}

		/// [EN] The icon sits in the widened padding, centered vertically on the box.
		/// [JP] アイコンは広げた余白の中に、欄の縦方向の中央に置く。
		ImGui::PopStyleVar();
		ImVec2 inputMin = ImGui::GetItemRectMin();
		Float inputHeight = ImGui::GetItemRectSize().y;
		Float iconY = inputMin.y + (inputHeight - iconSize) * 0.5f;
		ImGui::GetWindowDrawList()->AddImage(imguiTexture.Icon(IconType::Search), ImVec2(inputMin.x + originalPaddingX, iconY), ImVec2(inputMin.x + originalPaddingX + iconSize, iconY + iconSize));
	}

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
	void AddComponentPanel::DrawComponent(Actor actor, ImGuiTexture& imguiTexture)
	{
		/// [EN] The search text, copied once per frame; an empty text means no search.
		/// [JP] 検索文字列。フレームごとに1回だけ写す。空なら検索していない。
		std::string filterText = state_.searchBuffer.str();
		Bool searching = !filterText.empty();

		/// [EN] clear() keeps the capacity, so after the first frame collecting the matches allocates nothing.
		/// [JP] clear() は確保済みの容量を残すので、2フレーム目以降は一致したものを集めても確保が起きない。
		matches_.clear();
		for (ComponentID componentID : ComponentRegistry::Registry())
		{
			const String& componentName = componentID->name_;
			if (CheckBuiltin(componentName) || !CheckFilter(componentName, filterText))
			{
				continue;
			}
			matches_.emplace_back(componentName, componentID);
		}

		if (matches_.empty())
		{
			ImGui::TextDisabled("該当なし");
			return;
		}

		/// [EN] Category then name outside a search, name alone while searching. Sorting by category puts every category that starts with the same path next to each other, which lets DrawCategory reach its range by binary search. Strings are compared as views onto the interned data, so no comparison allocates.
		/// [JP] 検索していなければカテゴリ → 名前、検索中は名前だけの順。カテゴリで並べると、同じパスで始まるカテゴリが必ず隣り合うので、DrawCategory が二分探索で自分の範囲に届く。文字列はインターン済みのデータへの view として比べるので、比較で確保は起きない。
		std::ranges::sort(matches_, [searching](const auto& a, const auto& b)
			{
				if (!searching)
				{
					std::string_view categoryA = static_cast<std::string_view>(a.second->category_);
					std::string_view categoryB = static_cast<std::string_view>(b.second->category_);
					if (categoryA != categoryB)
					{
						return categoryA < categoryB;
					}
				}
				return static_cast<std::string_view>(a.first) < static_cast<std::string_view>(b.first);
			});

		/// [EN] While searching, categories are ignored and the matches are listed flat.
		/// [JP] 検索中はカテゴリを無視し、一致したものを平らに並べる。
		if (searching)
		{
			for (auto& [componentName, componentID] : matches_)
			{
				DrawMenuItem(actor, componentName, componentID, imguiTexture);
			}
			return;
		}

		/// [EN] Every component registered from outside the engine sits under this category, shown last below a separator.
		/// [JP] エンジンの外で登録した全てのコンポーネントが入るカテゴリ。区切り線の下の最後に出す。
		static const std::string customCategory = "Custom";

		/// [EN] The top level of each category (the part before the first "/"), in sorted order without duplicates; views onto matches_, so collecting them copies no string. The roots are only a few, so a linear contains is faster than a set.
		/// [JP] 各カテゴリの一番上の段（最初の "/" より前）。並べ替えた順で、重複は除く。matches_ を指す view なので、集めても文字列はコピーしない。一番上の段は数個なので、set より線形の contains の方が速い。
		DynamicArray<std::string_view> roots;
		for (auto& [componentName, componentID] : matches_)
		{
			std::string_view category = static_cast<std::string_view>(componentID->category_);
			std::string_view root = category.substr(0, category.find('/'));
			if (!std::ranges::contains(roots, root))
			{
				roots.push_back(root);
			}
		}

		for (std::string_view root : roots)
		{
			if (root == customCategory)
			{
				continue;
			}

			/// [EN] BeginMenu needs a null-terminated label, so only the root being drawn is copied into a string.
			/// [JP] BeginMenu は終端付きの文字列を要るので、描くルートだけを文字列に写す。
			std::string rootPath = std::string(root);
			if (ImGui::BeginMenu(rootPath.c_str()))
			{
				DrawCategory(actor, rootPath, imguiTexture);
				ImGui::EndMenu();
			}
		}

		if (std::ranges::contains(roots, std::string_view(customCategory)))
		{
			ImGui::Separator();
			if (ImGui::BeginMenu(customCategory.c_str()))
			{
				DrawCategory(actor, customCategory, imguiTexture);
				ImGui::EndMenu();
			}
		}
	}

	/**
	* [EN]
	* Draws the inside of the menu for the category path: a submenu for
	* each category one level below it, then the components whose category
	* is path itself. Reads only the range of matches_ that starts with
	* path, found by binary search, so an open menu costs O(log N + its
	* own entries) rather than a pass over every component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カテゴリ path のメニューの中身を描く。1段下の各カテゴリをサブメニュー
	* にし、その後にカテゴリがちょうど path のコンポーネントを並べる。
	* matches_ のうち path で始まる範囲だけを二分探索で見つけて読むので、
	* 開いたメニュー1つの費用は全コンポーネントを回る分ではなく、
	* O(log N + 自分の範囲の数) で済む。
	*/
	void AddComponentPanel::DrawCategory(Actor actor, const std::string& path, ImGuiTexture& imguiTexture)
	{
		/// [EN] matches_ is sorted by category, and strings sharing a prefix are always contiguous in that order, so path and everything under it run from the first category not less than path to the first one that does not start with path.
		/// [JP] matches_ はカテゴリで並んでおり、その順では同じ文字列で始まるものが必ず続けて並ぶ。なので path とその下の全ては、path 以上の最初のカテゴリから、path で始まらなくなる最初のカテゴリの手前まで。
		auto first = std::ranges::lower_bound(matches_, std::string_view(path), {}, [](const auto& match) { return static_cast<std::string_view>(match.second->category_); });
		auto last = std::find_if(first, matches_.end(), [&](const auto& match) { return !static_cast<std::string_view>(match.second->category_).starts_with(path); });

		/// [EN] The names one level below path, without duplicates, as views onto matches_. The range also holds categories that merely start with the same characters (e.g. "Graphics2" under "Graphics"), which are skipped by requiring a "/" right after path.
		/// [JP] path の1段下の名前。重複は除き、matches_ を指す view で持つ。範囲には同じ文字で始まるだけのカテゴリ（"Graphics" に対する "Graphics2" など）も入るので、path の直後が "/" のものだけを取る。
		DynamicArray<std::string_view> children;
		for (auto it = first; it != last; ++it)
		{
			std::string_view category = static_cast<std::string_view>(it->second->category_);
			if (category.size() <= path.size() || category[path.size()] != '/')
			{
				continue;
			}
			std::string_view rest = category.substr(path.size() + 1);
			std::string_view child = rest.substr(0, rest.find('/'));
			if (!std::ranges::contains(children, child))
			{
				children.push_back(child);
			}
		}

		/// [EN] Submenus first; a child's full path is built only when its menu is actually open.
		/// [JP] 先にサブメニュー。子のパスは、そのメニューが実際に開いているときだけ作る。
		for (std::string_view child : children)
		{
			std::string childName = std::string(child);
			if (ImGui::BeginMenu(childName.c_str()))
			{
				DrawCategory(actor, path + "/" + childName, imguiTexture);
				ImGui::EndMenu();
			}
		}

		/// [EN] Then the components of this category itself, already in name order from the sort.
		/// [JP] 次に、このカテゴリそのもののコンポーネント。並べ替えで既に名前順になっている。
		for (auto it = first; it != last; ++it)
		{
			if (static_cast<std::string_view>(it->second->category_) == path)
			{
				DrawMenuItem(actor, it->first, it->second, imguiTexture);
			}
		}
	}

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
	void AddComponentPanel::DrawMenuItem(Actor actor, const String& componentName, ComponentID componentID, ImGuiTexture& imguiTexture)
	{
		/// [EN] A component can be added only once, so one actor already has stays in the list but cannot be picked.
		/// [JP] コンポーネントは1つしか付けられないので、既に持っているものは一覧に残したまま選べなくする。
		Bool alreadyAttached = actor.HasComponent(componentID);

		if (alreadyAttached)
		{
			ImGui::BeginDisabled();
		}

		/// [EN] The component's icon, sized to the text line, then its name on the same line.
		/// [JP] コンポーネントのアイコンを文字の行の高さで描き、同じ行に名前を続ける。
		Float iconSize = ImGui::GetTextLineHeight();
		ImGui::Image(imguiTexture.Icon(ImGuiTexture::ComponentIconType(componentName)), ImVec2(iconSize, iconSize));
		ImGui::SameLine();

		/// [EN] NoAutoClosePopups keeps ImGui from closing the popup on its own, so it closes here only after the component is added. The command is pushed before adding, recording the actor by its persistent ID so undo still finds it.
		/// [JP] NoAutoClosePopups で ImGui が勝手にポップアップを閉じないようにし、コンポーネントを追加した後にここで閉じる。コマンドは追加より前に積み、actor を永続 ID で記録しておくので、元に戻すときにも見つけられる。
		if (ImGui::Selectable(componentName.c_str(), false, ImGuiSelectableFlags_NoAutoClosePopups))
		{
			context_.scene_.history_.Push(MakePtr<ComponentAddCommand>(*context_.world_.world_, WorldAccessor::Actors(*context_.world_.world_).Record(ActorAccessor::GetEntity(actor).GetID())->persistentID_, componentID));
			actor.AddComponent(componentID);
			ImGui::CloseCurrentPopup();
		}

		if (alreadyAttached)
		{
			ImGui::EndDisabled();
		}
	}

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
	Bool AddComponentPanel::CheckBuiltin(const String& componentName)const
	{
		/// [EN] The components every actor is created with (Name through Bounds), the velocities Rigidbody adds and removes itself, and Material and Skeleton, which are added automatically once a mesh is set (Skeleton only for a skinned mesh). Interned once, so each check compares addresses only.
		/// [JP] 全ての actor が作られたときから持つもの（Name から Bounds まで）、Rigidbody が自分で付け外しする速度、メッシュを設定すると自動で付く Material と Skeleton（Skeleton はスキンのあるメッシュだけ）。1回だけインターンするので、確認はアドレスの比較だけで済む。
		static const String builtinComponents[] =
		{
			String("Name"),
			String("Position"),
			String("Rotation"),
			String("Scale"),
			String("WorldMatrix"),
			String("LinearVelocity"),
			String("AngularVelocity"),
			String("Active"),
			String("Bounds"),
			String("Material"),
			String("Skeleton"),

			/// [EN] Only ever made by loading a component whose script is missing, never added by hand.
			/// [JP] スクリプトの無いコンポーネントを読み込んだときにだけ作られるもので、手で追加するものではない。
			String("UnknownComponent")
		};

		return std::ranges::contains(builtinComponents, componentName);
	}

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
	Bool AddComponentPanel::CheckFilter(const String& componentName, const std::string& filterText)const
	{
		if (filterText.empty())
		{
			return true;
		}

		/// [EN] A view onto the interned name, so the check copies no string. tolower takes its argument as an unsigned char value, so each Char is converted first; multibyte UTF-8 bytes are negative as Char.
		/// [JP] インターン済みの名前への view なので、確認で文字列をコピーしない。tolower は引数を unsigned char の値として受け取るので、各 Char を先に変換する。UTF-8 の多バイト文字のバイトは Char では負になるため。
		std::string_view targetName = static_cast<std::string_view>(componentName);
		auto found = std::ranges::search(targetName, filterText, [](Char a, Char b) { return std::tolower(static_cast<Uchar>(a)) == std::tolower(static_cast<Uchar>(b)); });
		return !found.empty();
	}
}

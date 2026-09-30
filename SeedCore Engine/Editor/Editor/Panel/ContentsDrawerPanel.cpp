#include <Editor/Editor/Panel/ContentsDrawerPanel.h>

#include <Editor/Editor/Build/VisualStudioAutomation.h>
#include <Editor/Editor/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/ResourceSyncControlPanel.h>

#include <FoundationEngine/File/FileDialog.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Prefab/Prefab.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/ResourceSync.h>
#include <FoundationEngine/World/Actor/Actor.h>

#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Graphics.h>
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/Model/ModelExporter.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Texture/TextureResource.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the context and icons and builds the first tree.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンテキストとアイコンを結び付け、最初のツリーを作る。
	*/
	ContentsDrawerPanel::ContentsDrawerPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		BuildDirectory();
	}

	/**
	* [EN]
	* Refreshes the tree when needed, then draws the toolbar, the search
	* results or the tree and contents, and the new-script dialog.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 必要ならツリーを更新してから、ツールバー、検索結果またはツリーと中身、
	* 新規スクリプトのダイアログを描く。
	*/
	void ContentsDrawerPanel::Draw()
	{
		/// [EN] Let the ResourceCache reload on file changes under UserProject; a reload advances its revision.
		/// [JP] UserProject 以下のファイル変更で ResourceCache に読み直させる。読み直すと revision が進む。
		ResourceCache* resource = context_.worldContext_.resource_;
		D3D12Context& d3d12Context = context_.graphicsContext_.graphics_->GetContext();
		resource->Watch(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());

		/// [EN] Rebuild here, before anything is drawn, because the menus below hold references into the tree.
		/// [JP] 下のメニューはツリーの中を参照するので、何かを描く前のここで作り直す。
		Uint64 syncRevision = context_.resourceSync_ ? context_.resourceSync_->Revision() : 0;
		if (resourceRevision_ != resource->Revision() || syncRevision_ != syncRevision)
		{
			resourceRevision_ = resource->Revision();
			syncRevision_ = syncRevision;
			BuildDirectory();
		}

		ImGui::SetNextWindowDockID(context_.graphicsContext_.imgui_->DockSpaceID(), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("コンテンツドロワー"))
		{
			if (context_.resourceSync_)
			{
				ResourceSyncControlPanel::DrawStatus(context_);
			}

			/// [EN] Mouse buttons 3 and 4 are the side back/forward buttons.
			/// [JP] マウスボタン 3 と 4 は、横にある戻る/進むボタン。
			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
			{
				if (ImGui::IsMouseClicked(3) && historyState_.index_ > 0)
				{
					--historyState_.index_;
				}
				if (ImGui::IsMouseClicked(4) && historyState_.index_ + 1 < historyState_.directoryList_.size())
				{
					++historyState_.index_;
				}
			}

			/// [EN] View switch, and the icon size slider that only grid view uses.
			/// [JP] 表示の切り替えと、グリッド表示だけで使うアイコンの大きさのスライダー。
			if (ImGui::RadioButton("リスト", viewType_ == ViewType::List))
			{
				viewType_ = ViewType::List;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("グリッド", viewType_ == ViewType::Grid))
			{
				viewType_ = ViewType::Grid;
			}

			if (viewType_ == ViewType::Grid)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(120.0f);
				ImGui::SliderFloat("##IconSize", &gridIconSize_, 32.0f, 128.0f, "%.0f");
			}

			/// [EN] Widen the left padding by one icon so the search icon can be drawn inside the field.
			/// [JP] 検索アイコンを入力欄の中に描けるよう、左の余白をアイコン 1 つ分広げる。
			ImGui::SameLine();
			Float iconSize = ImGui::GetTextLineHeight();
			Float paddingX = ImGui::GetStyle().FramePadding.x;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(iconSize + paddingX * 2.0f, ImGui::GetStyle().FramePadding.y));
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputTextWithHint("##Search", "検索...", searchFilter_.InputBuf, IM_ARRAYSIZE(searchFilter_.InputBuf)))
			{
				searchFilter_.Build();
			}
			ImGui::PopStyleVar();

			/// [EN] Draw the search icon in that padding, centred vertically.
			/// [JP] その余白に、縦中央で検索アイコンを描く。
			ImVec2 inputMin = ImGui::GetItemRectMin();
			Float iconY = inputMin.y + (ImGui::GetItemRectSize().y - iconSize) * 0.5f;
			ImGui::GetWindowDrawList()->AddImage(imguiTexture_.Icon(IconType::Search), ImVec2(inputMin.x + paddingX, iconY), ImVec2(inputMin.x + paddingX + iconSize, iconY + iconSize));
			ImGui::Separator();

			/// [EN] A search lists matching assets from every folder instead of the tree.
			/// [JP] 検索中は、ツリーの代わりに全フォルダから一致するアセットを一覧で出す。
			if (searchFilter_.IsActive())
			{
				for (const AssetRecord& asset : assetList_)
				{
					if (!searchFilter_.PassFilter(asset.path_.c_str()))
					{
						continue;
					}
					ImGui::PushID(asset.assetID_);

					ImGui::Image(GetAssetIcon(asset), ImVec2(iconSize, iconSize));

					/// [EN] The badge sits on the lower-right quarter of the icon, the
					///      corner an asset icon is least likely to fill.
					/// [JP] バッジはアイコンの右下 1/4 に重ねる。アセットのアイコンが
					///      埋めている可能性が最も低い角だから。
					if (ImTextureID sharingIcon = GetSharingIcon(asset))
					{
						ImVec2 iconMin = ImGui::GetItemRectMin();
						ImVec2 iconMax = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddImage(sharingIcon, ImVec2((iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f), iconMax);
					}

					ImGui::SameLine();
					ImGui::Selectable(asset.path_.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowDoubleClick);

					/// [EN] A remote-only asset has no local file yet, so it cannot be dragged anywhere.
					/// [JP] 共有ライブラリにしか無いアセットはまだローカルのファイルが無いので、ドラッグできない。
					if ((!context_.resourceSync_ || !context_.resourceSync_->RemoteOnly(asset.assetID_)) && ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload(GetPayloadType(asset.type_), &asset.assetID_, sizeof(Uint32));
						ImGui::Text("%s", FilePath(asset.fullpath_.str(), resource->ProjectRootPath()).FilenameText().c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::IsItemHovered())
					{
						DrawAssetTooltip(asset);
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							OpenAssetPopup(asset);
						}
					}

					ImGui::PopID();
				}
			}
			else
			{
				/// [EN] Folder tree on the left, 30% of the width but never narrower than 150 pixels, resizable by dragging its edge.
				/// [JP] 左はフォルダのツリー。幅の 30% だが 150 ピクセルより狭くはせず、端をドラッグして変えられる。
				ImGui::BeginChild("##DirectoryTree", ImVec2(std::max(ImGui::GetContentRegionAvail().x * 0.3f, 150.0f), 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
				DrawDirectoryTree(root_);
				DrawBackgroundMenu();
				ImGui::EndChild();

				ImGui::SameLine();

				/// [EN] Contents of the selected folder on the right.
				/// [JP] 右は選択中のフォルダの中身。
				ImGui::BeginChild("##AssetList", ImVec2(0, 0), ImGuiChildFlags_Borders);

				/// [EN] Walk the selected folder's relative path down the tree, stopping early if a folder has gone.
				/// [JP] 選択中のフォルダの相対パスをたどってツリーを下りる。途中のフォルダが無くなっていればそこで止まる。
				FolderNode* folder = &root_;
				DynamicArray<FolderNode*> breadcrumb{ folder };
				for (const std::filesystem::path& segment : SelectedDirectory().RelativePath())
				{
					if (!folder->children_.contains(segment.string()))
					{
						break;
					}
					breadcrumb.push_back(folder = &folder->children_.at(segment.string()));
				}

				/// [EN] Breadcrumb: every ancestor is a borderless button that jumps there; the last entry is plain text.
				/// [JP] パンくずリスト。祖先はそこへ移動する枠無しのボタン、最後の要素はただの文字。
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				for (Size breadIndex = 0; breadIndex < breadcrumb.size(); ++breadIndex)
				{
					ImGui::PushID(static_cast<Int>(breadIndex));
					if (breadIndex > 0)
					{
						ImGui::SameLine(0.0f, 2.0f);
						ImGui::TextDisabled(">");
						ImGui::SameLine(0.0f, 2.0f);
					}
					if (breadIndex + 1 == breadcrumb.size())
					{
						ImGui::Text("%s", breadcrumb[breadIndex]->path_.FilenameText().c_str());
					}
					else if (ImGui::SmallButton(breadcrumb[breadIndex]->path_.FilenameText().c_str()))
					{
						SelectDirectory(breadcrumb[breadIndex]->path_);
					}
					ImGui::PopID();
				}
				ImGui::PopStyleColor();
				ImGui::Separator();

				if (viewType_ == ViewType::List)
				{
					DrawAssetListMode(*folder);
				}
				else
				{
					DrawAssetGridMode(*folder);
				}
				DrawBackgroundMenu();
				ImGui::EndChild();

				/// [EN] An actor dropped from the Hierarchy is saved as a Prefab in the selected folder and linked to it.
				/// [JP] Hierarchy からドロップしたアクターは、選択中のフォルダへ Prefab として保存し、その Prefab に結び付ける。
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ACTOR"))
					{
						Actor dropped = *static_cast<const Actor*>(payload->Data);
						const FilePath savedPath(Prefab::SaveToDirectory(dropped, SelectedDirectory().FullPath()), SelectedDirectory().RootPath());
						if (!savedPath.Empty())
						{
							/// [EN] Reload at once rather than waiting for the watch, because the new Prefab's ID is needed right now.
							/// [JP] 新しい Prefab の ID がすぐ要るので、監視を待たずにここで読み直す。
							resource->Reload(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());
							if (Uint32 newAssetID = resource->GetAssetID(String(savedPath.RelativeText())))
							{
								dropped.PrefabID(newAssetID);
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
			}

			/// [EN] Drawn at window level so a request from any menu, tree or contents, opens in the same place.
			/// [JP] ウィンドウの階層で描くので、ツリーと中身どちらのメニューからの要求も同じ場所で開く。
			DrawScriptMenu();
		}

		ImGui::End();
	}

	/**
	* [EN]
	* Draws one tree folder and its open children.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ツリーのフォルダ 1 つと、開いている子を描く。
	*/
	void ContentsDrawerPanel::DrawDirectoryTree(FolderNode& node)
	{
		/// [EN] A folder with no subfolders is a leaf, the selected folder is highlighted, and the root starts open.
		/// [JP] 子フォルダが無ければ葉、選択中のフォルダは強調、根は最初から開いておく。
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | (node.children_.empty() ? ImGuiTreeNodeFlags_Leaf : 0) | (SelectedDirectory().FullPath() == node.path_.FullPath() ? ImGuiTreeNodeFlags_Selected : 0) | (&node == &root_ ? ImGuiTreeNodeFlags_DefaultOpen : 0);

		/// [EN] A cut folder is dimmed; pushing the current alpha otherwise keeps a cut parent's dimming on its children.
		/// [JP] 切り取ったフォルダは薄くする。そうでなければ今の alpha を積むので、切り取った親の薄さが子にも残る。
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardState_.type_ == ClipboardType::Cut && clipboardState_.path_.FullPath() == node.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
		ImGui::PushID(node.path_.FilenameText().c_str());

		/// [EN] The tree node itself has no label; icon and name follow it on the same line.
		/// [JP] ツリーノード自体はラベル無しで、アイコンと名前を同じ行に続ける。
		Bool opened = ImGui::TreeNodeEx("##tree", flags);
		Bool treeClicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
		DrawFolderMenu(node.path_);

		ImGui::SameLine();
		ImGui::Image(GetFolderIcon(node), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));
		ImGui::SameLine();
		if (!DrawRenameMenu(node.path_))
		{
			ImGui::Text("%s", node.path_.FilenameText().c_str());
		}

		/// [EN] Clicking the row or the name selects the folder; clicking the arrow only opens or closes it.
		/// [JP] 行か名前をクリックするとフォルダを選ぶ。矢印のクリックは開閉だけ。
		if (treeClicked || ImGui::IsItemClicked())
		{
			SelectDirectory(node.path_);
		}

		if (opened)
		{
			for (FolderNode& child : node.children_ | std::ranges::views::values)
			{
				DrawDirectoryTree(child);
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
		ImGui::PopStyleVar();
	}

	/**
	* [EN]
	* Draws the folder's contents as icon-and-name rows.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォルダの中身をアイコンと名前の行で描く。
	*/
	void ContentsDrawerPanel::DrawAssetListMode(FolderNode& folder)
	{
		Float iconSize = ImGui::GetTextLineHeight();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		/// [EN] Folders first. The icon space is reserved with a dummy and drawn after the row, so the selection highlight does not cover it.
		/// [JP] 先にフォルダ。アイコンの場所はダミーで空けておき、行の後で描くので、選択の強調に隠れない。
		for (FolderNode& child : folder.children_ | std::ranges::views::values)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardState_.type_ == ClipboardType::Cut && clipboardState_.path_.FullPath() == child.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::PushID(child.path_.FilenameText().c_str());

			ImGui::Dummy(ImVec2(iconSize, iconSize));
			ImVec2 iconMin = ImGui::GetItemRectMin();
			ImVec2 iconMax = ImGui::GetItemRectMax();
			ImGui::SameLine();

			if (!DrawRenameMenu(child.path_))
			{
				ImGui::Selectable(child.path_.FilenameText().c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowDoubleClick);
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					SelectDirectory(child.path_);
				}
				DrawFolderMenu(child.path_);
			}

			drawList->AddImage(GetFolderIcon(child), iconMin, iconMax);

			ImGui::PopID();
			ImGui::PopStyleVar();
		}

		/// [EN] Then assets, laid out the same way.
		/// [JP] 次にアセット。並べ方はフォルダと同じ。
		for (const AssetRecord* asset : folder.assets_)
		{
			const FilePath assetPath(asset->fullpath_.str(), root_.path_.RootPath());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardState_.type_ == ClipboardType::Cut && clipboardState_.path_.FullPath() == assetPath.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::PushID(asset->assetID_);

			ImGui::Dummy(ImVec2(iconSize, iconSize));
			ImVec2 iconMin = ImGui::GetItemRectMin();
			ImVec2 iconMax = ImGui::GetItemRectMax();
			ImGui::SameLine();

			if (!DrawRenameMenu(assetPath))
			{
				ImGui::Selectable(assetPath.FilenameText().c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowDoubleClick);
			}

			/// [EN] A remote-only asset has no local file yet, so it cannot be dragged anywhere.
			/// [JP] 共有ライブラリにしか無いアセットはまだローカルのファイルが無いので、ドラッグできない。
			if ((!context_.resourceSync_ || !context_.resourceSync_->RemoteOnly(asset->assetID_)) && ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload(GetPayloadType(asset->type_), &asset->assetID_, sizeof(Uint32));
				ImGui::Text("%s", assetPath.FilenameText().c_str());
				ImGui::EndDragDropSource();
			}

			DrawAssetMenu(*asset, assetPath);

			if (ImGui::IsItemHovered())
			{
				DrawAssetTooltip(*asset);
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					OpenAssetPopup(*asset);
				}
			}

			drawList->AddImage(GetAssetIcon(*asset), iconMin, iconMax);

			/// [EN] The badge sits on the lower-right quarter of the icon, the
			///      corner an asset icon is least likely to fill.
			/// [JP] バッジはアイコンの右下 1/4 に重ねる。アセットのアイコンが
			///      埋めている可能性が最も低い角だから。
			if (ImTextureID sharingIcon = GetSharingIcon(*asset))
			{
				drawList->AddImage(sharingIcon, ImVec2((iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f), iconMax);
			}

			ImGui::PopID();
			ImGui::PopStyleVar();
		}
	}

	/**
	* [EN]
	* Draws the folder's contents as a grid of icon buttons.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォルダの中身をアイコンボタンの格子で描く。
	*/
	void ContentsDrawerPanel::DrawAssetGridMode(FolderNode& folder)
	{
		/// [EN] As many columns as whole cells fit the width, at least one.
		/// [JP] 列の数は、幅に収まるセルの数。最低 1 列。
		Float buttonWidth = gridIconSize_ + ImGui::GetStyle().FramePadding.x * 2.0f;
		Int columns = std::max(1, static_cast<Int>(ImGui::GetContentRegionAvail().x / (buttonWidth + ImGui::GetStyle().ItemSpacing.x)));
		Int index = 0;

		/// [EN] A name under a button: wrapped when wider than the button, otherwise centred under it.
		/// [JP] ボタンの下の名前。ボタンより広ければ折り返し、そうでなければ中央に寄せる。
		auto drawLabel = [buttonWidth](const std::string& label)
		{
			Float textWidth = ImGui::CalcTextSize(label.c_str()).x;
			if (textWidth > buttonWidth)
			{
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + buttonWidth);
				ImGui::TextWrapped("%s", label.c_str());
				ImGui::PopTextWrapPos();
			}
			else
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (buttonWidth - textWidth) * 0.5f);
				ImGui::Text("%s", label.c_str());
			}
		};

		/// [EN] Folders first; every cell except the first of a row continues the current line.
		/// [JP] 先にフォルダ。行の先頭以外のセルは、今の行に続けて並べる。
		for (FolderNode& child : folder.children_ | std::ranges::views::values)
		{
			if (index++ % columns != 0)
			{
				ImGui::SameLine();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardState_.type_ == ClipboardType::Cut && clipboardState_.path_.FullPath() == child.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::BeginGroup();

			ImGui::PushID(child.path_.FilenameText().c_str());
			ImGui::ImageButton("##folder", GetFolderIcon(child), ImVec2(gridIconSize_, gridIconSize_));
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				SelectDirectory(child.path_);
			}
			DrawFolderMenu(child.path_);
			ImGui::PopID();

			ImGui::SetNextItemWidth(buttonWidth);
			if (!DrawRenameMenu(child.path_))
			{
				drawLabel(child.path_.FilenameText());
			}

			ImGui::EndGroup();
			ImGui::PopStyleVar();
		}

		/// [EN] Then assets, continuing the same grid.
		/// [JP] 次にアセット。同じ格子の続きに並べる。
		for (const AssetRecord* asset : folder.assets_)
		{
			if (index++ % columns != 0)
			{
				ImGui::SameLine();
			}

			const FilePath assetPath(asset->fullpath_.str(), root_.path_.RootPath());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardState_.type_ == ClipboardType::Cut && clipboardState_.path_.FullPath() == assetPath.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::BeginGroup();

			ImGui::PushID(asset->assetID_);
			ImGui::ImageButton("##asset", GetAssetIcon(*asset), ImVec2(gridIconSize_, gridIconSize_));

			/// [EN] The badge takes a third of the button in grid mode, where the
			///      icon is large enough that a quarter would read as noise.
			/// [JP] グリッド表示ではボタンの1/3をバッジに使う。アイコンが大きいため、
			///      1/4 ではゴミのように見えてしまう。
			if (ImTextureID sharingIcon = GetSharingIcon(*asset))
			{
				ImVec2 badgeMax = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddImage(sharingIcon, ImVec2(badgeMax.x - gridIconSize_ / 3.0f, badgeMax.y - gridIconSize_ / 3.0f), badgeMax);
			}

			/// [EN] A remote-only asset has no local file yet, so it cannot be dragged anywhere.
			/// [JP] 共有ライブラリにしか無いアセットはまだローカルのファイルが無いので、ドラッグできない。
			if ((!context_.resourceSync_ || !context_.resourceSync_->RemoteOnly(asset->assetID_)) && ImGui::BeginDragDropSource())
			{
				ImGui::SetDragDropPayload(GetPayloadType(asset->type_), &asset->assetID_, sizeof(Uint32));
				ImGui::Text("%s", assetPath.FilenameText().c_str());
				ImGui::EndDragDropSource();
			}

			DrawAssetMenu(*asset, assetPath);
			ImGui::PopID();

			if (ImGui::IsItemHovered())
			{
				DrawAssetTooltip(*asset);
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					OpenAssetPopup(*asset);
				}
			}

			ImGui::SetNextItemWidth(buttonWidth);
			if (!DrawRenameMenu(assetPath))
			{
				drawLabel(assetPath.FilenameText());
			}

			ImGui::EndGroup();
			ImGui::PopStyleVar();
		}
	}

	/**
	* [EN]
	* Draws an asset's hover tooltip.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットのツールチップを描く。
	*/
	void ContentsDrawerPanel::DrawAssetTooltip(const AssetRecord& asset)
	{
		ImGui::BeginTooltip();

		/// [EN] A loaded texture gets a 128-pixel preview above the text; anything else a small icon beside it.
		/// [JP] 読み込み済みのテクスチャは文字の上に 128 ピクセルのプレビュー、それ以外は横に小さなアイコン。
		Bool preview = asset.type_ == AssetType::Texture && asset.isLoaded_;
		Float iconSize = preview ? 128.0f : ImGui::GetTextLineHeight();
		ImGui::Image(GetAssetIcon(asset), ImVec2(iconSize, iconSize));
		if (preview)
		{
			ImGui::Separator();
		}
		else
		{
			ImGui::SameLine();
		}

		ImGui::Text("%s", asset.path_.c_str());
		ImGui::Text("ID: %u", asset.assetID_);
		if (context_.resourceSync_)
		{
			ResourceSyncControlPanel::DrawState(context_, asset);
		}

		/// [EN] File size in the largest unit that keeps the number at 1 or above; a remote-only asset has no file and shows none.
		/// [JP] ファイルサイズは、数字が 1 以上になる最大の単位で出す。共有ライブラリにしか無いアセットはファイルが無いので出さない。
		std::error_code errorCode;
		Uint64 fileSize = std::filesystem::file_size(std::filesystem::path(asset.fullpath_.c_str()), errorCode);
		if (!errorCode)
		{
			if (fileSize >= 1024 * 1024)
			{
				ImGui::Text("%.2f MB", static_cast<Float>(fileSize) / (1024.0f * 1024.0f));
			}
			else if (fileSize >= 1024)
			{
				ImGui::Text("%.1f KB", static_cast<Float>(fileSize) / 1024.0f);
			}
			else
			{
				ImGui::Text("%llu Bytes", fileSize);
			}
		}

		/// [EN] Pixel size of the texture, read from its GPU resource.
		/// [JP] テクスチャのピクセルサイズ。GPU リソースから読む。
		if (preview)
		{
			TextureResource* textureResource = context_.worldContext_.resource_->GetResource<TextureResource>(AssetType::Texture);
			Texture* texture = textureResource->Resolve(*context_.worldContext_.loader_, &context_.graphicsContext_.graphics_->GetBindlessHeap(), textureResource->GetHandle(asset.assetID_), context_.uiFrame_);
			if (texture && texture->Resource())
			{
				D3D12_RESOURCE_DESC desc = texture->Resource()->GetDesc();
				ImGui::Text("%llu x %u", desc.Width, desc.Height);
			}
		}

		ImGui::EndTooltip();
	}

	/**
	* [EN]
	* Draws a folder's right-click menu. A MenuItem closes its popup when
	* clicked, so no item closes it by hand.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォルダの右クリックメニューを描く。MenuItem はクリックでポップアップを
	* 閉じるので、どの項目も自分では閉じない。
	*/
	void ContentsDrawerPanel::DrawFolderMenu(const FilePath& folderPath)
	{
		if (ImGui::BeginPopupContextItem("##FolderContext"))
		{
			if (ImGui::MenuItem("新規フォルダ"))
			{
				CreateNewFolder(FilePath(folderPath.ChildPath("New Folder"), folderPath.RootPath()));
			}
			if (ImGui::MenuItem("新規 C++ スクリプト"))
			{
				OpenScriptPopup(folderPath, ScriptType::Cpp);
			}
			if (ImGui::MenuItem("新規 C# スクリプト"))
			{
				OpenScriptPopup(folderPath, ScriptType::Csharp);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("切り取り"))
			{
				clipboardState_.type_ = ClipboardType::Cut;
				clipboardState_.path_ = folderPath;
			}
			if (ImGui::MenuItem("コピー"))
			{
				clipboardState_.type_ = ClipboardType::Copy;
				clipboardState_.path_ = folderPath;
			}
			if (ImGui::MenuItem("貼り付け", nullptr, false, clipboardState_.type_ != ClipboardType::None))
			{
				ExecutePaste(folderPath);
			}

			ImGui::Separator();

			/// [EN] The name field edits the filename text in place, so it is padded to 256 characters of buffer.
			/// [JP] 入力欄はファイル名の文字列をそのまま編集するので、バッファとして 256 文字に広げる。
			if (ImGui::MenuItem("名前変更"))
			{
				renameMenuState_.focusRequested_ = true;
				renameMenuState_.targetPath_.emplace(folderPath);
				renameMenuState_.targetPath_->FilenameText().resize(256);
			}
			if (ImGui::MenuItem("削除"))
			{
				ExecuteDelete(folderPath);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("エクスプローラーで開く"))
			{
				ShellExecuteW(NULL, L"explore", folderPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}

			ImGui::EndPopup();
		}
	}

	/**
	* [EN]
	* Draws an asset's right-click menu.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの右クリックメニューを描く。
	*/
	void ContentsDrawerPanel::DrawAssetMenu(const AssetRecord& asset, const FilePath& assetPath)
	{
		if (ImGui::BeginPopupContextItem("##AssetContext"))
		{
			if (context_.resourceSync_)
			{
				ResourceSyncControlPanel::DrawActions(context_, asset);
			}
			if (ImGui::MenuItem("開く"))
			{
				OpenAssetPopup(asset);
			}

			/// [EN] Asset actions exist only for models.
			/// [JP] アセットアクションはモデルにだけある。
			if (asset.type_ == AssetType::Model && ImGui::BeginMenu("アセットアクション"))
			{
				if (ImGui::BeginMenu("コリジョン生成"))
				{
					if (ImGui::MenuItem("Proxy"))
					{
						GenerateMeshCollision(asset, MeshCollisionDetail::Proxy);
					}
					if (ImGui::MenuItem("Exact"))
					{
						GenerateMeshCollision(asset, MeshCollisionDetail::Exact);
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("モデル変換"))
				{
					context_.modelTransformPreviewContext_.requestedAssetId_ = asset.assetID_;
				}
				if (ImGui::MenuItem("マテリアル生成"))
				{
					GenerateMaterial(asset);
				}
				if (ImGui::MenuItem("スケルトン生成"))
				{
					GenerateSkeleton(asset);
				}

				/// [EN] One entry per export preset: its label, the preset and the file extension it writes.
				/// [JP] エクスポートのプリセットごとに 1 項目。表示名、プリセット、書き出す拡張子。
				if (ImGui::BeginMenu("エクスポート"))
				{
					static const std::tuple<const Char*, ExportPreset, const Wchar*> exportList[] =
					{
						{ "glTF", ExportPreset::Gltf, L"gltf" },
						{ "glTF binary", ExportPreset::Glb, L"glb" },
						{ "FBX (Maya)", ExportPreset::FbxMaya, L"fbx" },
						{ "FBX (Unreal)", ExportPreset::FbxUnreal, L"fbx" },
						{ "FBX (Unity)", ExportPreset::FbxUnity, L"fbx" },
						{ "FBX (エンジン)", ExportPreset::FbxNative, L"fbx" },
					};
					for (const auto& [label, preset, extension] : exportList)
					{
						if (ImGui::MenuItem(label))
						{
							ExportModel(asset, preset, extension);
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("切り取り"))
			{
				clipboardState_.type_ = ClipboardType::Cut;
				clipboardState_.path_ = assetPath;
			}
			if (ImGui::MenuItem("コピー"))
			{
				clipboardState_.type_ = ClipboardType::Copy;
				clipboardState_.path_ = assetPath;
			}

			ImGui::Separator();

			/// [EN] The name field edits the filename text in place, so it is padded to 256 characters of buffer.
			/// [JP] 入力欄はファイル名の文字列をそのまま編集するので、バッファとして 256 文字に広げる。
			if (ImGui::MenuItem("名前変更"))
			{
				renameMenuState_.focusRequested_ = true;
				renameMenuState_.targetPath_.emplace(assetPath);
				renameMenuState_.targetPath_->FilenameText().resize(256);
			}
			if (ImGui::MenuItem("削除"))
			{
				ExecuteDelete(assetPath);
			}

			ImGui::Separator();

			/// [EN] "/select," opens the containing folder with the file highlighted.
			/// [JP] "/select," は、そのファイルを選んだ状態で入っているフォルダを開く。
			if (ImGui::MenuItem("エクスプローラーで表示"))
			{
				std::wstring param = L"/select,\"" + assetPath.FullPath().wstring() + L"\"";
				ShellExecuteW(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
			}

			ImGui::EndPopup();
		}
	}

	/**
	* [EN]
	* Draws the empty-space menu for the selected folder. It only opens
	* where no item is under the cursor, so folder and asset menus win.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 選択中のフォルダを対象に、何も無いところのメニューを描く。カーソルの下に
	* 項目が無いときだけ開くので、フォルダとアセットのメニューが優先される。
	*/
	void ContentsDrawerPanel::DrawBackgroundMenu()
	{
		if (ImGui::BeginPopupContextWindow("##BackgroundContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			const FilePath& directoryPath = SelectedDirectory();
			if (ImGui::MenuItem("新規フォルダ"))
			{
				CreateNewFolder(FilePath(directoryPath.ChildPath("New Folder"), directoryPath.RootPath()));
			}
			if (ImGui::MenuItem("新規 C++ スクリプト"))
			{
				OpenScriptPopup(directoryPath, ScriptType::Cpp);
			}
			if (ImGui::MenuItem("新規 C# スクリプト"))
			{
				OpenScriptPopup(directoryPath, ScriptType::Csharp);
			}
			if (ImGui::MenuItem("貼り付け", nullptr, false, clipboardState_.type_ != ClipboardType::None))
			{
				ExecutePaste(directoryPath);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("エクスプローラーで開く"))
			{
				ShellExecuteW(NULL, L"explore", directoryPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}

			ImGui::EndPopup();
		}
	}

	/**
	* [EN]
	* Opens and draws the new-script dialog.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 新規スクリプトのダイアログを開いて描く。
	*/
	void ContentsDrawerPanel::DrawScriptMenu()
	{
		/// [EN] The title is also the popup's ID, so the language picks which dialog opens.
		/// [JP] タイトルはポップアップの ID も兼ねるので、言語でどちらのダイアログを開くかが決まる。
		const Char* title = scriptMenuState_.scriptType_ == ScriptType::Csharp ? "新規 C# スクリプト" : "新規 C++ スクリプト";
		if (scriptMenuState_.openRequested_)
		{
			ImGui::OpenPopup(title);
			scriptMenuState_.openRequested_ = false;
		}

		/// [EN] Passing open gives the dialog a close button.
		/// [JP] open を渡すと、ダイアログに閉じるボタンが付く。
		Bool open = true;
		if (ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (scriptMenuState_.focusRequested_)
			{
				ImGui::SetKeyboardFocusHere();
				scriptMenuState_.focusRequested_ = false;
			}

			/// [EN] The name field edits the target path's filename text directly.
			/// [JP] 名前の入力欄は、作るパスのファイル名の文字列を直接編集する。
			std::string& nameBuffer = scriptMenuState_.targetPath_->FilenameText();
			Bool confirmed = ImGui::InputText("スクリプト名", nameBuffer.data(), nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

			ImGui::Separator();

			/// [EN] Editing the text leaves the path's other parts stale, so the path is rebuilt from the typed name first.
			/// [JP] 文字列を編集してもパスの他の部分は古いままなので、入力した名前からパスを作り直してから渡す。
			if (ImGui::Button("作成") || confirmed)
			{
				const FilePath& targetPath = *scriptMenuState_.targetPath_;
				CreateNewScript(FilePath(targetPath.SiblingPath(targetPath.FilenameText().c_str()), targetPath.RootPath()));
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	/**
	* [EN]
	* Draws the inline rename field for the item being renamed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 名前変更中の項目に、その場の入力欄を描く。
	*/
	Bool ContentsDrawerPanel::DrawRenameMenu(const FilePath& itemPath)
	{
		if (!renameMenuState_.targetPath_ || renameMenuState_.targetPath_->FullPath() != itemPath.FullPath())
		{
			return false;
		}

		if (renameMenuState_.focusRequested_)
		{
			ImGui::SetKeyboardFocusHere();
			renameMenuState_.focusRequested_ = false;
		}

		std::string& nameBuffer = renameMenuState_.targetPath_->FilenameText();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		Bool confirmed = ImGui::InputText("##InlineRename", nameBuffer.data(), nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopStyleVar();

		/// [EN] Enter renames unless the name is empty; Enter or leaving the field ends the rename either way.
		/// [JP] Enter で名前を変える（空なら変えない）。Enter でも入力欄を離れても、名前変更はどちらでも終わる。
		if (confirmed && nameBuffer.front() != '\0')
		{
			ExecuteRename(*renameMenuState_.targetPath_);
		}
		if (confirmed || ImGui::IsItemDeactivated())
		{
			renameMenuState_.targetPath_.reset();
		}

		return true;
	}

	/**
	* [EN]
	* Moves to a folder and records it in the history.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォルダへ移動し、履歴に記録する。
	*/
	void ContentsDrawerPanel::SelectDirectory(const FilePath& directoryPath)
	{
		if (SelectedDirectory().FullPath() == directoryPath.FullPath())
		{
			return;
		}

		/// [EN] Moving somewhere new drops the entries after the current one, as a browser does.
		/// [JP] 新しい場所へ移動したら、ブラウザと同じく今より後の履歴を捨てる。
		historyState_.directoryList_.erase(historyState_.directoryList_.begin() + historyState_.index_ + 1, historyState_.directoryList_.end());
		historyState_.directoryList_.push_back(directoryPath);
		historyState_.index_ = historyState_.directoryList_.size() - 1;
	}

	/**
	* [EN]
	* Returns the history entry at the current position.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 今の位置にある履歴の要素を返す。
	*/
	const FilePath& ContentsDrawerPanel::SelectedDirectory()const
	{
		return historyState_.directoryList_[historyState_.index_];
	}

	/**
	* [EN]
	* Rebuilds the folder tree from disk and the asset list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ディスクとアセット一覧からフォルダのツリーを作り直す。
	*/
	void ContentsDrawerPanel::BuildDirectory()
	{
		/// [EN] Engine, tooling and build folders under the root; they hold no gameplay content, so they are not listed or descended into.
		/// [JP] ルート直下のエンジン、ツール、ビルドのフォルダ。ゲームの中身は無いので、並べず中にも入らない。
		static const std::set<std::string> excludeDirectories =
		{
			"AIEngine", "AudioEngine", "CompiledShaderObject", "Editor",
			"External", "FoundationEngine", "GraphicsEngine", "Launcher", "Logs",
			"Package", "PhysicsEngine", "Platform", "Runtime", "SeedCore", "Tools",
			".vs", "x64", ".git", ".asset",
		};

		const std::filesystem::path& projectRoot = context_.worldContext_.resource_->ProjectRootPath();
		root_ = {};
		root_.path_ = FilePath(projectRoot, projectRoot);

		/// [EN] Walks a relative path down from the root, creating each missing folder on the way, and returns the last one.
		/// [JP] 相対パスをたどって根から下り、途中で無いフォルダを作りながら、最後のフォルダを返す。
		auto insertDirectory = [this, &projectRoot](const std::filesystem::path& relativePath) -> FolderNode*
		{
			FolderNode* current = &root_;
			for (const std::filesystem::path& segment : relativePath)
			{
				const std::string name = segment.string();
				if (!current->children_.contains(name))
				{
					FolderNode node;
					node.path_ = FilePath(current->path_.ChildPath(segment), projectRoot);
					current->children_.insert(name, std::move(node));
				}
				current = &current->children_.at(name);
			}
			return current;
		};

		/// [EN] Every folder on disk appears, including empty ones.
		/// [JP] ディスク上のフォルダは、空のものも含めて全て出す。
		std::error_code errorCode;
		for (auto it = std::filesystem::recursive_directory_iterator(projectRoot, errorCode); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (!it->is_directory())
			{
				continue;
			}
			if (excludeDirectories.contains(it->path().filename().string()))
			{
				it.disable_recursion_pending();
				continue;
			}
			insertDirectory(FilePath(it->path(), projectRoot).RelativePath());
		}

		/// [EN] Local assets first, then shared-library assets that are not in this workspace yet.
		/// [JP] 先にローカルのアセット、次に共有ライブラリにあってこのワークスペースにまだ無いアセット。
		assetList_.clear();
		for (const AssetRecord& asset : context_.worldContext_.resource_->AssetList() | std::ranges::views::values)
		{
			assetList_.push_back(asset);
		}
		if (context_.resourceSync_)
		{
			DynamicArray<AssetRecord> remoteAssets;
			context_.resourceSync_->Gather(remoteAssets);
			for (const AssetRecord& remote : remoteAssets)
			{
				if (!std::ranges::any_of(assetList_, [&remote](const AssetRecord& local) { return local.assetID_ == remote.assetID_; }))
				{
					assetList_.push_back(remote);
				}
			}
		}

		/// [EN] Filed only after assetList_ stops growing, so the pointers into it stay valid.
		/// [JP] assetList_ が増えなくなってから振り分けるので、その中を指すポインタは有効なまま。
		for (const AssetRecord& asset : assetList_)
		{
			insertDirectory(std::filesystem::path(asset.path_.str()).parent_path())->assets_.push_back(&asset);
		}

		/// [EN] The first build starts the history at the root; later builds keep the history as it is.
		/// [JP] 最初の作成では履歴を根から始める。以降の作成では履歴をそのまま残す。
		if (historyState_.directoryList_.empty())
		{
			historyState_.directoryList_.push_back(root_.path_);
		}
	}

	/**
	* [EN]
	* Returns the icon of an asset type; a texture falls back to the text
	* icon when it has no thumbnail.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの種類のアイコンを返す。テクスチャはサムネイルが無いときに
	* テキストのアイコンになる。
	*/
	ImTextureID ContentsDrawerPanel::GetAssetIcon(AssetType type)const
	{
		switch (type)
		{
		case AssetType::Model:
			return imguiTexture_.Icon(IconType::Model);
		case AssetType::Effect:
			return imguiTexture_.Icon(IconType::Effect);
		case AssetType::Audio:
			return imguiTexture_.Icon(IconType::Audio);
		case AssetType::Font:
			return imguiTexture_.Icon(IconType::Font);
		case AssetType::Skymap:
			return imguiTexture_.Icon(IconType::Sky);
		case AssetType::Animation:
			return imguiTexture_.Icon(IconType::Animation);
		case AssetType::MeshCollision:
			return imguiTexture_.Icon(IconType::MeshCollision);
		case AssetType::Material:
			return imguiTexture_.Icon(IconType::Material);
		case AssetType::Skeleton:
			return imguiTexture_.Icon(IconType::Skeleton);
		case AssetType::Movie:
			return imguiTexture_.Icon(IconType::Movie);
		case AssetType::Prefab:
			return imguiTexture_.Icon(IconType::Prefab);
		case AssetType::Scene:
			return imguiTexture_.Icon(IconType::Scene);
		case AssetType::Texture:
			[[fallthrough]];
		default:
			return imguiTexture_.Icon(IconType::Text);
		}
	}

	/**
	* [EN]
	* Returns an asset's icon, creating and caching a texture thumbnail on
	* first use.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットのアイコンを返す。テクスチャのサムネイルは初回に作ってキャッシュする。
	*/
	ImTextureID ContentsDrawerPanel::GetAssetIcon(const AssetRecord& asset)const
	{
		if (asset.type_ == AssetType::Texture && asset.isLoaded_)
		{
			if (thumbnailCache_.contains(asset.assetID_))
			{
				return thumbnailCache_.at(asset.assetID_);
			}

			BindlessHeap& bindlessHeap = context_.graphicsContext_.graphics_->GetBindlessHeap();
			TextureResource* textureResource = context_.worldContext_.resource_->GetResource<TextureResource>(AssetType::Texture);
			Texture* texture = textureResource->Resolve(*context_.worldContext_.loader_, &bindlessHeap, textureResource->GetHandle(asset.assetID_), context_.uiFrame_);
			if (texture && texture->Resource())
			{
				/// [EN] Pin the texture so streaming never evicts the resource the thumbnail views.
				/// [JP] サムネイルが参照するリソースをストリーミングが追い出さないよう、テクスチャを固定する。
				texture->Pin();

				/// [EN] A view slot of its own, covering the whole resource, so the thumbnail stays valid when streaming swaps the texture's own bindless index.
				/// [JP] リソース全体を覆う専用のビューの枠を使う。ストリーミングがテクスチャ自身のバインドレス番号を差し替えても、サムネイルは有効なまま。
				Uint descIndex = bindlessHeap.AllocateIndex();
				context_.graphicsContext_.graphics_->GetContext().GetDevice()->CreateShaderResourceView(texture->Resource(), nullptr, bindlessHeap.CPUHandle(descIndex));
				ImTextureID textureID = static_cast<ImTextureID>(bindlessHeap.GPUHandle(descIndex).ptr);
				thumbnailCache_.insert({ asset.assetID_, textureID });
				return textureID;
			}
		}

		/// [EN] Source files are not asset types of their own, so they are told apart by extension.
		/// [JP] ソースファイルはアセットの種類としては区別されないので、拡張子で見分ける。
		std::string extension = std::filesystem::path(asset.path_.c_str()).extension().string();
		if (extension == ".h")
		{
			return imguiTexture_.Icon(IconType::Header);
		}
		if (extension == ".cpp")
		{
			return imguiTexture_.Icon(IconType::Cpp);
		}
		if (extension == ".hlsli" || extension == ".hlsl")
		{
			return imguiTexture_.Icon(IconType::Hlsl);
		}

		return GetAssetIcon(asset.type_);
	}

	/**
	* [EN]
	* Returns the sharing badge that matters most for the asset.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* そのアセットにとって最も重要な共有のバッジを返す。
	*/
	ImTextureID ContentsDrawerPanel::GetSharingIcon(const AssetRecord& asset)const
	{
		if (!context_.resourceSync_)
		{
			return 0;
		}

		const SharedAsset* shared = context_.resourceSync_->GetAsset(asset.assetID_);
		if (!shared)
		{
			return 0;
		}

		/// [EN] A conflict comes first because it is the only state that no
		///      automatic step will clear on its own.
		/// [JP] 競合を最優先にする。自動の処理では解消されない唯一の状態だから。
		if (context_.resourceSync_->Conflicted(asset.assetID_))
		{
			return imguiTexture_.Icon(IconType::SharedConflict);
		}

		/// [EN] Someone else's lease is next, since it is the one state that
		///      stops this member from doing anything with the asset.
		/// [JP] 次は他のメンバーの Lease。このメンバーがそのアセットに何もできない、
		///      唯一の状態だから。
		const DynamicArray<EditLease>& leaseList = context_.resourceSync_->GetLeases();
		if (std::ranges::any_of(leaseList, [shared](const EditLease& lease) { return lease.assetId_ == shared->id_ && !lease.mine_; }))
		{
			return imguiTexture_.Icon(IconType::Lock);
		}

		/// [EN] Unsent work outranks holding the lease, because the lease is
		///      already visible in the panel while unsent work is not.
		/// [JP] 未送信の作業は、Lease を持っていることより優先する。Lease はパネルに
		///      既に出ているが、未送信の作業はどこにも出ないため。
		if (context_.resourceSync_->Modified(asset.assetID_))
		{
			return imguiTexture_.Icon(IconType::SharedModified);
		}

		/// [EN] Any lease left at this point is this member's own.
		/// [JP] ここまで来て残っている Lease は、このメンバー自身のもの。
		if (std::ranges::any_of(leaseList, [shared](const EditLease& lease) { return lease.assetId_ == shared->id_; }))
		{
			return imguiTexture_.Icon(IconType::Unlock);
		}

		/// [EN] Remote-only means the catalog has it but this workspace does
		///      not, so it is on its way in rather than usable.
		/// [JP] RemoteOnly はカタログにあってこのワークスペースに無い状態。
		///      使えるのではなく、これから入ってくるということ。
		if (context_.resourceSync_->RemoteOnly(asset.assetID_))
		{
			return imguiTexture_.Icon(IconType::SharedOutdated);
		}

		return imguiTexture_.Icon(IconType::SharedAsset);
	}

	/**
	* [EN]
	* Returns the filled folder icon when the folder holds a subfolder or
	* an asset, the empty one otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 子フォルダかアセットがあれば中身ありのフォルダのアイコン、無ければ空のアイコンを返す。
	*/
	ImTextureID ContentsDrawerPanel::GetFolderIcon(const FolderNode& node)const
	{
		return imguiTexture_.Icon(node.children_.empty() && node.assets_.empty() ? IconType::FolderNoItem : IconType::FolderInItem);
	}

	/**
	* [EN]
	* Returns the drag-and-drop payload name of an asset type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの種類のドラッグ&ドロップのペイロード名を返す。
	*/
	const Char* ContentsDrawerPanel::GetPayloadType(AssetType type)const
	{
		switch (type)
		{
		case AssetType::Texture:
			return "ASSET_TEXTURE";
		case AssetType::Model:
			return "ASSET_MODEL";
		case AssetType::Effect:
			return "ASSET_EFFECT";
		case AssetType::Audio:
			return "ASSET_AUDIO";
		case AssetType::Font:
			return "ASSET_FONT";
		case AssetType::Movie:
			return "ASSET_MOVIE";
		case AssetType::Animation:
			return "ASSET_ANIMATION";
		case AssetType::MeshCollision:
			return "ASSET_MESHCOLLISION";
		case AssetType::Material:
			return "ASSET_MATERIAL";
		case AssetType::Skeleton:
			return "ASSET_SKELETON";
		case AssetType::Skymap:
			return "ASSET_SKY";
		case AssetType::Prefab:
			return "ASSET_PREFAB";
		case AssetType::Scene:
			return "ASSET_SCENE";
		default:
			return "ASSET_UNKNOWN";
		}
	}

	/**
	* [EN]
	* Bakes a model's collision and registers it as an asset.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのコリジョンを焼き、アセットとして登録する。
	*/
	void ContentsDrawerPanel::GenerateMeshCollision(const AssetRecord& asset, MeshCollisionDetail detail)
	{
		D3D12Context& d3d12Context = context_.graphicsContext_.graphics_->GetContext();
		if (!context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model)->GenerateCollision(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), &context_.graphicsContext_.graphics_->GetBindlessHeap(), context_.graphicsContext_.graphics_->GetBC7CompressShader(), *context_.worldContext_.resource_, asset.assetID_, detail))
		{
			SC_LOG_WARNING("コンテンツドロワー: コリジョン生成に失敗しました: {}", asset.path_.c_str());
			return;
		}

		/// [EN] Rescan so the just-written ".collision" sibling is picked up as its own asset.
		/// [JP] 書き出した ".collision" 兄弟を個別アセットとして拾えるよう再スキャンする。
		context_.worldContext_.resource_->Reload(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());
		SC_LOG_NOTICE("コンテンツドロワー: コリジョンを生成しました: {}", asset.path_.c_str());
	}

	/**
	* [EN]
	* Writes a model's materials and registers them as assets.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのマテリアルを書き出し、アセットとして登録する。
	*/
	void ContentsDrawerPanel::GenerateMaterial(const AssetRecord& asset)
	{
		D3D12Context& d3d12Context = context_.graphicsContext_.graphics_->GetContext();
		if (!context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model)->GenerateMaterial(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), &context_.graphicsContext_.graphics_->GetBindlessHeap(), context_.graphicsContext_.graphics_->GetBC7CompressShader(), *context_.worldContext_.resource_, asset.assetID_, true))
		{
			SC_LOG_WARNING("コンテンツドロワー: マテリアル生成に失敗しました: {}", asset.path_.c_str());
			return;
		}

		/// [EN] Rescan so the just-written ".material" siblings are picked up as their own assets.
		/// [JP] 書き出した ".material" 兄弟を個別アセットとして拾えるよう再スキャンする。
		context_.worldContext_.resource_->Reload(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());
		SC_LOG_NOTICE("コンテンツドロワー: マテリアルを生成しました: {}", asset.path_.c_str());
	}

	/**
	* [EN]
	* Writes a skinned model's skeleton and registers it as an asset.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スキン付きモデルのスケルトンを書き出し、アセットとして登録する。
	*/
	void ContentsDrawerPanel::GenerateSkeleton(const AssetRecord& asset)
	{
		D3D12Context& d3d12Context = context_.graphicsContext_.graphics_->GetContext();
		if (!context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model)->GenerateSkeleton(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), &context_.graphicsContext_.graphics_->GetBindlessHeap(), context_.graphicsContext_.graphics_->GetBC7CompressShader(), *context_.worldContext_.resource_, asset.assetID_, false))
		{
			SC_LOG_WARNING("コンテンツドロワー: スケルトン生成に失敗しました（スキン無し？）: {}", asset.path_.c_str());
			return;
		}

		/// [EN] Rescan so the just-written ".skeleton" sibling is picked up as its own asset.
		/// [JP] 書き出した ".skeleton" 兄弟を個別アセットとして拾えるよう再スキャンする。
		context_.worldContext_.resource_->Reload(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());
		SC_LOG_NOTICE("コンテンツドロワー: スケルトンを生成しました: {}", asset.path_.c_str());
	}

	/**
	* [EN]
	* Deletes a file or folder with its ".meta".
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ファイルかフォルダを ".meta" ごと削除する。
	*/
	void ContentsDrawerPanel::ExecuteDelete(const FilePath& targetPath)
	{
		/// [EN] Shared content changes only through the shared library, so local deletion is refused.
		/// [JP] 共有コンテンツは共有ライブラリを通してしか変えないので、ローカルでの削除は断る。
		if (context_.resourceSync_ && context_.resourceSync_->Managed(targetPath.FullPath()))
		{
			SC_LOG_WARNING("コンテンツドロワー: 共有コンテンツはローカルのファイル操作で削除・名前変更できません");
			return;
		}

		/// [EN] Errors are ignored; a ".meta" that does not exist is simply not removed.
		/// [JP] エラーは無視する。".meta" が無ければ単に何も消さない。
		std::error_code errorCode;
		std::filesystem::remove_all(targetPath.FullPath(), errorCode);
		std::filesystem::remove(targetPath.AppendedSuffixPath(".meta"), errorCode);

		if (clipboardState_.path_.FullPath() == targetPath.FullPath())
		{
			clipboardState_.type_ = ClipboardType::None;
		}
	}

	/**
	* [EN]
	* Renames a file or folder with its ".meta".
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ファイルかフォルダを ".meta" ごと名前変更する。
	*/
	void ContentsDrawerPanel::ExecuteRename(const FilePath& targetPath)
	{
		/// [EN] Shared content changes only through the shared library, so local renaming is refused.
		/// [JP] 共有コンテンツは共有ライブラリを通してしか変えないので、ローカルでの名前変更は断る。
		if (context_.resourceSync_ && context_.resourceSync_->Managed(targetPath.FullPath()))
		{
			SC_LOG_WARNING("コンテンツドロワー: 共有コンテンツはローカルのファイル操作で削除・名前変更できません");
			return;
		}

		/// [EN] The new name is the edited filename text; the full path still holds the old name.
		/// [JP] 新しい名前は編集したファイル名の文字列。フルパスはまだ古い名前のまま。
		const FilePath renamedPath(targetPath.SiblingPath(targetPath.FilenameText().c_str()), targetPath.RootPath());
		std::error_code errorCode;
		std::filesystem::rename(targetPath.FullPath(), renamedPath.FullPath(), errorCode);
		std::filesystem::rename(targetPath.AppendedSuffixPath(".meta"), renamedPath.AppendedSuffixPath(".meta"), errorCode);

		if (clipboardState_.path_.FullPath() == targetPath.FullPath())
		{
			clipboardState_.path_ = renamedPath;
		}
	}

	/**
	* [EN]
	* Moves or copies the clipboard item into a folder.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クリップボードの項目をフォルダへ移動かコピーする。
	*/
	void ContentsDrawerPanel::ExecutePaste(const FilePath& directoryPath)
	{
		/// [EN] An item deleted since it was cut or copied empties the clipboard.
		/// [JP] 切り取りかコピーの後に項目が消えていたら、クリップボードを空にする。
		const FilePath& sourcePath = clipboardState_.path_;
		if (clipboardState_.type_ == ClipboardType::None || !std::filesystem::exists(sourcePath.FullPath()))
		{
			clipboardState_.type_ = ClipboardType::None;
			return;
		}

		/// [EN] Refused when either side is shared content.
		/// [JP] どちらか一方でも共有コンテンツなら断る。
		const FilePath destPath(directoryPath.ChildPath(sourcePath.FilenamePath()), directoryPath.RootPath());
		if (context_.resourceSync_ && (context_.resourceSync_->Managed(sourcePath.FullPath()) || context_.resourceSync_->Managed(destPath.FullPath())))
		{
			SC_LOG_WARNING("コンテンツドロワー: 共有コンテンツはローカルのクリップボード操作で移動・コピーできません");
			return;
		}

		/// [EN] A cut moves the item with its ".meta" so the asset keeps its ID; a copy leaves the ".meta" behind so the copy gets a new one.
		/// [JP] 切り取りは ".meta" ごと移動するのでアセットの ID は変わらない。コピーは ".meta" を持っていかないので、複製には新しい ID が付く。
		std::error_code errorCode;
		if (clipboardState_.type_ == ClipboardType::Cut)
		{
			std::filesystem::rename(sourcePath.FullPath(), destPath.FullPath(), errorCode);
			std::filesystem::rename(sourcePath.AppendedSuffixPath(".meta"), destPath.AppendedSuffixPath(".meta"), errorCode);
			clipboardState_.type_ = ClipboardType::None;
		}
		else if (std::filesystem::is_directory(sourcePath.FullPath()))
		{
			std::filesystem::copy(sourcePath.FullPath(), destPath.FullPath(), std::filesystem::copy_options::recursive, errorCode);
		}
		else
		{
			std::filesystem::copy_file(sourcePath.FullPath(), destPath.FullPath(), std::filesystem::copy_options::skip_existing, errorCode);
		}
	}

	/**
	* [EN]
	* Adds a script's files to UserProject.Cplusplus.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スクリプトのファイルを UserProject.Cplusplus に追加する。
	*/
	Bool ContentsDrawerPanel::ExecuteRegister(const FilePath& headerPath, const FilePath& cppPath)
	{
		const std::filesystem::path& projectRoot = context_.worldContext_.resource_->ProjectRootPath();

		/// [EN] Tried first: if Visual Studio has Runtime.sln open, letting it add the files itself keeps Solution Explorer in sync immediately and never triggers the "project modified outside the editor" reload prompt (see VisualStudioAutomation's own doc comment). Falls through to editing UserProject.Cplusplus.vcxproj directly — the only path available when Visual Studio isn't running this solution at all.
		/// [JP] まずこちらを試す: Visual Studio が Runtime.sln を開いていれば、ファイルの追加自体をVSにやらせることで Solution Explorer が即座に同期され、「プロジェクトが外部で変更されました」という再読み込み確認も一切発生しない(詳細は VisualStudioAutomation 自身のドキュメントコメントを参照)。Visual Studio がこのソリューションを開いていない場合にのみ、UserProject.Cplusplus.vcxproj を直接編集する経路へフォールバックする。
		if (VisualStudioAutomation::TryAddFile(projectRoot / "Runtime" / "Runtime.sln", "UserProject.Cplusplus", headerPath.FullPath(), cppPath.FullPath()))
		{
			return true;
		}

		/// [EN] Otherwise CreateScript.py edits the .vcxproj, run on the bundled Python with paths relative to UserProject.
		/// [JP] そうでなければ CreateScript.py が .vcxproj を書き換える。同梱の Python で、UserProject 基準のパスを渡して実行する。
		std::wstring commandLine = std::format(L"\"{}\" \"{}\" \"{}\" \"{}\" \"{}\"", (projectRoot / "Platform" / "Python" / "python.exe").wstring(), (projectRoot / "Tools" / "Python" / "CreateScript.py").wstring(), projectRoot.wstring(), headerPath.RelativePath().wstring(), cppPath.RelativePath().wstring());

		/// [EN] CreateProcessW may write into the command line, which std::wstring::data() allows.
		/// [JP] CreateProcessW はコマンドラインを書き換えることがあるが、std::wstring::data() はそれを許す。
		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};
		if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
		{
			return false;
		}

		/// [EN] Wait for the script so the files are registered before this returns; exit code 0 means success.
		/// [JP] 戻る前に登録が済むようスクリプトの終了を待つ。終了コード 0 が成功。
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode = 1;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return exitCode == 0;
	}

	/**
	* [EN]
	* Creates a folder with a free name and starts renaming it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 空いている名前でフォルダを作り、名前変更を始める。
	*/
	void ContentsDrawerPanel::CreateNewFolder(const FilePath& targetPath)
	{
		/// [EN] Number the name the same way Explorer does: "New Folder", "New Folder(2)", ...
		/// [JP] エクスプローラーと同じく番号を付ける。"New Folder"、"New Folder(2)"…
		std::filesystem::path newPath = targetPath.FullPath();
		for (Int index = 2; std::filesystem::exists(newPath); ++index)
		{
			newPath = targetPath.SiblingPath(targetPath.FilenameText() + "(" + std::to_string(index) + ")");
		}

		std::error_code errorCode;
		std::filesystem::create_directories(newPath, errorCode);
		if (errorCode)
		{
			SC_LOG_WARNING("コンテンツドロワー: フォルダの作成に失敗しました: {}", newPath.string());
			return;
		}

		/// [EN] The field shows once the watch reloads and the new folder is in the tree.
		/// [JP] 入力欄は、監視が読み直して新しいフォルダがツリーに入ってから出る。
		renameMenuState_.focusRequested_ = true;
		renameMenuState_.targetPath_.emplace(newPath, targetPath.RootPath());
		renameMenuState_.targetPath_->FilenameText().resize(256);
	}

	/**
	* [EN]
	* Creates a C++ or C# script and opens it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* C++ か C# のスクリプトを作って開く。
	*/
	void ContentsDrawerPanel::CreateNewScript(const FilePath& targetPath)
	{
		/// [EN] Reduce the name to a valid identifier: letters, digits and underscores, never starting with a digit.
		/// [JP] 名前を有効な識別子に削る。英数字とアンダースコアだけで、数字では始めない。
		std::string baseName;
		std::ranges::copy_if(targetPath.StemText(), std::back_inserter(baseName), [](Char character) { return std::isalnum(static_cast<unsigned char>(character)) || character == '_'; });
		if (baseName.empty())
		{
			baseName = "NewScript";
		}
		if (std::isdigit(static_cast<unsigned char>(baseName.front())))
		{
			baseName.insert(baseName.begin(), '_');
		}

		/// [EN] Scripts belong to UserProject, so a folder outside it ("..") falls back to UserProject/Script.
		/// [JP] スクリプトは UserProject に属するので、その外（".."）のフォルダなら UserProject/Script にする。
		const FilePath parentPath(targetPath.ParentPath(), targetPath.RootPath() / "UserProject");
		const std::filesystem::path targetDirectory = *parentPath.RelativePath().begin() == ".." ? parentPath.RootPath() / "Script" : parentPath.FullPath();

		/// [EN] Number the name while any of its .h/.cpp/.cs already exists, so C++ and C# never share a class name.
		/// [JP] .h/.cpp/.cs のどれかがあれば番号を付ける。C++ と C# でクラス名が重ならないようにするため。
		std::string finalName = baseName;
		for (Int index = 2; std::filesystem::exists(targetDirectory / (finalName + ".h")) || std::filesystem::exists(targetDirectory / (finalName + ".cpp")) || std::filesystem::exists(targetDirectory / (finalName + ".cs")); ++index)
		{
			finalName = baseName + std::to_string(index);
		}

		std::error_code errorCode;
		std::filesystem::create_directories(targetDirectory, errorCode);
		if (errorCode)
		{
			SC_LOG_WARNING("コンテンツドロワー: スクリプト保存先の作成に失敗しました: {}", targetDirectory.string());
			return;
		}

		/// [EN] Opens a created file in the Visual Studio that has Runtime.sln open, falling back to the file's shell association when none does.
		/// [JP] 作ったファイルを、Runtime.sln を開いている Visual Studio で開く。開いているものが無ければ、ファイルの関連付けで開く。
		const std::filesystem::path solutionPath = targetPath.RootPath() / "Runtime" / "Runtime.sln";
		auto openScript = [&solutionPath](const FilePath& scriptPath)
		{
			if (!VisualStudioAutomation::TryOpenFile(solutionPath, scriptPath.FullPath()))
			{
				ShellExecuteW(NULL, L"open", scriptPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		};

		/// [EN] C# is a single file that needs no project registration.
		/// [JP] C# はファイル 1 つで、プロジェクトへの登録は要らない。
		if (scriptMenuState_.scriptType_ == ScriptType::Csharp)
		{
			const FilePath scriptPath(targetDirectory / (finalName + ".cs"), parentPath.RootPath());
			std::ofstream file(scriptPath.FullPath(), std::ios::binary);
			file << "using System;\r\nusing SeedCore;\r\n\r\npublic class " << finalName << " : SeedScript\r\n{\r\n\tvoid OnStart()\r\n\t{\r\n\t}\r\n\r\n\tvoid OnTick(Single elapsedTime)\r\n\t{\r\n\t}\r\n}\r\n";
			file.close();
			SC_LOG_NOTICE("コンテンツドロワー: C# スクリプトを作成しました: {}", finalName);
			openScript(scriptPath);
			return;
		}

		/// [EN] C++ is a header and source; the source includes the header by its UserProject-relative path.
		/// [JP] C++ はヘッダとソース。ソースはヘッダを UserProject 基準のパスで include する。
		const FilePath headerPath(targetDirectory / (finalName + ".h"), parentPath.RootPath());
		const FilePath cppPath(targetDirectory / (finalName + ".cpp"), parentPath.RootPath());
		std::ofstream headerFile(headerPath.FullPath(), std::ios::binary);
		headerFile << "#pragma once\n#include <FoundationEngine/Prelude.h>\n#include <FoundationEngine/SeedScript.h>\n\nclass " << finalName << " :public SeedCore::SeedScript\n{\npublic:\n\tvoid OnStart();\n\tvoid OnTick(float elapsedTime);\n};\nREGISTER_COMPONENT(" << finalName << ");\n";
		std::ofstream cppFile(cppPath.FullPath(), std::ios::binary);
		cppFile << "#include \"UserProject/" << headerPath.RelativeText() << "\"\n\nvoid " << finalName << "::OnStart()\n{\n}\n\nvoid " << finalName << "::OnTick(float elapsedTime)\n{\n}\n";

		/// [EN] Close both files before Visual Studio reads them for registration and opening, so it sees the full contents.
		/// [JP] Visual Studio が登録と表示のために読む前に両方のファイルを閉じ、中身が全て書き込まれた状態にする。
		headerFile.close();
		cppFile.close();

		if (!ExecuteRegister(headerPath, cppPath))
		{
			SC_LOG_WARNING("コンテンツドロワー: スクリプトのプロジェクトへの登録に失敗しました: {}", finalName);
			return;
		}

		SC_LOG_NOTICE("コンテンツドロワー: C++ スクリプトを生成しました: {}", finalName);
		openScript(headerPath);
		openScript(cppPath);
	}

	/**
	* [EN]
	* Opens an asset according to its type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットを種類に応じて開く。
	*/
	void ContentsDrawerPanel::OpenAssetPopup(const AssetRecord& asset)
	{
		if (context_.resourceSync_ && context_.resourceSync_->RemoteOnly(asset.assetID_))
		{
			context_.resourceSync_->RequestGet(asset.assetID_);
			return;
		}
		if (asset.type_ == AssetType::Scene)
		{
			context_.sceneContext_.requestedSceneAssetID_ = asset.assetID_;
			return;
		}
		if (asset.type_ == AssetType::Material)
		{
			context_.panelContext_.materialViewerPanel_->Open();
			return;
		}

		/// [EN] Anything else opens in the application Windows associates with it.
		/// [JP] それ以外は、Windows が関連付けているアプリケーションで開く。
		const FilePath assetPath(asset.fullpath_.str(), context_.worldContext_.resource_->ProjectRootPath());
		ShellExecuteW(NULL, L"open", assetPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	/**
	* [EN]
	* Requests the new-script dialog.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 新規スクリプトのダイアログを要求する。
	*/
	void ContentsDrawerPanel::OpenScriptPopup(const FilePath& parentPath, ScriptType script)
	{
		/// [EN] The name field edits the filename text in place, so it is padded to 256 characters of buffer.
		/// [JP] 入力欄はファイル名の文字列をそのまま編集するので、バッファとして 256 文字に広げる。
		scriptMenuState_.openRequested_ = true;
		scriptMenuState_.focusRequested_ = true;
		scriptMenuState_.scriptType_ = script;
		scriptMenuState_.targetPath_.emplace(parentPath.ChildPath("NewScript"), parentPath.RootPath());
		scriptMenuState_.targetPath_->FilenameText().resize(256);
	}

	/**
	* [EN]
	* Exports a model to a file chosen with a save dialog.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 保存ダイアログで選んだファイルへモデルを書き出す。
	*/
	void ContentsDrawerPanel::ExportModel(const AssetRecord& asset, ExportPreset preset, const Wchar* extension)
	{
		/// [EN] The dialog starts next to the model with the model's name, filtered to the preset's extension.
		/// [JP] ダイアログはモデルの隣、モデルの名前で始め、プリセットの拡張子で絞り込む。
		const FilePath sourcePath(asset.fullpath_.str(), context_.worldContext_.resource_->ProjectRootPath());
		const std::wstring filter = std::wstring(L"*.") + extension;

		std::filesystem::path outputPath;
		if (!FileDialog::SaveFile(outputPath, sourcePath.ParentPath(), filter.c_str(), filter.c_str(), extension, sourcePath.StemPath().wstring().c_str()))
		{
			return;
		}

		D3D12Context& d3d12Context = context_.graphicsContext_.graphics_->GetContext();
		if (!context_.worldContext_.resource_->GetResource<ModelResource>(AssetType::Model)->Export(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), &context_.graphicsContext_.graphics_->GetBindlessHeap(), context_.graphicsContext_.graphics_->GetBC7CompressShader(), *context_.worldContext_.resource_, asset.assetID_, preset, String(outputPath.string())))
		{
			SC_LOG_WARNING("コンテンツドロワー: モデルのエクスポートに失敗しました: {}", asset.path_.c_str());
			return;
		}

		/// [EN] Reload so an export written inside the project shows up as an asset.
		/// [JP] プロジェクト内へ書き出した場合にアセットとして出てくるよう、読み直す。
		context_.worldContext_.resource_->Reload(*context_.worldContext_.loader_, d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), context_.graphicsContext_.graphics_->GetBC7CompressShader());
		SC_LOG_NOTICE("コンテンツドロワー: モデルをエクスポートしました: {}", asset.path_.c_str());
	}
}

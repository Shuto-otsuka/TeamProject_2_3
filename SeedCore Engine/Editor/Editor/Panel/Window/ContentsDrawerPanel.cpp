#include <Editor/Editor/Panel/Window/ContentsDrawerPanel.h>

#include <Editor/Editor/Build/VisualStudioAutomation.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/Window/BootScreenPanel.h>
#include <Editor/Editor/Panel/Window/ConfigPanel.h>
#include <Editor/Editor/Panel/Window/EffectGraphPanel.h>
#include <Editor/Editor/Panel/Window/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/Window/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/Window/ModelTransformPanel.h>

#include <FoundationEngine/File/FileDialog.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Prefab/PrefabSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/ResourceSync.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>

#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/ModelExporter.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
	ContentsDrawerPanel::ContentsDrawerPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), resourceSyncControlPanel_(context, imguiTexture)
	{
		BuildDirectory();
	}

	void ContentsDrawerPanel::Draw()
	{
		ResourceCache* resource = context_.world_.resource_;
		resource->Watch();

		Uint64 syncRevision = context_.application_.resourceSync_ ? context_.application_.resourceSync_->Revision() : 0;
		if (resourceRevision_ != resource->Revision() || syncRevision_ != syncRevision)
		{
			resourceRevision_ = resource->Revision();
			syncRevision_ = syncRevision;
			BuildDirectory();
		}

		ImGui::SetNextWindowDockID(context_.graphics_.imgui_->DockSpaceID(), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("コンテンツドロワー"))
		{
			if (context_.application_.resourceSync_)
			{
				resourceSyncControlPanel_.Draw();
			}

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
			{
				if (ImGui::IsMouseClicked(3) && historyIndex_ > 0)
				{
					--historyIndex_;
				}
				if (ImGui::IsMouseClicked(4) && historyIndex_ + 1 < historyDirectoryList_.size())
				{
					++historyIndex_;
				}
			}

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

			ImVec2 inputMin = ImGui::GetItemRectMin();
			Float iconY = inputMin.y + (ImGui::GetItemRectSize().y - iconSize) * 0.5f;
			ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(imguiTexture_.Icon(String("Misc/Search"))), ImVec2(inputMin.x + paddingX, iconY), ImVec2(inputMin.x + paddingX + iconSize, iconY + iconSize));
			ImGui::Separator();

			if (searchFilter_.IsActive())
			{
				for (const AssetRecord& asset : assetList_)
				{
					if (!searchFilter_.PassFilter(asset.path_.c_str()))
					{
						continue;
					}
					ImGui::PushID(asset.assetID_);

					ImGui::Image(static_cast<ImTextureID>(GetAssetIcon(asset)), ImVec2(iconSize, iconSize));

					if (Uint64 sharingIcon = GetSharingIcon(asset))
					{
						ImVec2 iconMin = ImGui::GetItemRectMin();
						ImVec2 iconMax = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(sharingIcon), ImVec2((iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f), iconMax);
					}

					ImGui::SameLine();
					ImGui::Selectable(asset.path_.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowDoubleClick);

					if ((!context_.application_.resourceSync_ || !context_.application_.resourceSync_->RemoteOnly(asset.assetID_)) && ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload(GetPayloadType(asset.type_), &asset.assetID_, sizeof(Uint32));
						ImGui::Text("%s", FilePath(asset.fullpath_.str(), FileLocator::EngineRoot()).FilenameText().c_str());
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
				ImGui::BeginChild("##DirectoryTree", ImVec2(std::max(ImGui::GetContentRegionAvail().x * 0.3f, 150.0f), 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
				DrawDirectoryTree(root_);
				DrawBackgroundMenu();
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginChild("##AssetList", ImVec2(0, 0), ImGuiChildFlags_Borders);

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

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ACTOR"))
					{
						Actor dropped = *static_cast<const Actor*>(payload->Data);
						const FilePath savedPath(PrefabSystem::SaveToDirectory(dropped, SelectedDirectory().FullPath()), SelectedDirectory().RootPath());
						if (!savedPath.Empty())
						{
							resource->Reload();
							if (Uint32 newAssetID = resource->Find(String(savedPath.RelativeText())))
							{
								PrefabRegistry* prefabs = dropped.GetWorld().AddService<PrefabRegistry>();
								EntityID droppedID = ActorAccessor::GetEntity(dropped).GetID();
								const PrefabRecord* prefabRecord = prefabs->Record(droppedID);
								prefabs->Add(droppedID, newAssetID, prefabRecord ? prefabRecord->fromPrefab_ : false);
							}
						}
					}
					ImGui::EndDragDropTarget();
				}
			}

			DrawScriptMenu();
		}

		ImGui::End();
	}

	void ContentsDrawerPanel::DrawDirectoryTree(FolderNode& node)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | (node.children_.empty() ? ImGuiTreeNodeFlags_Leaf : 0) | (SelectedDirectory().FullPath() == node.path_.FullPath() ? ImGuiTreeNodeFlags_Selected : 0) | (&node == &root_ ? ImGuiTreeNodeFlags_DefaultOpen : 0);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardType_ == ClipboardType::Cut && clipboardPath_.FullPath() == node.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
		ImGui::PushID(node.path_.FilenameText().c_str());

		Bool opened = ImGui::TreeNodeEx("##tree", flags);
		Bool treeClicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
		DrawFolderMenu(node.path_);

		ImGui::SameLine();
		ImGui::Image(static_cast<ImTextureID>(GetFolderIcon(node)), ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));
		ImGui::SameLine();
		if (!DrawRenameMenu(node.path_))
		{
			ImGui::Text("%s", node.path_.FilenameText().c_str());
		}

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

	void ContentsDrawerPanel::DrawAssetListMode(FolderNode& folder)
	{
		Float iconSize = ImGui::GetTextLineHeight();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		for (FolderNode& child : folder.children_ | std::ranges::views::values)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardType_ == ClipboardType::Cut && clipboardPath_.FullPath() == child.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
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

			drawList->AddImage(static_cast<ImTextureID>(GetFolderIcon(child)), iconMin, iconMax);

			ImGui::PopID();
			ImGui::PopStyleVar();
		}

		for (const AssetRecord* asset : folder.assets_)
		{
			const FilePath assetPath(asset->fullpath_.str(), root_.path_.RootPath());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardType_ == ClipboardType::Cut && clipboardPath_.FullPath() == assetPath.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::PushID(asset->assetID_);

			ImGui::Dummy(ImVec2(iconSize, iconSize));
			ImVec2 iconMin = ImGui::GetItemRectMin();
			ImVec2 iconMax = ImGui::GetItemRectMax();
			ImGui::SameLine();

			if (!DrawRenameMenu(assetPath))
			{
				ImGui::Selectable(assetPath.FilenameText().c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowDoubleClick);
			}

			if ((!context_.application_.resourceSync_ || !context_.application_.resourceSync_->RemoteOnly(asset->assetID_)) && ImGui::BeginDragDropSource())
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

			drawList->AddImage(static_cast<ImTextureID>(GetAssetIcon(*asset)), iconMin, iconMax);

			if (Uint64 sharingIcon = GetSharingIcon(*asset))
			{
				drawList->AddImage(static_cast<ImTextureID>(sharingIcon), ImVec2((iconMin.x + iconMax.x) * 0.5f, (iconMin.y + iconMax.y) * 0.5f), iconMax);
			}

			ImGui::PopID();
			ImGui::PopStyleVar();
		}
	}

	void ContentsDrawerPanel::DrawAssetGridMode(FolderNode& folder)
	{
		Float buttonWidth = gridIconSize_ + ImGui::GetStyle().FramePadding.x * 2.0f;
		Int columns = std::max(1, static_cast<Int>(ImGui::GetContentRegionAvail().x / (buttonWidth + ImGui::GetStyle().ItemSpacing.x)));
		Int index = 0;

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

		for (FolderNode& child : folder.children_ | std::ranges::views::values)
		{
			if (index++ % columns != 0)
			{
				ImGui::SameLine();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardType_ == ClipboardType::Cut && clipboardPath_.FullPath() == child.path_.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::BeginGroup();

			ImGui::PushID(child.path_.FilenameText().c_str());
			ImGui::ImageButton("##folder", static_cast<ImTextureID>(GetFolderIcon(child)), ImVec2(gridIconSize_, gridIconSize_));
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

		for (const AssetRecord* asset : folder.assets_)
		{
			if (index++ % columns != 0)
			{
				ImGui::SameLine();
			}

			const FilePath assetPath(asset->fullpath_.str(), root_.path_.RootPath());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, clipboardType_ == ClipboardType::Cut && clipboardPath_.FullPath() == assetPath.FullPath() ? 0.4f : ImGui::GetStyle().Alpha);
			ImGui::BeginGroup();

			ImGui::PushID(asset->assetID_);
			ImGui::ImageButton("##asset", static_cast<ImTextureID>(GetAssetIcon(*asset)), ImVec2(gridIconSize_, gridIconSize_));

			if (Uint64 sharingIcon = GetSharingIcon(*asset))
			{
				ImVec2 badgeMax = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(sharingIcon), ImVec2(badgeMax.x - gridIconSize_ / 3.0f, badgeMax.y - gridIconSize_ / 3.0f), badgeMax);
			}

			if ((!context_.application_.resourceSync_ || !context_.application_.resourceSync_->RemoteOnly(asset->assetID_)) && ImGui::BeginDragDropSource())
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

	void ContentsDrawerPanel::DrawAssetTooltip(const AssetRecord& asset)
	{
		ImGui::BeginTooltip();

		Bool preview = asset.type_ == AssetType::Texture && asset.isLoaded_;
		Float iconSize = preview ? 128.0f : ImGui::GetTextLineHeight();
		ImGui::Image(static_cast<ImTextureID>(GetAssetIcon(asset)), ImVec2(iconSize, iconSize));
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
		if (context_.application_.resourceSync_)
		{
			resourceSyncControlPanel_.DrawState(asset);
		}

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

		if (preview)
		{
			GraphicsResourceSystem& graphicsResources = context_.graphics_.graphics_->Module().Resources();
			Texture* texture = graphicsResources.textureStreaming_->Resolve(graphicsResources.textureResource_->GetHandle(asset.assetID_), context_.application_.uiFrame_);
			if (texture && texture->Resource())
			{
				D3D12_RESOURCE_DESC desc = texture->Resource()->GetDesc();
				ImGui::Text("%llu x %u", desc.Width, desc.Height);
			}
		}

		ImGui::EndTooltip();
	}

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
			if (ImGui::MenuItem("新規エフェクト"))
			{
				CreateNewEffect(folderPath);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("切り取り"))
			{
				clipboardType_ = ClipboardType::Cut;
				clipboardPath_ = folderPath;
			}
			if (ImGui::MenuItem("コピー"))
			{
				clipboardType_ = ClipboardType::Copy;
				clipboardPath_ = folderPath;
			}
			if (ImGui::MenuItem("貼り付け", nullptr, false, clipboardType_ != ClipboardType::None))
			{
				ExecutePaste(folderPath);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("名前変更"))
			{
				renameMenuFocusRequested_ = true;
				renameMenuTargetPath_.emplace(folderPath);
				renameMenuTargetPath_->FilenameText().resize(256);
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

	void ContentsDrawerPanel::DrawAssetMenu(const AssetRecord& asset, const FilePath& assetPath)
	{
		if (ImGui::BeginPopupContextItem("##AssetContext"))
		{
			if (context_.application_.resourceSync_)
			{
				resourceSyncControlPanel_.DrawActions(asset);
			}
			if (ImGui::MenuItem("開く"))
			{
				OpenAssetPopup(asset);
			}

			Asset* assetResource = context_.world_.resource_->Resource(asset.type_);
			Bool cached = asset.type_ == AssetType::Texture || asset.type_ == AssetType::Model || asset.type_ == AssetType::Audio || asset.type_ == AssetType::Movie || asset.type_ == AssetType::Skymap || asset.type_ == AssetType::Effekseer;
			if (assetResource && cached && ImGui::BeginMenu("アセットアクション"))
			{
				if (ImGui::MenuItem("キャッシュファイルを作成"))
				{
					GenerateAsset(asset, *assetResource, "キャッシュファイル");
				}

				GraphicsResourceSystem& resources = context_.graphics_.graphics_->Module().Resources();
				if (asset.type_ == AssetType::Model)
				{
					if (ImGui::MenuItem("コリジョン生成"))
					{
						GenerateAsset(asset, *resources.meshCollisionResource_, "コリジョン");
					}
					if (ImGui::MenuItem("マテリアル生成"))
					{
						GenerateAsset(asset, *resources.materialResource_, "マテリアル");
					}
					if (ImGui::MenuItem("スケルトン生成"))
					{
						GenerateAsset(asset, *resources.skeletonResource_, "スケルトン");
					}
					if (ImGui::MenuItem("アニメーション抽出"))
					{
						GenerateAsset(asset, *resources.animationResource_, "アニメーション");
					}
					if (ImGui::MenuItem("モデル変換"))
					{
						context_.panel_.tools_->Panel<ModelTransformPanel>().Open(asset.assetID_);
					}
				}

				if (asset.type_ == AssetType::Model && ImGui::BeginMenu("エクスポート"))
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
				clipboardType_ = ClipboardType::Cut;
				clipboardPath_ = assetPath;
			}
			if (ImGui::MenuItem("コピー"))
			{
				clipboardType_ = ClipboardType::Copy;
				clipboardPath_ = assetPath;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("名前変更"))
			{
				renameMenuFocusRequested_ = true;
				renameMenuTargetPath_.emplace(assetPath);
				renameMenuTargetPath_->FilenameText().resize(256);
			}
			if (ImGui::MenuItem("削除"))
			{
				ExecuteDelete(assetPath);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("エクスプローラーで表示"))
			{
				std::wstring param = L"/select,\"" + assetPath.FullPath().wstring() + L"\"";
				ShellExecuteW(NULL, L"open", L"explorer.exe", param.c_str(), NULL, SW_SHOWNORMAL);
			}

			ImGui::EndPopup();
		}
	}

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
			if (ImGui::MenuItem("新規エフェクト"))
			{
				CreateNewEffect(directoryPath);
			}
			if (ImGui::MenuItem("貼り付け", nullptr, false, clipboardType_ != ClipboardType::None))
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

	void ContentsDrawerPanel::DrawScriptMenu()
	{
		const Char* title = scriptMenuType_ == ScriptType::Csharp ? "新規 C# スクリプト" : "新規 C++ スクリプト";
		if (scriptMenuOpenRequested_)
		{
			ImGui::OpenPopup(title);
			scriptMenuOpenRequested_ = false;
		}

		Bool open = true;
		if (ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (scriptMenuFocusRequested_)
			{
				ImGui::SetKeyboardFocusHere();
				scriptMenuFocusRequested_ = false;
			}

			std::string& nameBuffer = scriptMenuTargetPath_->FilenameText();
			Bool confirmed = ImGui::InputText("スクリプト名", nameBuffer.data(), nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

			ImGui::Separator();

			if (ImGui::Button("作成") || confirmed)
			{
				const FilePath& targetPath = *scriptMenuTargetPath_;
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

	Bool ContentsDrawerPanel::DrawRenameMenu(const FilePath& itemPath)
	{
		if (!renameMenuTargetPath_ || renameMenuTargetPath_->FullPath() != itemPath.FullPath())
		{
			return false;
		}

		if (renameMenuFocusRequested_)
		{
			ImGui::SetKeyboardFocusHere();
			renameMenuFocusRequested_ = false;
		}

		std::string& nameBuffer = renameMenuTargetPath_->FilenameText();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		Bool confirmed = ImGui::InputText("##InlineRename", nameBuffer.data(), nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopStyleVar();

		if (confirmed && nameBuffer.front() != '\0')
		{
			ExecuteRename(*renameMenuTargetPath_);
		}
		if (confirmed || ImGui::IsItemDeactivated())
		{
			renameMenuTargetPath_.reset();
		}

		return true;
	}

	void ContentsDrawerPanel::SelectDirectory(const FilePath& directoryPath)
	{
		if (SelectedDirectory().FullPath() == directoryPath.FullPath())
		{
			return;
		}

		historyDirectoryList_.erase(historyDirectoryList_.begin() + historyIndex_ + 1, historyDirectoryList_.end());
		historyDirectoryList_.push_back(directoryPath);
		historyIndex_ = historyDirectoryList_.size() - 1;
	}

	const FilePath& ContentsDrawerPanel::SelectedDirectory()const
	{
		return historyDirectoryList_[historyIndex_];
	}

	void ContentsDrawerPanel::BuildDirectory()
	{
		static const std::set<std::string> excludeDirectories =
		{
			"AIEngine", "AudioEngine", "CompiledShaderObject", "Editor",
			"External", "FoundationEngine", "GraphicsEngine", "Launcher", "Logs",
			"Package", "PhysicsEngine", "Platform", "Runtime", "SeedCore", "Tools",
			".vs", "x64", ".git", ".asset",
		};

		const std::filesystem::path& projectRoot = FileLocator::EngineRoot();
		root_ = {};
		root_.path_ = FilePath(projectRoot, projectRoot);

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

		assetList_.clear();
		for (const AssetRecord& asset : context_.world_.resource_->Records() | std::ranges::views::values)
		{
			assetList_.push_back(asset);
		}
		if (context_.application_.resourceSync_)
		{
			DynamicArray<AssetRecord> remoteAssets;
			context_.application_.resourceSync_->Gather(remoteAssets);
			for (const AssetRecord& remote : remoteAssets)
			{
				if (!std::ranges::any_of(assetList_, [&remote](const AssetRecord& local) { return local.assetID_ == remote.assetID_; }))
				{
					assetList_.push_back(remote);
				}
			}
		}

		for (const AssetRecord& asset : assetList_)
		{
			insertDirectory(std::filesystem::path(asset.path_.str()).parent_path())->assets_.push_back(&asset);
		}

		if (historyDirectoryList_.empty())
		{
			historyDirectoryList_.push_back(root_.path_);
		}
	}

	Uint64 ContentsDrawerPanel::GetAssetIcon(const AssetRecord& asset)const
	{
		if (asset.type_ == AssetType::Texture && asset.isLoaded_)
		{
			if (thumbnailCache_.contains(asset.assetID_))
			{
				return thumbnailCache_.at(asset.assetID_);
			}

			BindlessHeap& bindlessHeap = context_.graphics_.graphics_->Device().ResourceHeap();
			GraphicsResourceSystem& graphicsResources = context_.graphics_.graphics_->Module().Resources();
			Texture* texture = graphicsResources.textureStreaming_->Resolve(graphicsResources.textureResource_->GetHandle(asset.assetID_), context_.application_.uiFrame_);
			if (texture && texture->Resource())
			{
				texture->Pin();

				Uint descIndex = bindlessHeap.AllocateIndex();
				context_.graphics_.graphics_->Device().Context().GetDevice()->CreateShaderResourceView(texture->Resource(), nullptr, bindlessHeap.CPUHandle(descIndex));
				Uint64 textureID = static_cast<Uint64>(bindlessHeap.GPUHandle(descIndex).ptr);
				thumbnailCache_.insert({ asset.assetID_, textureID });
				return textureID;
			}
		}

		static const FlatMap<String, String> icons =
		{
			{ String(".texture"), String("Asset/Engine/Texture") },
			{ String(".fbx"), String("Asset/Source/Model") },
			{ String(".gltf"), String("Asset/Source/Model") },
			{ String(".glb"), String("Asset/Source/Model") },
			{ String(".crister"), String("Asset/Engine/Model") },
			{ String(".animation"), String("Asset/Engine/Animation") },
			{ String(".collision"), String("Asset/Engine/MeshCollision") },
			{ String(".material"), String("Asset/Engine/Material") },
			{ String(".skeleton"), String("Asset/Engine/Skeleton") },
			{ String(".navmesh"), String("Asset/Engine/Navmesh") },
			{ String(".zephyr"), String("Asset/Engine/Effect") },
			{ String(".efk"), String("Asset/Source/Effekseer") },
			{ String(".efkefc"), String("Asset/Source/Effekseer") },
			{ String(".effekseer"), String("Asset/Engine/Effekseer") },
			{ String(".efkmodel"), String("Asset/Source/EffekseerModel") },
			{ String(".efkmat"), String("Asset/Source/EffekseerMaterial") },
			{ String(".efkcurve"), String("Asset/Source/EffekseerCurve") },
			{ String(".mp3"), String("Asset/Source/Audio") },
			{ String(".wav"), String("Asset/Source/Audio") },
			{ String(".acb"), String("Asset/Source/Audio") },
			{ String(".awb"), String("Asset/Source/Audio") },
			{ String(".audio"), String("Asset/Engine/Audio") },
			{ String(".ttf"), String("Asset/Source/Font") },
			{ String(".otf"), String("Asset/Source/Font") },
			{ String(".ttc"), String("Asset/Source/Font") },
			{ String(".mp4"), String("Asset/Source/Movie") },
			{ String(".movie"), String("Asset/Engine/Movie") },
			{ String(".hdr"), String("Asset/Source/Sky") },
			{ String(".skymap"), String("Asset/Engine/Sky") },
			{ String(".prefab"), String("Asset/Engine/Prefab") },
			{ String(".scene"), String("Asset/Engine/Scene") },
			{ String(".scg"), String("Asset/Engine/Config") },
			{ String(".h"), String("Asset/Source/Header") },
			{ String(".cpp"), String("Asset/Source/CPlusPlus") },
			{ String(".cs"), String("Asset/Source/CSharp") },
			{ String(".hlsl"), String("Asset/Source/Hlsl") },
			{ String(".hlsli"), String("Asset/Source/Hlsl") },
		};

		String extension = String(std::filesystem::path(asset.path_.c_str()).extension().string());
		auto found = icons.find(extension);
		if (found != icons.end())
		{
			return imguiTexture_.Icon(found->second);
		}
		return imguiTexture_.Icon(String("Asset/Source/Text"));
	}

	Uint64 ContentsDrawerPanel::GetSharingIcon(const AssetRecord& asset)const
	{
		if (!context_.application_.resourceSync_)
		{
			return 0;
		}

		const SharedAsset* shared = context_.application_.resourceSync_->GetAsset(asset.assetID_);
		if (!shared)
		{
			return 0;
		}

		if (context_.application_.resourceSync_->Conflicted(asset.assetID_))
		{
			return imguiTexture_.Icon(String("Sharing/CloudWarning"));
		}

		if (context_.application_.resourceSync_->Modified(asset.assetID_))
		{
			return imguiTexture_.Icon(String("Sharing/CloudUpload"));
		}

		if (context_.application_.resourceSync_->RemoteOnly(asset.assetID_))
		{
			return imguiTexture_.Icon(String("Sharing/CloudDownload"));
		}

		return imguiTexture_.Icon(String("Sharing/Cloud"));
	}

	Uint64 ContentsDrawerPanel::GetFolderIcon(const FolderNode& node)const
	{
		return imguiTexture_.Icon(node.children_.empty() && node.assets_.empty() ? String("Folder/FolderNoItem") : String("Folder/FolderInItem"));
	}

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
		case AssetType::Effekseer:
			return "ASSET_EFFEKSEER";
		case AssetType::Houdini:
			return "ASSET_HOUDINI";
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

	void ContentsDrawerPanel::GenerateAsset(const AssetRecord& asset, Asset& resource, const Char* label)
	{
		ResourceCache& cache = *context_.world_.resource_;

		DynamicArray<String> writtenPaths;
		Bool succeeded = resource.Generate(AssetContext{ cache, LoadThread::Main }, asset.assetID_, writtenPaths);

		DynamicArray<String> assetPaths;
		for (const String& writtenPath : writtenPaths)
		{
			std::string relativePath = std::filesystem::relative(std::filesystem::path(writtenPath.c_str()), FileLocator::EngineRoot()).string();
			std::ranges::replace(relativePath, '\\', '/');
			assetPaths.push_back(String(relativePath));
		}

		DynamicArray<Uint32> overwrittenIDs;
		for (const String& assetPath : assetPaths)
		{
			Uint32 id = cache.Find(assetPath);
			AssetRecord* record = id == 0 ? nullptr : cache.Record(id);
			if (record && record->path_ == assetPath)
			{
				overwrittenIDs.push_back(id);
			}
		}
		if (!writtenPaths.empty())
		{
			cache.Reload();
		}
		for (Uint32 id : overwrittenIDs)
		{
			cache.Reload(id);
		}

		ResourceSync* resourceSync = context_.application_.resourceSync_;
		if (resourceSync)
		{
			for (const String& assetPath : assetPaths)
			{
				Uint32 id = cache.Find(assetPath);
				AssetRecord* record = id == 0 ? nullptr : cache.Record(id);
				if (!record || record->path_ != assetPath)
				{
					continue;
				}

				if (resourceSync->GetAsset(id))
				{
					resourceSync->RequestPublish(id);
				}
				else
				{
					resourceSync->RequestRegister(*record);
				}
			}
		}

		if (!succeeded)
		{
			SC_LOG_WARNING("コンテンツドロワー: {}の作成に失敗しました: {}", label, asset.path_.c_str());
			return;
		}
		SC_LOG_NOTICE("コンテンツドロワー: {}を作成しました（{} 件）: {}", label, writtenPaths.size(), asset.path_.c_str());
	}

	void ContentsDrawerPanel::ExecuteDelete(const FilePath& targetPath)
	{
		std::error_code errorCode;
		std::filesystem::remove_all(targetPath.FullPath(), errorCode);
		std::filesystem::remove(targetPath.AppendedSuffixPath(".meta"), errorCode);

		if (clipboardPath_.FullPath() == targetPath.FullPath())
		{
			clipboardType_ = ClipboardType::None;
		}
	}

	void ContentsDrawerPanel::ExecuteRename(const FilePath& targetPath)
	{
		if (context_.application_.resourceSync_ && context_.application_.resourceSync_->Managed(targetPath.FullPath()))
		{
			SC_LOG_WARNING("コンテンツドロワー: 共有コンテンツはローカルのファイル操作で名前変更できません");
			return;
		}

		const FilePath renamedPath(targetPath.SiblingPath(targetPath.FilenameText().c_str()), targetPath.RootPath());
		std::error_code errorCode;
		std::filesystem::rename(targetPath.FullPath(), renamedPath.FullPath(), errorCode);
		std::filesystem::rename(targetPath.AppendedSuffixPath(".meta"), renamedPath.AppendedSuffixPath(".meta"), errorCode);

		if (clipboardPath_.FullPath() == targetPath.FullPath())
		{
			clipboardPath_ = renamedPath;
		}
	}

	void ContentsDrawerPanel::ExecutePaste(const FilePath& directoryPath)
	{
		const FilePath& sourcePath = clipboardPath_;
		if (clipboardType_ == ClipboardType::None || !std::filesystem::exists(sourcePath.FullPath()))
		{
			clipboardType_ = ClipboardType::None;
			return;
		}

		const FilePath destPath(directoryPath.ChildPath(sourcePath.FilenamePath()), directoryPath.RootPath());
		if (context_.application_.resourceSync_ && (context_.application_.resourceSync_->Managed(sourcePath.FullPath()) || context_.application_.resourceSync_->Managed(destPath.FullPath())))
		{
			SC_LOG_WARNING("コンテンツドロワー: 共有コンテンツはローカルのクリップボード操作で移動・コピーできません");
			return;
		}

		std::error_code errorCode;
		if (clipboardType_ == ClipboardType::Cut)
		{
			std::filesystem::rename(sourcePath.FullPath(), destPath.FullPath(), errorCode);
			std::filesystem::rename(sourcePath.AppendedSuffixPath(".meta"), destPath.AppendedSuffixPath(".meta"), errorCode);
			clipboardType_ = ClipboardType::None;
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

	Bool ContentsDrawerPanel::ExecuteRegister(const FilePath& headerPath, const FilePath& cppPath)
	{
		const std::filesystem::path& projectRoot = FileLocator::EngineRoot();

		if (VisualStudioAutomation::TryAddFile(projectRoot / "Runtime" / "Runtime.sln", "UserProject.Cplusplus", headerPath.FullPath(), cppPath.FullPath()))
		{
			return true;
		}

		std::wstring commandLine = std::format(L"\"{}\" \"{}\" \"{}\" \"{}\" \"{}\"", (projectRoot / "Platform" / "Python" / "python.exe").wstring(), (projectRoot / "Tools" / "Python" / "CreateScript.py").wstring(), projectRoot.wstring(), headerPath.RelativePath().wstring(), cppPath.RelativePath().wstring());

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};
		if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
		{
			return false;
		}

		WaitForSingleObject(processInfo.hProcess, INFINITE);
		DWORD exitCode = 1;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return exitCode == 0;
	}

	void ContentsDrawerPanel::CreateNewFolder(const FilePath& targetPath)
	{
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

		renameMenuFocusRequested_ = true;
		renameMenuTargetPath_.emplace(newPath, targetPath.RootPath());
		renameMenuTargetPath_->FilenameText().resize(256);
	}

	void ContentsDrawerPanel::CreateNewEffect(const FilePath& folderPath)
	{
		std::filesystem::path folder = folderPath.FullPath();
		std::filesystem::path newPath = folder / "NewEffect.zephyr";
		for (Int index = 2;std::filesystem::exists(newPath);++index)
		{
			newPath = folder / std::format("NewEffect({}).zephyr", index);
		}

		std::ofstream file(newPath, std::ios::binary);
		if (!file)
		{
			SC_LOG_WARNING("コンテンツドロワー: エフェクトの作成に失敗しました: {}", newPath.string());
			return;
		}
		file.close();

		ResourceCache& cache = *context_.world_.resource_;
		cache.Reload();
		Uint32 assetID = cache.Find(String(std::filesystem::relative(newPath, FileLocator::EngineRoot()).generic_string()));
		if (assetID == 0)
		{
			return;
		}

		context_.panel_.tools_->Panel<EffectGraphPanel>().Open(assetID);
	}

	void ContentsDrawerPanel::CreateNewScript(const FilePath& targetPath)
	{
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

		const FilePath parentPath(targetPath.ParentPath(), targetPath.RootPath() / "UserProject");
		const std::filesystem::path targetDirectory = *parentPath.RelativePath().begin() == ".." ? parentPath.RootPath() / "Script" : parentPath.FullPath();

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

		const std::filesystem::path solutionPath = targetPath.RootPath() / "Runtime" / "Runtime.sln";
		auto openScript = [&solutionPath](const FilePath& scriptPath)
		{
			if (!VisualStudioAutomation::TryOpenFile(solutionPath, scriptPath.FullPath()))
			{
				ShellExecuteW(NULL, L"open", scriptPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		};

		if (scriptMenuType_ == ScriptType::Csharp)
		{
			const FilePath scriptPath(targetDirectory / (finalName + ".cs"), parentPath.RootPath());
			std::ofstream file(scriptPath.FullPath(), std::ios::binary);
			file << "using System;\r\nusing SeedCore;\r\n\r\npublic class " << finalName << " : SeedScript\r\n{\r\n\tvoid OnStart()\r\n\t{\r\n\t}\r\n\r\n\tvoid OnTick(Single elapsedTime)\r\n\t{\r\n\t}\r\n}\r\n";
			file.close();
			SC_LOG_NOTICE("コンテンツドロワー: C# スクリプトを作成しました: {}", finalName);
			openScript(scriptPath);
			return;
		}

		const FilePath headerPath(targetDirectory / (finalName + ".h"), parentPath.RootPath());
		const FilePath cppPath(targetDirectory / (finalName + ".cpp"), parentPath.RootPath());
		std::ofstream headerFile(headerPath.FullPath(), std::ios::binary);
		headerFile << "#pragma once\n#include <FoundationEngine/Prelude.h>\n#include <FoundationEngine/SeedScript.h>\n\nclass " << finalName << " :public SeedCore::SeedScript\n{\npublic:\n\tvoid OnStart();\n\tvoid OnTick(float elapsedTime);\n};\nREGISTER_COMPONENT(" << finalName << ");\n";
		std::ofstream cppFile(cppPath.FullPath(), std::ios::binary);
		cppFile << "#include \"UserProject/" << headerPath.RelativeText() << "\"\n\nvoid " << finalName << "::OnStart()\n{\n}\n\nvoid " << finalName << "::OnTick(float elapsedTime)\n{\n}\n";

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

	void ContentsDrawerPanel::OpenAssetPopup(const AssetRecord& asset)
	{
		if (context_.application_.resourceSync_ && context_.application_.resourceSync_->RemoteOnly(asset.assetID_))
		{
			context_.application_.resourceSync_->RequestGet(asset.assetID_);
			return;
		}
		if (asset.type_ == AssetType::Scene)
		{
			context_.scene_.request_ = asset.assetID_;
			return;
		}
		if (asset.type_ == AssetType::Material)
		{
			context_.panel_.tools_->Panel<MaterialViewerPanel>().Open();
			return;
		}
		if (asset.type_ == AssetType::Effect)
		{
			context_.panel_.tools_->Panel<EffectGraphPanel>().Open(asset.assetID_);
			return;
		}

		std::filesystem::path assetFile(asset.path_.c_str());
		if (assetFile.extension() == ".scg")
		{
			std::string name = assetFile.stem().string();
			if (name == "LayerBindings")
			{
				context_.panel_.tools_->Panel<LayerSettingsPanel>().Open();
				return;
			}
			if (name == "BootConfig")
			{
				context_.panel_.tools_->Panel<BootScreenPanel>().Open();
				return;
			}

			ConfigPanel::ConfigCategory category = ConfigPanel::ConfigCategory::Game;
			if (name == "AudioBindings")
			{
				category = ConfigPanel::ConfigCategory::Audio;
			}
			else if (name == "IconBindings")
			{
				category = ConfigPanel::ConfigCategory::Icon;
			}
			else if (name == "InputBindings")
			{
				category = ConfigPanel::ConfigCategory::Input;
			}
			context_.panel_.tools_->Panel<ConfigPanel>().Open(category);
			return;
		}

		const FilePath assetPath(asset.fullpath_.str(), FileLocator::EngineRoot());
		ShellExecuteW(NULL, L"open", assetPath.FullPath().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	void ContentsDrawerPanel::OpenScriptPopup(const FilePath& parentPath, ScriptType script)
	{
		scriptMenuOpenRequested_ = true;
		scriptMenuFocusRequested_ = true;
		scriptMenuType_ = script;
		scriptMenuTargetPath_.emplace(parentPath.ChildPath("NewScript"), parentPath.RootPath());
		scriptMenuTargetPath_->FilenameText().resize(256);
	}

	void ContentsDrawerPanel::ExportModel(const AssetRecord& asset, ExportPreset preset, const Wchar* extension)
	{
		const FilePath sourcePath(asset.fullpath_.str(), FileLocator::EngineRoot());
		const std::wstring filter = std::wstring(L"*.") + extension;

		std::filesystem::path outputPath;
		if (!FileDialog::SaveFile(outputPath, sourcePath.ParentPath(), filter.c_str(), filter.c_str(), extension, sourcePath.StemPath().wstring().c_str()))
		{
			return;
		}

		ModelExporter& modelExporter = *context_.graphics_.graphics_->Module().Exporters().modelExporter_;
		modelExporter.Preset(preset);
		if (!modelExporter.Export(AssetContext{ *context_.world_.resource_, LoadThread::Main }, asset.assetID_, String(outputPath.string())))
		{
			SC_LOG_WARNING("コンテンツドロワー: モデルのエクスポートに失敗しました: {}", asset.path_.c_str());
			return;
		}

		context_.world_.resource_->Reload();
		SC_LOG_NOTICE("コンテンツドロワー: モデルをエクスポートしました: {}", asset.path_.c_str());
	}
}

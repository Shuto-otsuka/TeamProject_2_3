#include <Editor/Editor/Panel/MenuBarPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/Build/AtomCraft.h>
#include <Editor/Editor/Panel/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/AnimatorControllerPanel.h>
#include <Editor/Editor/Panel/TimelinePanel.h>
#include <Editor/Editor/Panel/SkeletonControllerPanel.h>
#include <Editor/Editor/Panel/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/ModelTransformPanel.h>
#include <Editor/Editor/Panel/AvatarPanel.h>
#include <Editor/Editor/Panel/BootScreenPanel.h>
#include <Editor/Editor/Panel/ShortCutKeyPanel.h>
#include <Editor/Editor/Panel/SpecMemoPanel.h>
#include <Editor/Editor/Panel/DiagnosticsPanel.h>
#include <Editor/Editor/Panel/TodoListPanel.h>
#include <Editor/Editor/Panel/VersionPanel.h>
#include <Editor/Editor/Panel/ConfigPanel.h>
#include <FoundationEngine/File/FileDialog.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	MenuBarPanel::MenuBarPanel(EditorContext& context) : context_(context), graphicsMenuPanel_(context)
	{
		/// No Code
	}

	void MenuBarPanel::Draw()
	{
		const Bool isPlaying = context_.world_.timer_->Playing();

		if (context_.scene_.request_ != 0)
		{
			Uint32 assetID = context_.scene_.request_;
			context_.scene_.request_ = 0;
			if (!isPlaying)
			{
				RequestSceneSwitch(PendingSceneOp::OpenAsset, {}, assetID);
			}
		}

		if (context_.application_.exitRequested_)
		{
			context_.application_.exitRequested_ = false;
			RequestSceneSwitch(PendingSceneOp::Exit, {}, 0);
		}

		ImGuiIO& io = ImGui::GetIO();
		if (!isPlaying && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_N, false))
		{
			RequestSceneSwitch(PendingSceneOp::New, {}, 0);
		}
		if (!isPlaying && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O, false))
		{
			OpenScene();
		}
		if (!isPlaying && io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			SaveScene();
		}
		if (!isPlaying && io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			OverwriteSaveScene();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F1, false))
		{
			context_.panel_.shortCutKey_->Open();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F2, false))
		{
			context_.panel_.specMemo_->Open();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F3, false))
		{
			context_.panel_.diagnostics_->ShowConsoleTab();
		}
		if (io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_F4, false))
		{
			RequestSceneSwitch(PendingSceneOp::Exit, {}, 0);
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F4, false))
		{
			context_.panel_.diagnostics_->ShowProfilerTab();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F8, false))
		{
			context_.panel_.todoList_->Open();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F9, false))
		{
			context_.panel_.version_->Open();
		}

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("ファイル"))
			{
				ImGui::BeginDisabled(isPlaying);
				if (ImGui::MenuItem("Runtimeビルドでexeを出力"))
				{
					BuildRuntime();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("新規", "Ctrl+N"))
				{
					RequestSceneSwitch(PendingSceneOp::New, {}, 0);
				}
				if (ImGui::MenuItem("開く", "Ctrl+O"))
				{
					OpenScene();
				}
				if (ImGui::MenuItem("保存", "Ctrl+Shift+S"))
				{
					SaveScene();
				}
				if (ImGui::MenuItem("上書き保存", "Ctrl+S"))
				{
					OverwriteSaveScene();
				}
				ImGui::EndDisabled();
				ImGui::Separator();
				if (ImGui::MenuItem("終了", "Alt+F4"))
				{
					RequestSceneSwitch(PendingSceneOp::Exit, {}, 0);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("編集"))
			{
				if (ImGui::MenuItem("元に戻す", "Ctrl+Z", false, !isPlaying))
				{
					context_.scene_.history_.Undo();
				}
				if (ImGui::MenuItem("やり直す", "Ctrl+Y", false, !isPlaying))
				{
					context_.scene_.history_.Redo();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("レイヤー編集"))
				{
					context_.panel_.layerSettings_->Open();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("表示"))
			{
				if (ImGui::MenuItem("アニメーターコントローラー"))
				{
					Animator* animator = context_.selection_.Primary() ? const_cast<Animator*>(context_.selection_.Primary().GetComponent<Animator>()) : nullptr;
					context_.panel_.animatorController_->Open(animator);
				}
				if (ImGui::MenuItem("タイムライン"))
				{
					context_.panel_.timeline_->Open();
				}
				if (ImGui::MenuItem("スケルトンコントローラー"))
				{
					context_.panel_.skeletonController_->Open();
				}
				if (ImGui::MenuItem("マテリアルビューア"))
				{
					context_.panel_.materialViewer_->Open();
				}
				if (ImGui::MenuItem("モデル変換"))
				{
					context_.panel_.modelTransform_->Open();
				}
				ImGui::EndMenu();
			}

			graphicsMenuPanel_.Draw();

			if (ImGui::BeginMenu("ツール"))
			{
				if (ImGui::MenuItem("アバター生成"))
				{
					context_.panel_.avatar_->Open();
				}
				if (ImGui::MenuItem("起動ローディング画面"))
				{
					context_.panel_.bootScreen_->Open();
				}
				if (ImGui::BeginMenu("CRI ADX2"))
				{
					if (ImGui::MenuItem("AtomCraft を開く"))
					{
						std::filesystem::path projectPath;
						if (FileDialog::OpenFile(projectPath, std::filesystem::current_path(), L"Atom Craft Project (*.atmcproject)", L"*.atmcproject"))
						{
							AtomCraft::Open(context_.config_.editor_->atomCraftPath_, String(projectPath.generic_string()));
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("ヘルプ"))
			{
				if (ImGui::MenuItem("ショートカットキー一覧", "Ctrl+F1"))
				{
					context_.panel_.shortCutKey_->Open();
				}
				if (ImGui::MenuItem("仕様メモ", "Ctrl+F2"))
				{
					context_.panel_.specMemo_->Open();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("コンソール", "Ctrl+F3"))
				{
					context_.panel_.diagnostics_->ShowConsoleTab();
				}
				if (ImGui::MenuItem("パフォーマンスプロファイラー", "Ctrl+F4"))
				{
					context_.panel_.diagnostics_->ShowProfilerTab();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("ToDoリスト", "Ctrl+F8"))
				{
					context_.panel_.todoList_->Open();
				}
				if (ImGui::MenuItem("バージョン情報", "Ctrl+F9"))
				{
					context_.panel_.version_->Open();
				}
				ImGui::Separator();
				if (ImGui::MenuItem("エンジン/ゲーム構成設定"))
				{
					context_.panel_.config_->Open();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (pendingSceneOp_ != PendingSceneOp::None && !ImGui::IsPopupOpen("未保存の変更"))
		{
			ImGui::OpenPopup("未保存の変更");
		}

		if (ImGui::BeginPopupModal("未保存の変更", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("現在のシーンを保存しますか？");
			ImGui::Separator();

			if (ImGui::Button("保存"))
			{
				OverwriteSaveScene();
				ExecutePendingSceneOp();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("保存しない"))
			{
				ExecutePendingSceneOp();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("キャンセル"))
			{
				pendingSceneOp_ = PendingSceneOp::None;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (runtimeBuilder_.IsBuilding() && !ImGui::IsPopupOpen("Runtimeビルド中"))
		{
			ImGui::OpenPopup("Runtimeビルド中");
		}

		if (ImGui::BeginPopupModal("Runtimeビルド中", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::Text("Runtimeをビルドしています...");
			ImGui::ProgressBar(runtimeBuilder_.GetProgress(), ImVec2(300, 0));

			if (!runtimeBuilder_.IsBuilding())
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		Bool buildSuccess = false;
		std::string buildLog;
		if (runtimeBuilder_.ConsumeResult(buildSuccess, buildLog))
		{
			if (buildSuccess)
			{
				SC_LOG_NOTICE("Runtimeビルドが完了しました");
			}
			else
			{
				SC_LOG_WARNING("Runtimeビルドに失敗しました: {}", buildLog);
			}
		}
	}

	void MenuBarPanel::BuildRuntime()
	{
		if (runtimeBuilder_.IsBuilding())
		{
			return;
		}

		SC_LOG_NOTICE("Runtimeビルドを開始します");
		runtimeBuilder_.BuildAsync(context_.world_.resource_->ProjectRootPath());
	}

	void MenuBarPanel::RequestSceneSwitch(PendingSceneOp op, const std::filesystem::path& path, Uint32 assetID)
	{
		pendingSceneOp_ = op;
		pendingScenePath_ = path;
		pendingSceneAssetID_ = assetID;

		if (context_.world_.world_->GetActors().empty())
		{
			ExecutePendingSceneOp();
		}
	}

	void MenuBarPanel::ExecutePendingSceneOp()
	{
		PendingSceneOp op = pendingSceneOp_;
		std::filesystem::path path = pendingScenePath_;
		Uint32 assetID = pendingSceneAssetID_;

		pendingSceneOp_ = PendingSceneOp::None;
		pendingScenePath_.clear();
		pendingSceneAssetID_ = 0;

		switch (op)
		{
		case PendingSceneOp::New:
			NewScene();
			break;
		case PendingSceneOp::OpenPath:
		{
			SceneVisual visual;
			if (!Scene::Load(*context_.world_.world_, *context_.world_.resource_, path, &visual))
			{
				SC_LOG_WARNING("シーンの読み込みに失敗しました: {}", path.string());
				break;
			}
			context_.sceneVisual_.raytracing_ = DeserializeRaytracingContext(visual.raytracing_);
			context_.sceneVisual_.screenSpace_ = DeserializeScreenSpaceContext(visual.screenSpace_);
			context_.sceneVisual_.rasterization_ = DeserializeRasterizationContext(visual.rasterization_);
			context_.sceneVisual_.qualityPreset_ = GraphicsQualityPreset::Custom;
			context_.scene_.path_ = FilePath(path, context_.world_.resource_->ProjectRootPath());
			context_.selection_.Clear();
			context_.scene_.history_.Clear();
			SC_LOG_NOTICE("シーンを読み込みました: {}", path.string());
			break;
		}
		case PendingSceneOp::OpenAsset:
		{
			SceneVisual visual;
			if (!Scene::Load(*context_.world_.world_, *context_.world_.resource_, assetID, &visual))
			{
				SC_LOG_WARNING("シーンの読み込みに失敗しました(assetID: {})", assetID);
				break;
			}
			context_.sceneVisual_.raytracing_ = DeserializeRaytracingContext(visual.raytracing_);
			context_.sceneVisual_.screenSpace_ = DeserializeScreenSpaceContext(visual.screenSpace_);
			context_.sceneVisual_.rasterization_ = DeserializeRasterizationContext(visual.rasterization_);
			context_.sceneVisual_.qualityPreset_ = GraphicsQualityPreset::Custom;
			context_.scene_.path_ = FilePath(context_.world_.resource_->GetAsset(assetID)->fullpath_.c_str(), context_.world_.resource_->ProjectRootPath());
			context_.selection_.Clear();
			context_.scene_.history_.Clear();
			SC_LOG_NOTICE("シーンを読み込みました: {}", context_.scene_.path_.FullPath().string());
			break;
		}
		case PendingSceneOp::Exit:
			PostQuitMessage(0);
			break;
		default:
			break;
		}
	}

	void MenuBarPanel::NewScene()
	{
		context_.world_.world_->DestroyActors();

		context_.scene_.path_ = FilePath();
		context_.selection_.Clear();
		context_.scene_.history_.Clear();

		SC_LOG_NOTICE("新規シーンを作成しました");
	}

	void MenuBarPanel::OpenScene()
	{
		std::filesystem::path sceneDir = context_.world_.resource_->ProjectRootPath() / "UserProject" / "Assets" / "Scene";
		std::filesystem::create_directories(sceneDir);

		std::filesystem::path selectedPath;
		if (!FileDialog::OpenFile(selectedPath, sceneDir, L"Scene Files (*.scene)", L"*.scene"))
		{
			return;
		}

		RequestSceneSwitch(PendingSceneOp::OpenPath, selectedPath, 0);
	}

	void MenuBarPanel::SaveScene()
	{
		std::filesystem::path sceneDir = context_.world_.resource_->ProjectRootPath() / "UserProject" / "Assets" / "Scene";
		std::filesystem::create_directories(sceneDir);

		std::filesystem::path savePath;
		if (!FileDialog::SaveFile(savePath, sceneDir, L"Scene Files (*.scene)", L"*.scene", L"scene"))
		{
			return;
		}

		if (!Scene::Save(*context_.world_.world_, *context_.world_.resource_, savePath, SceneVisual{ SerializeRaytracingContext(context_.sceneVisual_.raytracing_), SerializeScreenSpaceContext(context_.sceneVisual_.screenSpace_), SerializeRasterizationContext(context_.sceneVisual_.rasterization_) }))
		{
			SC_LOG_WARNING("シーンの保存に失敗しました: {}", savePath.string());
			return;
		}

		context_.scene_.path_ = FilePath(savePath, context_.world_.resource_->ProjectRootPath());

		SC_LOG_NOTICE("シーンを保存しました: {}", savePath.string());
	}

	void MenuBarPanel::OverwriteSaveScene()
	{
		if (context_.scene_.path_.Empty())
		{
			SaveScene();
			return;
		}

		if (!Scene::Save(*context_.world_.world_, *context_.world_.resource_, context_.scene_.path_.FullPath(), SceneVisual{ SerializeRaytracingContext(context_.sceneVisual_.raytracing_), SerializeScreenSpaceContext(context_.sceneVisual_.screenSpace_), SerializeRasterizationContext(context_.sceneVisual_.rasterization_) }))
		{
			SC_LOG_WARNING("シーンの上書き保存に失敗しました: {}", context_.scene_.path_.FullPath().string());
			return;
		}

		SC_LOG_NOTICE("シーンを上書き保存しました: {}", context_.scene_.path_.FullPath().string());
	}
}

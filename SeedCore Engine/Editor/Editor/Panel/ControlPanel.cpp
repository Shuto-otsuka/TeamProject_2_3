#include <Editor/Editor/Panel/ControlPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/World/ECS/System/SystemScheduler.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <AudioEngine/Audio/MixerSystem.h>

namespace SeedCore
{
	namespace
	{
		_CrtMemState g_playMemCheckpoint;

		void BeginPlayMemCheck()
		{
			_CrtMemCheckpoint(&g_playMemCheckpoint);
		}

		void EndPlayMemCheck()
		{
			_CrtMemState afterState;
			_CrtMemState diffState;
			_CrtMemCheckpoint(&afterState);
			if (_CrtMemDifference(&diffState, &g_playMemCheckpoint, &afterState))
			{
				_CrtMemDumpStatistics(&diffState);
			}
		}
	}

	ControlPanel::ControlPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	Float ControlPanel::Draw()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		Float menuBarHeight = ImGui::GetFrameHeight();

		Float toolbarHeight = 30.0f;
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		if (ImGui::Begin("##ControlPanel", nullptr, flags))
		{
			Bool isPlaying = context_.world_.timer_->Playing();
			Bool isPaused = context_.world_.timer_->Paused();
			Bool quitRequested = context_.world_.world_->ConsumeQuit();

			if (!isPlaying && ImGui::IsKeyPressed(ImGuiKey_F5))
			{
				playModeScene_.Capture(*context_.world_.world_);
				playModeRaytracing_ = context_.sceneVisual_.raytracing_;
				playModeScreenSpace_ = context_.sceneVisual_.screenSpace_;
				playModeRasterization_ = context_.sceneVisual_.rasterization_;
				playModeMasterVolume_ = MixerSystem::MasterVolume();
				playModeCategoryVolumes_.clear();
				for (const String& category : MixerSystem::CategoryNameList())
				{
					playModeCategoryVolumes_.push_back(MixerSystem::CategoryVolume(category));
				}
				BeginPlayMemCheck();
				context_.world_.timer_->Play();
				isPlaying = true;
				ImGui::SetWindowFocus("ゲームビュー");
			}
			if (isPlaying && (ImGui::IsKeyPressed(ImGuiKey_F7) || quitRequested))
			{
				context_.world_.timer_->Stop();
				Scene::Reset();
				context_.world_.world_->DestroyActors();
				playModeScene_.Instantiate(*context_.world_.world_, *context_.world_.resource_);
				playModeScene_.Clear();
				context_.world_.system_->Reset();
				context_.selection_.Clear();
				context_.scene_.history_.Clear();
				context_.sceneVisual_.raytracing_ = playModeRaytracing_;
				context_.sceneVisual_.screenSpace_ = playModeScreenSpace_;
				context_.sceneVisual_.rasterization_ = playModeRasterization_;
				MixerSystem::MasterVolume(playModeMasterVolume_);
				const DynamicArray<String>& categoryNames = MixerSystem::CategoryNameList();
				for (Size categoryIndex = 0; categoryIndex < categoryNames.size() && categoryIndex < playModeCategoryVolumes_.size(); ++categoryIndex)
				{
					MixerSystem::CategoryVolume(categoryNames[categoryIndex], playModeCategoryVolumes_[categoryIndex]);
				}
				InputSystem::EndMouseCapture();
				InputSystem::UnlockCursor();
				InputSystem::RevealCursor();
				InputSystem::RumbleBody(0, 0, 0);
				isPlaying = false;
				EndPlayMemCheck();
				ImGui::SetWindowFocus("エディタービュー");
			}
			if (isPlaying && ImGui::IsKeyPressed(ImGuiKey_F6))
			{
				if (isPaused)
				{
					context_.world_.timer_->Resume();
					isPaused = false;
				}
				else
				{
					context_.world_.timer_->Pause();
					isPaused = true;
				}
			}

			ImVec2 buttonSize(18, 18);
			Float totalWidth = buttonSize.x * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f + ImGui::GetStyle().FramePadding.x * 6.0f;
			Float offsetX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
			if (offsetX > 0.0f)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
			}

			if (isPlaying)
			{
				ImGui::BeginDisabled();
				ImGui::ImageButton("##Play", imguiTexture_.Icon(IconType::Play), buttonSize);
				ImGui::EndDisabled();
			}
			else
			{
				if (ImGui::ImageButton("##Play", imguiTexture_.Icon(IconType::Play), buttonSize))
				{
					playModeScene_.Capture(*context_.world_.world_);
					playModeRaytracing_ = context_.sceneVisual_.raytracing_;
					playModeScreenSpace_ = context_.sceneVisual_.screenSpace_;
					playModeRasterization_ = context_.sceneVisual_.rasterization_;
					playModeMasterVolume_ = MixerSystem::MasterVolume();
					playModeCategoryVolumes_.clear();
					for (const String& category : MixerSystem::CategoryNameList())
					{
						playModeCategoryVolumes_.push_back(MixerSystem::CategoryVolume(category));
					}
					BeginPlayMemCheck();
					context_.world_.timer_->Play();
					ImGui::SetWindowFocus("ゲームビュー");
				}
			}

			ImGui::SameLine();

			if (!isPlaying)
			{
				ImGui::BeginDisabled();
				ImGui::ImageButton("##Pause", imguiTexture_.Icon(IconType::Pause), buttonSize);
				ImGui::EndDisabled();
			}
			else if (isPaused)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				if (ImGui::ImageButton("##Pause", imguiTexture_.Icon(IconType::Pause), buttonSize))
				{
					context_.world_.timer_->Resume();
				}
				ImGui::PopStyleColor();
			}
			else
			{
				if (ImGui::ImageButton("##Pause", imguiTexture_.Icon(IconType::Pause), buttonSize))
				{
					context_.world_.timer_->Pause();
				}
			}

			ImGui::SameLine();

			if (!isPlaying)
			{
				ImGui::BeginDisabled();
				ImGui::ImageButton("##Stop", imguiTexture_.Icon(IconType::Stop), buttonSize);
				ImGui::EndDisabled();
			}
			else
			{
				if (ImGui::ImageButton("##Stop", imguiTexture_.Icon(IconType::Stop), buttonSize))
				{
					context_.world_.timer_->Stop();
					Scene::Reset();
					context_.world_.world_->DestroyActors();
					playModeScene_.Instantiate(*context_.world_.world_, *context_.world_.resource_);
					playModeScene_.Clear();
					context_.world_.system_->Reset();
					context_.selection_.Clear();
					context_.scene_.history_.Clear();
					context_.sceneVisual_.raytracing_ = playModeRaytracing_;
					context_.sceneVisual_.screenSpace_ = playModeScreenSpace_;
					context_.sceneVisual_.rasterization_ = playModeRasterization_;
					MixerSystem::MasterVolume(playModeMasterVolume_);
					const DynamicArray<String>& categoryNames = MixerSystem::CategoryNameList();
					for (Size categoryIndex = 0; categoryIndex < categoryNames.size() && categoryIndex < playModeCategoryVolumes_.size(); ++categoryIndex)
					{
						MixerSystem::CategoryVolume(categoryNames[categoryIndex], playModeCategoryVolumes_[categoryIndex]);
					}
					InputSystem::EndMouseCapture();
					InputSystem::UnlockCursor();
					InputSystem::RevealCursor();
					InputSystem::RumbleBody(0, 0, 0);
					EndPlayMemCheck();
					ImGui::SetWindowFocus("エディタービュー");
				}
			}

		}
		ImGui::End();

		ImGui::PopStyleVar(3);

		return toolbarHeight;
	}
}

#include <Editor/Editor/Panel/Embed/GameViewPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>
#include <GraphicsEngine/System/CameraSystem.h>

namespace SeedCore
{
	GameViewPanel::GameViewPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void GameViewPanel::Draw()
	{
		CameraSystem* cameraSystem = context_.view_.game_.cameraSystem_;
		if (!cameraSystem)
		{
			return;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle = context_.graphics_.graphics_->Display().View(RenderViewKind::Game);
		ImTextureID displayTextureId = cameraSystem->ActiveCamera() ? ImTextureID(frameBufferHandle.ptr) : imguiTexture_.Icon(String("Warning/NonCamera"));

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			context_.view_.focused_ = FocusedView::Game;
		}

		Bool freeHovered = cameraSystem->Mode() == CameraMode::Free && ImGui::IsWindowHovered();
		Bool rotateHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);
		Bool panHeld = InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::IsPressed);
		if (freeHovered && (rotateHeld || panHeld))
		{
			InputSystem::BeginMouseCapture();
		}
		if (!rotateHeld && !panHeld)
		{
			InputSystem::EndMouseCapture();
		}

		if (freeHovered)
		{
			Float deltaTime = ImGui::GetIO().DeltaTime;
			cameraSystem->Navigate(deltaTime);
		}

		ImVec2 regionSize = ImGui::GetContentRegionAvail();
		if (regionSize.x <= 0.0f || regionSize.y <= 0.0f)
		{
			return;
		}

		constexpr Float aspectRatio = 16.0f / 9.0f;
		Float imageWidth = regionSize.x;
		Float imageHeight = regionSize.x / aspectRatio;

		if (imageHeight > regionSize.y)
		{
			imageHeight = regionSize.y;
			imageWidth = regionSize.y * aspectRatio;
		}

		imageWidth = ImFloor(imageWidth);
		imageHeight = ImFloor(imageHeight);
		Float offsetX = ImFloor((regionSize.x - imageWidth) * 0.5f);
		Float offsetY = ImFloor((regionSize.y - imageHeight) * 0.5f);

		ImVec2 cursorPos = ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY);
		ImGui::SetCursorPos(cursorPos);

		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		screenPos.x = IM_ROUND(screenPos.x);
		screenPos.y = IM_ROUND(screenPos.y);
		ImGui::SetCursorScreenPos(screenPos);

		cameraSystem->Rect(Vector4(screenPos.x, screenPos.y, imageWidth, imageHeight));

		ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0.0f);
		if (displayTextureId != 0)
		{
			ImGui::Image(displayTextureId, ImVec2(imageWidth, imageHeight));
		}
		else
		{
			ImGui::Dummy(ImVec2(imageWidth, imageHeight));
		}
		ImGui::PopStyleVar();
	}
}

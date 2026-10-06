#include <Editor/Editor/Panel/EditorWindowPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/ViewportPicking.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Effect/Zephyr/Effect.h>
#include <GraphicsEngine/Texture/Image.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <AudioEngine/Audio/AudioListener.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	EditorWindowPanel::EditorWindowPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), guizmoPanel_(context)
	{
		/// No Code
	}

	void EditorWindowPanel::Draw(D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle)
	{
		ImGuiID dockspaceID = context_.graphics_.imgui_->DockSpaceID();
		ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

		/// [EN] The editor view takes focus the first time it is created; the window then lives until the editor closes.
		/// [JP] エディタービューは作られる最初の 1 回だけフォーカスを取る。ウィンドウはそのあとエディタを閉じるまで残る。
		if (!ImGui::FindWindowByName("エディタービュー"))
		{
			ImGui::SetNextWindowFocus();
		}

		if (ImGui::Begin("エディタービュー"))
		{
			DrawGuizmo();
			ImGui::Separator();

			ImVec2 regionSize = ImGui::GetContentRegionAvail();

			if (regionSize.x > 0.0f && regionSize.y > 0.0f)
			{
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

				ImVec2 cursorPosition = ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY);
				ImGui::SetCursorPos(cursorPosition);

				ImVec2 screenPosition = ImGui::GetCursorScreenPos();
				screenPosition.x = IM_ROUND(screenPosition.x);
				screenPosition.y = IM_ROUND(screenPosition.y);
				ImGui::SetCursorScreenPos(screenPosition);

				ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 0.0f);
				ImGui::Image(ImTextureID(frameBufferHandle.ptr), ImVec2(imageWidth, imageHeight));
				ImGui::PopStyleVar();

				ImVec2 borderMin = ImVec2(screenPosition.x - 1.0f, screenPosition.y - 1.0f);
				ImVec2 borderMax = ImVec2(screenPosition.x + imageWidth + 1.0f, screenPosition.y + imageHeight + 1.0f);
				ImGui::GetWindowDrawList()->AddRect(borderMin, borderMax, ImGui::GetColorU32(ImGuiCol_Border));

				Vector2 imagePosition = Vector2(screenPosition.x, screenPosition.y);
				Vector2 imageSize = Vector2(imageWidth, imageHeight);
				DrawIcon(imagePosition, imageSize);
				UpdatePick(imagePosition, imageSize);
				DrawManipulator(imagePosition, imageSize);
				DrawSpeed(imagePosition);
			}

			UpdateSnap();
			UpdateCamera();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorWindowPanel::DrawGuizmo()
	{
		ImVec2 iconSize(24, 24);
		ImVec4 transparent(0, 0, 0, 0);
		ImVec4 activeColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
		ImVec4 hoverColor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered);

		auto& op = context_.guizmo_.operation_;

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);

		ImGui::PushStyleColor(ImGuiCol_Button, !context_.guizmo_.visible_ ? activeColor : transparent);
		if (ImGui::ImageButton("##NonSelected", imguiTexture_.Icon(IconType::NonSelected), iconSize))
		{
			context_.guizmo_.visible_ = !context_.guizmo_.visible_;
			context_.guizmo_.rectTool_ = false;
			op = (ImGuizmo::OPERATION)0;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && op == ImGuizmo::TRANSLATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Translate", imguiTexture_.Icon(IconType::Translate), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			op = ImGuizmo::TRANSLATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && op == ImGuizmo::ROTATE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Rotate", imguiTexture_.Icon(IconType::Rotate), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			op = ImGuizmo::ROTATE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, (!context_.guizmo_.rectTool_ && op == ImGuizmo::SCALE) ? activeColor : transparent);
		if (ImGui::ImageButton("##Scale", imguiTexture_.Icon(IconType::Scale), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = false;
			op = ImGuizmo::SCALE;
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.guizmo_.rectTool_ ? activeColor : transparent);
		if (ImGui::ImageButton("##Rect", imguiTexture_.Icon(IconType::Rect), iconSize))
		{
			context_.guizmo_.visible_ = true;
			context_.guizmo_.rectTool_ = true;
		}
		ImGui::PopStyleColor();

		ImGui::PopStyleColor(2);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##GizmoIcon", imguiTexture_.Icon(IconType::Guizmo), iconSize))
		{
			ImGui::OpenPopup("##GizmoSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##GizmoSettings"))
		{
			ImGui::SeparatorText("スナップ");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("移動", &context_.guizmo_.translateSnap_, 0.1f, 0.01f, 100.0f, "%.2f");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("回転", &context_.guizmo_.rotateSnap_, 0.5f, 0.1f, 90.0f, "%.0f\xc2\xb0");

			ImGui::SetNextItemWidth(120.0f);
			ImGui::DragFloat("拡大縮小", &context_.guizmo_.scaleSnap_, 0.05f, 0.01f, 10.0f, "%.2f");

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);

		ImGui::SameLine();

		/// [EN] Toggle buttons stay filled with the active color while on.
		/// [JP] 切り替えボタンは、オンの間は地をアクティブ色で塗る。
		ImGui::PushStyleColor(ImGuiCol_Button, context_.view_.editor_.iconVisible_ ? activeColor : transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ShowIcon", imguiTexture_.Icon(IconType::ShowIcon), iconSize))
		{
			context_.view_.editor_.iconVisible_ = !context_.view_.editor_.iconVisible_;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("アイコンを表示");
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, context_.view_.editor_.shapeVisible_ ? activeColor : transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ShowShape", imguiTexture_.Icon(IconType::ShowShape), iconSize))
		{
			context_.view_.editor_.shapeVisible_ = !context_.view_.editor_.shapeVisible_;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("範囲などの形を常に表示");
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##CameraIcon", imguiTexture_.Icon(IconType::Camera), iconSize))
		{
			ImGui::OpenPopup("##CameraSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##CameraSettings"))
		{
			if (context_.view_.editor_.camera_)
			{
				EditorCamera& camera = *context_.view_.editor_.camera_;
				ImGui::SeparatorText("投影");

				Float nearPlane = camera.Near();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Near", &nearPlane, 0.001f, 0.0001f, 10.0f, "%.4f"))
				{
					camera.Near(nearPlane);
				}

				Float farPlane = camera.Far();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Far", &farPlane, 1.0f, 1.0f, 100000.0f, "%.0f"))
				{
					camera.Far(farPlane);
				}

				Float fov = camera.Fov();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("FOV", &fov, 0.5f, 1.0f, 179.0f, "%.1f"))
				{
					camera.Fov(fov);
				}
			}

			if (context_.view_.editor_.cameraController_)
			{
				EditorCameraController& controller = *context_.view_.editor_.cameraController_;
				ImGui::SeparatorText("操作速度");

				Float moveSpeed = controller.MoveSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("移動", &moveSpeed, 0.1f, 0.1f, 200.0f, "%.1f"))
				{
					controller.MoveSpeed(moveSpeed);
				}

				Float rotateSpeed = controller.RotateSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("回転", &rotateSpeed, 0.01f, 0.01f, 2.0f, "%.2f"))
				{
					controller.RotateSpeed(rotateSpeed);
				}

				Float scrollSpeed = controller.ScrollSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("ズーム", &scrollSpeed, 0.1f, 0.1f, 100.0f, "%.1f"))
				{
					controller.ScrollSpeed(scrollSpeed);
				}

				Float panSpeed = controller.PanSpeed();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("パン", &panSpeed, 0.001f, 0.001f, 1.0f, "%.3f"))
				{
					controller.PanSpeed(panSpeed);
				}

				Float shiftMultiplier = controller.ShiftSpeedMultiplier();
				ImGui::SetNextItemWidth(120.0f);
				if (ImGui::DragFloat("Shift倍率", &shiftMultiplier, 0.1f, 1.0f, 20.0f, "x%.1f"))
				{
					controller.ShiftSpeedMultiplier(shiftMultiplier);
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);

		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, transparent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		if (ImGui::ImageButton("##ViewModeIcon", imguiTexture_.Icon(IconType::ViewMode), iconSize))
		{
			ImGui::OpenPopup("##ViewModeSettings");
		}
		ImGui::PopStyleColor(3);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
		if (ImGui::BeginPopup("##ViewModeSettings"))
		{
			auto item = [&](const Char* label, ViewMode mode)
			{
				if (ImGui::MenuItem(label, nullptr, context_.view_.editor_.viewMode_ == mode))
				{
					context_.view_.editor_.viewMode_ = mode;
				}
			};

			item("ライティングあり", ViewMode::Lit);
			item("ライティングなし", ViewMode::Unlit);
			item("ワイヤーフレーム", ViewMode::Wireframe);
			item("深度", ViewMode::Depth);
			item("メッシュレット", ViewMode::Meshlet);

			if (ImGui::BeginMenu("バッファービジュアライゼーション"))
			{
				item("法線", ViewMode::Normal);
				item("ラフネス", ViewMode::Roughness);
				item("メタルネス", ViewMode::Metallic);
				item("エミッシブ", ViewMode::Emissive);
				item("モーションベクター", ViewMode::Velocity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("レイトレーシング"))
			{
				item("反射（生）", ViewMode::ReflectionRaw);
				item("反射（デノイズ後）", ViewMode::ReflectionDenoised);
				ImGui::Separator();
				item("グローバルイルミネーション（生）", ViewMode::GlobalIlluminationRaw);
				item("グローバルイルミネーション（デノイズ後）", ViewMode::GlobalIlluminationDenoised);
				ImGui::Separator();
				item("シャドウ（生）", ViewMode::ShadowRaw);
				item("シャドウ（デノイズ後）", ViewMode::ShadowDenoised);
				ImGui::Separator();
				item("アンビエントオクルージョン（生）", ViewMode::AmbientOcclusionRaw);
				item("アンビエントオクルージョン（デノイズ後）", ViewMode::AmbientOcclusionDenoised);
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);
	}

	void EditorWindowPanel::DrawManipulator(const Vector2& position, const Vector2& size)
	{
		if (!context_.view_.editor_.camera_)
		{
			return;
		}

		constexpr Float gizmoSize = 80.0f;
		ImVec2 gizmoPosition = ImVec2(position.x, position.y + size.y - gizmoSize);

		Matrix view = context_.view_.editor_.camera_->View();

		Float orbitDistance = Vector3::Distance(context_.view_.editor_.camera_->Eye(), context_.view_.editor_.camera_->Focus());
		if (orbitDistance < 0.01f)
		{
			orbitDistance = 8.0f;
		}

		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(position.x, position.y, size.x, size.y);
		ImGuizmo::ViewManipulate(reinterpret_cast<Float*>(&view), orbitDistance, gizmoPosition, ImVec2(gizmoSize, gizmoSize), 0x00000000);

		if (ImGuizmo::IsUsingViewManipulate())
		{
			Matrix cameraWorld = view.Invert();

			Vector3 newEye = cameraWorld.Translation();
			Vector3 newForward = Vector3::TransformNormal(Vector3::Forward, cameraWorld);
			newForward.Normalize();

			Vector3 worldUp = Vector3::Up;
			Vector3 newUp = worldUp - newForward * newForward.Dot(worldUp);
			if (newUp.LengthSquared() < 1e-6f)
			{
				newUp = context_.view_.editor_.camera_->Up();
			}
			newUp.Normalize();

			context_.view_.editor_.camera_->Eye(newEye);
			context_.view_.editor_.camera_->Focus(newEye + newForward * orbitDistance);
			context_.view_.editor_.camera_->Up(newUp);
		}

		guizmoPanel_.Draw(position, size);
	}

	void EditorWindowPanel::DrawIcon(const Vector2& position, const Vector2& size)
	{
		icons_.clear();
		if (!context_.view_.editor_.iconVisible_ || !context_.view_.editor_.camera_ || !context_.world_.world_)
		{
			return;
		}

		Matrix viewProjection = context_.view_.editor_.camera_->NonJitterViewProjection();

		for (const Actor& actor : context_.world_.world_->GetActors())
		{
			if (!actor.Active())
			{
				continue;
			}

			/// [EN] Camera, then lights, then audio; actors with their own geometry or canvas content get no generic icon.
			/// [JP] カメラ、ライト、音の順に判定する。自前の見た目や Canvas の内容を持つアクターには汎用アイコンを出さない。
			ViewIcon icon;
			if (actor.GetComponent<Camera>() || actor.GetComponent<CameraBrain>())
			{
				icon.type_ = IconType::ViewCamera;
				icon.color_ = IM_COL32(127, 178, 255, 255);
			}
			else if (actor.GetComponent<PointLight>())
			{
				icon.type_ = IconType::ViewPointLight;
				icon.color_ = IM_COL32(255, 210, 90, 255);
			}
			else if (actor.GetComponent<SpotLight>())
			{
				icon.type_ = IconType::ViewSpotLight;
				icon.color_ = IM_COL32(255, 210, 90, 255);
			}
			else if (actor.GetComponent<DirectionalLight>())
			{
				icon.type_ = IconType::ViewDirectionalLight;
				icon.color_ = IM_COL32(255, 210, 90, 255);
			}
			else if (actor.GetComponent<RectangleLight>())
			{
				icon.type_ = IconType::ViewRectangleLight;
				icon.color_ = IM_COL32(255, 210, 90, 255);
			}
			else if (actor.GetComponent<SkyLight>())
			{
				icon.type_ = IconType::ViewSkyLight;
				icon.color_ = IM_COL32(150, 210, 255, 255);
			}
			else if (actor.GetComponent<AudioSource>())
			{
				icon.type_ = IconType::ViewAudioSource;
				icon.color_ = IM_COL32(111, 224, 180, 255);
			}
			else if (actor.GetComponent<AudioListener>())
			{
				icon.type_ = IconType::ViewAudioListener;
				icon.color_ = IM_COL32(111, 224, 180, 255);
			}
			else if (actor.GetComponent<Mesh>() || actor.GetComponent<Effect>() || actor.GetComponent<Image>() || actor.GetComponent<Text>() || actor.GetComponent<Movie>())
			{
				continue;
			}
			else
			{
				icon.type_ = IconType::ViewActor;
				icon.color_ = IM_COL32(255, 122, 217, 255);
			}

			/// [EN] Behind the camera or well outside the image, the icon is not drawn.
			/// [JP] カメラの後ろや画像の外側にあるものは描かない。
			Vector3 worldPosition = actor.WorldMatrix().Translation();
			Vector4 clip = Vector4::Transform(Vector4(worldPosition.x, worldPosition.y, worldPosition.z, 1.0f), viewProjection);
			if (clip.w <= 0.0001f)
			{
				continue;
			}

			Float ndcX = clip.x / clip.w;
			Float ndcY = clip.y / clip.w;
			if (ndcX < -1.1f || ndcX > 1.1f || ndcY < -1.1f || ndcY > 1.1f)
			{
				continue;
			}

			icon.actor_ = actor;
			icon.center_ = Vector2(position.x + (ndcX * 0.5f + 0.5f) * size.x, position.y + (0.5f - ndcY * 0.5f) * size.y);
			icon.depth_ = clip.w;
			icons_.push_back(icon);
		}

		/// [EN] Far to near, so nearer icons are drawn on top.
		/// [JP] 遠い順に並べ、近いアイコンが上に描かれるようにする。
		std::ranges::sort(icons_, [](const ViewIcon& a, const ViewIcon& b) { return a.depth_ > b.depth_; });

		/// [EN] A dark disc behind each icon keeps it readable on bright and dark scenes alike; selected actors get a ring.
		/// [JP] アイコンの後ろに暗い円を敷き、明るい場面でも暗い場面でも読めるようにする。選択中のアクターには輪を付ける。
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(ImVec2(position.x, position.y), ImVec2(position.x + size.x, position.y + size.y), true);
		Float half = iconSize_ * 0.5f;
		for (const ViewIcon& icon : icons_)
		{
			drawList->AddCircleFilled(ImVec2(icon.center_.x, icon.center_.y), half + 3.0f, IM_COL32(20, 20, 28, 170));
			drawList->AddImage(imguiTexture_.Icon(icon.type_), ImVec2(icon.center_.x - half, icon.center_.y - half), ImVec2(icon.center_.x + half, icon.center_.y + half), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImGui::ColorConvertFloat4ToU32(ImVec4(icon.color_.R(), icon.color_.G(), icon.color_.B(), icon.color_.A())));
			if (context_.selection_.Contains(icon.actor_))
			{
				drawList->AddCircle(ImVec2(icon.center_.x, icon.center_.y), half + 3.0f, IM_COL32(255, 200, 60, 255), 0, 2.0f);
			}
		}
		drawList->PopClipRect();
	}

	void EditorWindowPanel::DrawSpeed(const Vector2& position)
	{
		/// [EN] While right-click flying, the wheel changes move speed (see EditorCameraController::Update), so the speed is shown in the view's top-left corner for as long as the right button is held.
		/// [JP] 右クリックでの視点移動中はホイールで移動速度が変わる（EditorCameraController::Update 参照）。そのため右ボタンを押している間は、ビューの左上に速度を出す。
		Bool rotateHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);
		if (!rotateHeld || !ImGui::IsWindowHovered() || !context_.view_.editor_.cameraController_)
		{
			return;
		}

		std::string text = std::format("移動速度: {:.2f}", context_.view_.editor_.cameraController_->MoveSpeed());
		ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
		ImVec2 textPosition = ImVec2(position.x + 8.0f, position.y + 8.0f);

		/// [EN] A dark backdrop keeps the text readable over bright and dark scenes alike.
		/// [JP] 暗い下地を敷き、明るい場面でも暗い場面でも文字が読めるようにする。
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2(textPosition.x - 4.0f, textPosition.y - 2.0f), ImVec2(textPosition.x + textSize.x + 4.0f, textPosition.y + textSize.y + 2.0f), IM_COL32(20, 20, 28, 170), 4.0f);
		drawList->AddText(textPosition, IM_COL32(255, 255, 255, 255), text.c_str());
	}

	void EditorWindowPanel::UpdatePick(const Vector2& position, const Vector2& size)
	{
		if (ImGui::IsWindowHovered() && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !guizmoPanel_.RectToolActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && context_.view_.editor_.camera_ && context_.world_.world_)
		{
			ImVec2 mousePosition = ImGui::GetMousePos();
			if (mousePosition.x >= position.x && mousePosition.x <= position.x + size.x && mousePosition.y >= position.y && mousePosition.y <= position.y + size.y)
			{
				EditorCamera& camera = *context_.view_.editor_.camera_;

				Float ndcX = ((mousePosition.x - position.x) / size.x) * 2.0f - 1.0f;
				Float ndcY = 1.0f - ((mousePosition.y - position.y) / size.y) * 2.0f;

				Vector3 rayOrigin = camera.Eye();
				Vector4 farPointH = Vector4::Transform(Vector4(ndcX, ndcY, 1.0f, 1.0f), camera.InverseViewProjection());
				Vector3 farPoint = Vector3(farPointH.x, farPointH.y, farPointH.z) / farPointH.w;
				Vector3 rayDirection = farPoint - rayOrigin;
				rayDirection.Normalize();

				/// [EN] Icons sit on top of the scene, so the nearest icon under the cursor wins before the scene ray pick.
                /// [JP] アイコンは場面の上に重ねて描くので、カーソル下で最も近いアイコンを、場面へのレイによる選択より優先する。
				Actor hitActor;
				Float half = iconSize_ * 0.5f;
				for (auto it = icons_.rbegin(); it != icons_.rend(); ++it)
				{
					Float dx = mousePosition.x - it->center_.x;
					Float dy = mousePosition.y - it->center_.y;
					if (dx * dx + dy * dy <= half * half)
					{
						hitActor = it->actor_;
						break;
					}
				}
				if (!hitActor)
				{
					hitActor = ViewportPicking::Pick(*context_.world_.world_, rayOrigin, rayDirection);
				}

				if (ImGui::GetIO().KeyCtrl)
				{
					context_.selection_.Toggle(hitActor);
				}
				else
				{
					context_.selection_.Select(hitActor);
				}
			}
		}
	}

	void EditorWindowPanel::UpdateSnap()
	{
		if (ImGui::IsWindowHovered() && InputSystem::KeyState(InputSystem::Key::Control) && context_.guizmo_.operation_ != 0 && !context_.guizmo_.rectTool_)
		{
			auto& guizmo = context_.guizmo_;

			Float wheel = InputSystem::MouseWheel();
			if (Abs(wheel) > 0.0f)
			{
				if (guizmo.operation_ == ImGuizmo::TRANSLATE)
				{
					guizmo.translateSnap_ = Clamp(guizmo.translateSnap_ + wheel * 0.1f, 0.01f, 100.0f);
				}
				else if (guizmo.operation_ == ImGuizmo::ROTATE)
				{
					guizmo.rotateSnap_ = Clamp(guizmo.rotateSnap_ + wheel * 0.5f, 0.1f, 90.0f);
				}
				else if (guizmo.operation_ == ImGuizmo::SCALE)
				{
					guizmo.scaleSnap_ = Clamp(guizmo.scaleSnap_ + wheel * 0.05f, 0.01f, 10.0f);
				}
			}

			if (guizmo.operation_ == ImGuizmo::TRANSLATE)
			{
				ImGui::SetTooltip("移動スナップ: %.2f", guizmo.translateSnap_);
			}
			else if (guizmo.operation_ == ImGuizmo::ROTATE)
			{
				ImGui::SetTooltip("回転スナップ: %.0f\xc2\xb0", guizmo.rotateSnap_);
			}
			else if (guizmo.operation_ == ImGuizmo::SCALE)
			{
				ImGui::SetTooltip("拡大縮小スナップ: %.2f", guizmo.scaleSnap_);
			}
		}
	}

	void EditorWindowPanel::UpdateCamera()
	{
		Bool editorRotateHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);
		Bool editorPanHeld = InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::IsPressed);

		if (!ImGuizmo::IsUsing() && ImGui::IsWindowHovered() && context_.view_.editor_.camera_ && context_.view_.editor_.cameraController_)
		{
			if (editorRotateHeld || editorPanHeld)
			{
				InputSystem::BeginMouseCapture();
			}

			Float deltaTime = ImGui::GetIO().DeltaTime;
			context_.view_.editor_.cameraController_->Update(*context_.view_.editor_.camera_, deltaTime);
		}

		if (!editorRotateHeld && !editorPanHeld)
		{
			InputSystem::EndMouseCapture();
		}
	}
}

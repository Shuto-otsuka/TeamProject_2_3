#include <Editor/Editor/Panel/Embed/CanvasViewPanel.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Texture/Image.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <FoundationEngine/Input/InputSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>

namespace SeedCore
{
	CanvasViewPanel::CanvasViewPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture), guizmoPanel_(context, imguiTexture)
	{
		/// No Code
	}

	void CanvasViewPanel::Draw()
	{
		D3D12_GPU_DESCRIPTOR_HANDLE frameBufferHandle = context_.graphics_.graphics_->Display().View(RenderViewKind::Canvas);

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
			if (frameBufferHandle.ptr != 0)
			{
				ImGui::Image(ImTextureID(frameBufferHandle.ptr), ImVec2(imageWidth, imageHeight));
			}
			else
			{
				ImGui::Dummy(ImVec2(imageWidth, imageHeight));
			}
			ImGui::PopStyleVar();

			Bool canvasHovered = ImGui::IsItemHovered();
			Bool panHeld = InputSystem::MouseState(InputSystem::MouseButton::Right, InputSystem::IsPressed);

			if (canvasHovered && panHeld && !panActive_)
			{
				panActive_ = true;
				resetViewActive_ = false;
				InputSystem::BeginMouseCapture();
			}

			if (panActive_ && panHeld && context_.view_.canvas_.camera_)
			{
				CanvasCamera& canvasCamera = *context_.view_.canvas_.camera_;

				Float worldPerPixel = canvasCamera.VisibleHeight() / imageHeight;
				Float deltaX = -InputSystem::MouseMotion().x * worldPerPixel;
				Float deltaY = InputSystem::MouseMotion().y * worldPerPixel;

				Vector3 eye = canvasCamera.Eye();
				Vector3 focus = canvasCamera.Focus();
				eye.x += deltaX;
				eye.y += deltaY;
				focus.x += deltaX;
				focus.y += deltaY;
				canvasCamera.Eye(eye);
				canvasCamera.Focus(focus);
			}

			if (panActive_ && !panHeld)
			{
				panActive_ = false;
				InputSystem::EndMouseCapture();
			}

			if (canvasHovered && InputSystem::MouseState(InputSystem::MouseButton::Middle, InputSystem::TriggerMode::RISING_EDGE))
			{
				resetViewActive_ = true;
			}

			if (resetViewActive_ && context_.view_.canvas_.camera_)
			{
				CanvasCamera& canvasCamera = *context_.view_.canvas_.camera_;

				Vector3 targetFocus = Vector3(100000.0f + ScResolution::SC_CANVAS.Width * 0.5f, 100000.0f + ScResolution::SC_CANVAS.Height * 0.5f, 100000.0f);
				Vector3 targetEye = Vector3(100000.0f + ScResolution::SC_CANVAS.Width * 0.5f, 100000.0f + ScResolution::SC_CANVAS.Height * 0.5f, 99990.0f);

				Float lerpAmount = Clamp(ImGui::GetIO().DeltaTime * 10.0f, 0.0f, 1.0f);
				Vector3 newFocus = Vector3::Lerp(canvasCamera.Focus(), targetFocus, lerpAmount);
				Vector3 newEye = Vector3::Lerp(canvasCamera.Eye(), targetEye, lerpAmount);
				canvasCamera.Focus(newFocus);
				canvasCamera.Eye(newEye);

				if (Vector3::DistanceSq(newFocus, targetFocus) < 0.01f)
				{
					canvasCamera.Focus(targetFocus);
					canvasCamera.Eye(targetEye);
					resetViewActive_ = false;
				}
			}

			if (context_.view_.canvas_.camera_)
			{
				CanvasCamera& canvasCamera = *context_.view_.canvas_.camera_;

				Float worldPerPixel = canvasCamera.VisibleHeight() / imageHeight;
				Vector3 focus = canvasCamera.Focus();
				Float centerScreenX = screenPosition.x + imageWidth * 0.5f;
				Float centerScreenY = screenPosition.y + imageHeight * 0.5f;

				constexpr Float gridSpacing = 100.0f;
				Float worldMinX = focus.x - imageWidth * 0.5f * worldPerPixel;
				Float worldMaxX = focus.x + imageWidth * 0.5f * worldPerPixel;
				Float worldMinY = focus.y - imageHeight * 0.5f * worldPerPixel;
				Float worldMaxY = focus.y + imageHeight * 0.5f * worldPerPixel;

				ImGui::PushClipRect(ImVec2(screenPosition.x, screenPosition.y), ImVec2(screenPosition.x + imageWidth, screenPosition.y + imageHeight), true);

				Int gridStartX = static_cast<Int>(std::floor(worldMinX / gridSpacing));
				Int gridEndX = static_cast<Int>(std::ceil(worldMaxX / gridSpacing));
				for (Int gridIndex = gridStartX; gridIndex <= gridEndX; gridIndex++)
				{
					Float worldX = static_cast<Float>(gridIndex) * gridSpacing;
					Float lineScreenX = centerScreenX + (worldX - focus.x) / worldPerPixel;
					ImGui::GetWindowDrawList()->AddLine(ImVec2(lineScreenX, screenPosition.y), ImVec2(lineScreenX, screenPosition.y + imageHeight), IM_COL32(255, 255, 255, 30));
				}

				Int gridStartY = static_cast<Int>(std::floor(worldMinY / gridSpacing));
				Int gridEndY = static_cast<Int>(std::ceil(worldMaxY / gridSpacing));
				for (Int gridIndex = gridStartY; gridIndex <= gridEndY; gridIndex++)
				{
					Float worldY = static_cast<Float>(gridIndex) * gridSpacing;
					Float lineScreenY = centerScreenY - (worldY - focus.y) / worldPerPixel;
					ImGui::GetWindowDrawList()->AddLine(ImVec2(screenPosition.x, lineScreenY), ImVec2(screenPosition.x + imageWidth, lineScreenY), IM_COL32(255, 255, 255, 30));
				}

				ImGui::PopClipRect();

				Float landmarkMinX = centerScreenX + (100000.0f - focus.x) / worldPerPixel;
				Float landmarkMaxX = centerScreenX + (100000.0f + ScResolution::SC_CANVAS.Width - focus.x) / worldPerPixel;
				Float landmarkMinY = centerScreenY - (100000.0f + ScResolution::SC_CANVAS.Height - focus.y) / worldPerPixel;
				Float landmarkMaxY = centerScreenY - (100000.0f - focus.y) / worldPerPixel;

				ImVec2 landmarkMin = ImVec2(landmarkMinX, landmarkMinY);
				ImVec2 landmarkMax = ImVec2(landmarkMaxX, landmarkMaxY);

				ImGui::PushClipRect(ImVec2(screenPosition.x, screenPosition.y), ImVec2(screenPosition.x + imageWidth, screenPosition.y + imageHeight), true);
				ImGui::GetWindowDrawList()->AddRect(landmarkMin, landmarkMax, IM_COL32(0, 255, 255, 255), 0.0f, 0, 2.0f);
				ImGui::PopClipRect();
			}

			HandlePicking(screenPosition, imageWidth, imageHeight);

			Vector2 cachePosition = { screenPosition.x, screenPosition.y };
			Vector2 cacheSize = { imageWidth, imageHeight };

			ImGui::PushClipRect(ImVec2(screenPosition.x, screenPosition.y), ImVec2(screenPosition.x + imageWidth, screenPosition.y + imageHeight), true);
			guizmoPanel_.Draw(cachePosition, cacheSize);
			ImGui::PopClipRect();
		}
	}

	void CanvasViewPanel::HandlePicking(const ImVec2& imageScreenPos, Float imageWidth, Float imageHeight)
	{
		if (!context_.view_.canvas_.camera_ || !context_.world_.world_)
		{
			return;
		}

		ImVec2 mouse = ImGui::GetMousePos();
		Bool mouseInImage = mouse.x >= imageScreenPos.x && mouse.x <= imageScreenPos.x + imageWidth && mouse.y >= imageScreenPos.y && mouse.y <= imageScreenPos.y + imageHeight;

		if (!boxSelectPending_ && !boxSelectActive_)
		{
			if (mouseInImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !guizmoPanel_.RectToolActive())
			{
				boxSelectPending_ = true;
				boxSelectStart_ = mouse;
			}
			return;
		}

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			boxSelectPending_ = false;
			boxSelectActive_ = false;
			return;
		}

		if (boxSelectPending_ && !boxSelectActive_)
		{
			if (std::abs(mouse.x - boxSelectStart_.x) + std::abs(mouse.y - boxSelectStart_.y) > 4.0f)
			{
				boxSelectActive_ = true;
			}
		}

		CanvasCamera& camera = *context_.view_.canvas_.camera_;
		World& world = *context_.world_.world_;

		Float worldPerPixel = camera.VisibleHeight() / imageHeight;
		Vector3 focus = camera.Focus();
		Float centerScreenX = imageScreenPos.x + imageWidth * 0.5f;
		Float centerScreenY = imageScreenPos.y + imageHeight * 0.5f;

		if (boxSelectActive_)
		{
			ImVec2 rectMin = ImVec2(Min(boxSelectStart_.x, mouse.x), Min(boxSelectStart_.y, mouse.y));
			ImVec2 rectMax = ImVec2(Max(boxSelectStart_.x, mouse.x), Max(boxSelectStart_.y, mouse.y));
			ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, IM_COL32(80, 140, 255, 40));
			ImGui::GetWindowDrawList()->AddRect(rectMin, rectMax, IM_COL32(80, 140, 255, 200));
		}

		if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			return;
		}

		SelectionContext& selection = context_.selection_;
		Bool ctrl = ImGui::GetIO().KeyCtrl;

		if (boxSelectActive_)
		{
			Float startWorldX = focus.x + (boxSelectStart_.x - centerScreenX) * worldPerPixel;
			Float startWorldY = focus.y - (boxSelectStart_.y - centerScreenY) * worldPerPixel;
			Float endWorldX = focus.x + (mouse.x - centerScreenX) * worldPerPixel;
			Float endWorldY = focus.y - (mouse.y - centerScreenY) * worldPerPixel;
			Float rectMinX = Min(startWorldX, endWorldX);
			Float rectMaxX = Max(startWorldX, endWorldX);
			Float rectMinY = Min(startWorldY, endWorldY);
			Float rectMaxY = Max(startWorldY, endWorldY);

			DynamicArray<Actor> hits;
			Query<Read<Active>, Read<Image>, Read<Bounds>> query(world);
			query.ForEach([&](EntityID entityID, const Active& active, const Image& image, const Bounds& bounds)
				{
					if (!active.active_ || image.viewType_ != Image::ViewType::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float rotation = worldRotation.ToEuler().z;
					Float cosRotation = std::cos(rotation);
					Float sinRotation = std::sin(rotation);

					Float centerX = 100000.0f + worldTranslation.x;
					Float centerY = 100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y;
					Float halfWidth = bounds.extent_.x * worldScale.x;
					Float halfHeight = bounds.extent_.y * worldScale.y;
					Float pivotX = bounds.center_.x * worldScale.x;
					Float pivotY = bounds.center_.y * worldScale.y;

					Float quadMinX = FLT_MAX;
					Float quadMaxX = -FLT_MAX;
					Float quadMinY = FLT_MAX;
					Float quadMaxY = -FLT_MAX;
					for (Int cornerX = -1; cornerX <= 1; cornerX += 2)
					{
						for (Int cornerY = -1; cornerY <= 1; cornerY += 2)
						{
							Float localX = pivotX + static_cast<Float>(cornerX) * halfWidth;
							Float localY = pivotY + static_cast<Float>(cornerY) * halfHeight;
							Float worldX = centerX + localX * cosRotation + localY * sinRotation;
							Float worldY = centerY - localX * sinRotation + localY * cosRotation;
							quadMinX = Min(quadMinX, worldX);
							quadMaxX = Max(quadMaxX, worldX);
							quadMinY = Min(quadMinY, worldY);
							quadMaxY = Max(quadMaxY, worldY);
						}
					}

					if (quadMaxX >= rectMinX && quadMinX <= rectMaxX && quadMaxY >= rectMinY && quadMinY <= rectMaxY)
					{
						hits.push_back(actor);
					}
				});

			Query<Read<Active>, Read<Text>, Read<Bounds>> textQuery(world);
			textQuery.ForEach([&](EntityID entityID, const Active& active, const Text& text, const Bounds& bounds)
				{
					if (!active.active_ || text.viewType_ != Text::ViewType::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float centerX = 100000.0f + worldTranslation.x + bounds.center_.x * worldScale.x;
					Float centerY = 100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y + bounds.center_.y * worldScale.y;
					Float halfWidth = bounds.extent_.x * worldScale.x;
					Float halfHeight = bounds.extent_.y * worldScale.y;

					if (centerX + halfWidth >= rectMinX && centerX - halfWidth <= rectMaxX && centerY + halfHeight >= rectMinY && centerY - halfHeight <= rectMaxY)
					{
						hits.push_back(actor);
					}
				});

			Query<Read<Active>, Read<Movie>, Read<Bounds>> movieQuery(world);
			movieQuery.ForEach([&](EntityID entityID, const Active& active, const Movie& movie, const Bounds& bounds)
				{
					if (!active.active_ || movie.displayMode_ != Movie::DisplayMode::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float centerX = 100000.0f + worldTranslation.x + bounds.center_.x * worldScale.x;
					Float centerY = 100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y + bounds.center_.y * worldScale.y;
					Float halfWidth = bounds.extent_.x * worldScale.x;
					Float halfHeight = bounds.extent_.y * worldScale.y;

					if (centerX + halfWidth >= rectMinX && centerX - halfWidth <= rectMaxX && centerY + halfHeight >= rectMinY && centerY - halfHeight <= rectMaxY)
					{
						hits.push_back(actor);
					}
				});

			if (!ctrl)
			{
				selection.Clear();
			}
			for (Actor actor : hits)
			{
				selection.Add(actor);
			}
		}
		else if (mouseInImage)
		{
			Float mouseWorldX = focus.x + (mouse.x - centerScreenX) * worldPerPixel;
			Float mouseWorldY = focus.y - (mouse.y - centerScreenY) * worldPerPixel;

			Actor hit;
			Int hitOrder = 0;
			Query<Read<Active>, Read<Image>, Read<Bounds>> query(world);
			query.ForEach([&](EntityID entityID, const Active& active, const Image& image, const Bounds& bounds)
				{
					if (!active.active_ || image.viewType_ != Image::ViewType::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float rotation = worldRotation.ToEuler().z;
					Float cosRotation = std::cos(rotation);
					Float sinRotation = std::sin(rotation);

					Float deltaX = mouseWorldX - (100000.0f + worldTranslation.x);
					Float deltaY = mouseWorldY - (100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y);
					Float localX = deltaX * cosRotation - deltaY * sinRotation;
					Float localY = deltaX * sinRotation + deltaY * cosRotation;

					Float halfWidth = bounds.extent_.x * worldScale.x;
					Float halfHeight = bounds.extent_.y * worldScale.y;
					Float pivotX = bounds.center_.x * worldScale.x;
					Float pivotY = bounds.center_.y * worldScale.y;

					if (std::abs(localX - pivotX) <= halfWidth && std::abs(localY - pivotY) <= halfHeight && (!hit || image.order_ >= hitOrder))
					{
						hit = actor;
						hitOrder = image.order_;
					}
				});

			Query<Read<Active>, Read<Text>, Read<Bounds>> textQuery(world);
			textQuery.ForEach([&](EntityID entityID, const Active& active, const Text& text, const Bounds& bounds)
				{
					if (!active.active_ || text.viewType_ != Text::ViewType::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float localX = mouseWorldX - (100000.0f + worldTranslation.x) - bounds.center_.x * worldScale.x;
					Float localY = mouseWorldY - (100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y) - bounds.center_.y * worldScale.y;

					if (std::abs(localX) <= bounds.extent_.x * worldScale.x && std::abs(localY) <= bounds.extent_.y * worldScale.y && (!hit || text.order_ >= hitOrder))
					{
						hit = actor;
						hitOrder = text.order_;
					}
				});

			Query<Read<Active>, Read<Movie>, Read<Bounds>> movieQuery(world);
			movieQuery.ForEach([&](EntityID entityID, const Active& active, const Movie& movie, const Bounds& bounds)
				{
					if (!active.active_ || movie.displayMode_ != Movie::DisplayMode::Sprite)
					{
						return;
					}

					Actor actor = WorldAccessor::Make(world, entityID);
					if (!actor)
					{
						return;
					}

					Vector3 worldScale;
					Quaternion worldRotation;
					Vector3 worldTranslation;
					Matrix worldMatrix = actor.WorldMatrix();
					worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);

					Float localX = mouseWorldX - (100000.0f + worldTranslation.x) - bounds.center_.x * worldScale.x;
					Float localY = mouseWorldY - (100000.0f + ScResolution::SC_CANVAS.Height - worldTranslation.y) - bounds.center_.y * worldScale.y;

					if (std::abs(localX) <= bounds.extent_.x * worldScale.x && std::abs(localY) <= bounds.extent_.y * worldScale.y && (!hit || movie.order_ >= hitOrder))
					{
						hit = actor;
						hitOrder = movie.order_;
					}
				});

			if (ctrl && hit)
			{
				selection.Toggle(hit);
			}
			else
			{
				selection.Select(hit);
			}
		}

		boxSelectPending_ = false;
		boxSelectActive_ = false;
	}
}

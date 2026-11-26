#include <Editor/Editor/Editor.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/PanelGroup.h>
#include <Editor/Editor/Panel/Window/AnimatorControllerPanel.h>
#include <Editor/Editor/Panel/Window/AvatarPanel.h>
#include <Editor/Editor/Panel/Window/BootScreenPanel.h>
#include <Editor/Editor/Panel/Window/ConfigPanel.h>
#include <Editor/Editor/Panel/Window/ContentsDrawerPanel.h>
#include <Editor/Editor/Panel/Window/ControlPanel.h>
#include <Editor/Editor/Panel/Window/DiagnosticsPanel.h>
#include <Editor/Editor/Panel/Window/EditorWindowPanel.h>
#include <Editor/Editor/Panel/Window/EffectGraphPanel.h>
#include <Editor/Editor/Panel/Window/FullscreenPanel.h>
#include <Editor/Editor/Panel/Window/GameWindowPanel.h>
#include <Editor/Editor/Panel/Window/HierarchyPanel.h>
#include <Editor/Editor/Panel/Window/InspectorPanel.h>
#include <Editor/Editor/Panel/Window/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/Window/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/Window/MenuBarPanel.h>
#include <Editor/Editor/Panel/Window/ModelTransformPanel.h>
#include <Editor/Editor/Panel/Window/ShortCutKeyPanel.h>
#include <Editor/Editor/Panel/Window/SkeletonControllerPanel.h>
#include <Editor/Editor/Panel/Window/SpecMemoPanel.h>
#include <Editor/Editor/Panel/Window/TimelinePanel.h>
#include <Editor/Editor/Panel/Window/TodoListPanel.h>
#include <Editor/Editor/Panel/Window/VersionPanel.h>

#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Resource/ResourceSync.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>
#include <FoundationEngine/Resource/Sharing/CollaborationRegistry.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/Time/WorldTimer.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/World.h>

#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Preview/PreviewRequest.h>
#include <GraphicsEngine/Screen/Boot/BootRequest.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>
#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Animation/AnimatorControllerState.h>
#include <GraphicsEngine/Model/Material/MaterialState.h>
#include <GraphicsEngine/Model/Skeleton/SkeletonState.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Texture/Image.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
	class Editor::Impl
	{
	public:
		Impl(EditorContext& context);

		void Tick(Float deltaTime);

		void Draw();

	private:
		void PruneSelection();

		void FocusSelection();

		void ProcessPanels();

		void ProcessResources();

		void ProcessScene(Uint32 assetID);

	private:
		EditorContext& context_;

		ImGuiTexture texture_;

		ResourceSync resourceSync_;

		WorldTimer worldTimer_;

		FilePath followedScenePath_;

		MenuBarPanel menuBar_;

		ControlPanel control_;

		ToolPanels tools_;

		DockPanels docks_;
	};

	Editor::Impl::Impl(EditorContext& context) : context_(context), texture_(context_), resourceSync_(FileLocator::EngineRoot()), menuBar_(context_, texture_), control_(context_, texture_), tools_(context_, texture_), docks_(context_, texture_)
	{
		context_.panel_.tools_ = &tools_;
		context_.panel_.docks_ = &docks_;

		context_.application_.resourceSync_ = &resourceSync_;
	}

	void Editor::Impl::Tick(Float deltaTime)
	{
		context_.application_.uiFrame_++;

		ScResolution::ResSize outputSize = ToResSize(context_.config_.game_->resolution_);
		GraphicsSettings settings;
		settings.outputWidth_ = static_cast<Uint32>(outputSize.Width);
		settings.outputHeight_ = static_cast<Uint32>(outputSize.Height);
		settings.upscale_ = context_.config_.game_->upscale_;
		settings.verticalSync_ = context_.config_.game_->vsync_;
		settings.frameGeneration_ = context_.config_.game_->useFrameGeneration_;
		settings.shapeVisible_ = context_.view_.editor_.shapeVisible_;
		context_.graphics_.graphics_->Configure(settings);

		Benchmark::Begin("ImGuiNewFrame");
		context_.graphics_.imgui_->NewFrame();
		ImGuizmo::BeginFrame();
		Benchmark::End("ImGuiNewFrame");

		Benchmark::Begin("Camera");
		worldTimer_.Tick(deltaTime);
		context_.view_.editor_.camera_->Tick(deltaTime);
		context_.view_.canvas_.camera_->Resize(ScResolution::SC_CANVAS.Width, ScResolution::SC_CANVAS.Height);
		context_.view_.canvas_.camera_->Tick(deltaTime);
		context_.preview_.Tick(deltaTime);
		Benchmark::End("Camera");

		Benchmark::Begin("ResourceSync");
		resourceSync_.Update();
		ProcessResources();
		resourceSync_.FetchReferenced(*context_.world_.world_);
		Benchmark::End("ResourceSync");

		PruneSelection();
	}

	void Editor::Impl::Draw()
	{
		Graphics& graphics = *context_.graphics_.graphics_;
		ImGuiRenderer& imgui = *context_.graphics_.imgui_;
		World& world = *context_.world_.world_;
		GameTimer& gameTimer = *context_.world_.timer_;

		FocusedView focused = context_.view_.focused_;

		Float deltaTime = worldTimer_.DeltaTime();
		if (focused != FocusedView::Editor && (focused == FocusedView::Game || gameTimer.Playing()))
		{
			deltaTime = gameTimer.ScaledDeltaTime();
		}
		graphics.Update(deltaTime, world);

		if (focused == FocusedView::Editor)
		{
			graphics.EditorRender(worldTimer_, *context_.view_.editor_.camera_, world, context_.view_.editor_.viewMode_, context_.view_.editor_.debugSource_, context_.view_.editor_.showFlags_);
		}
		if (focused == FocusedView::Game || gameTimer.Playing())
		{
			ScResolution::ResSize outputSize = ToResSize(context_.config_.game_->resolution_);
			graphics.GameRender(gameTimer, *context_.view_.game_.cameraSystem_, world, RenderViewOutput::Texture, Vector4(0.0f, 0.0f, static_cast<Float>(outputSize.Width), static_cast<Float>(outputSize.Height)));
		}
		if (focused == FocusedView::Canvas)
		{
			graphics.CanvasRender(worldTimer_, *context_.view_.canvas_.camera_, world);
		}

		Benchmark::Begin("Preview");
		const PreviewContext& preview = context_.preview_;
		if (preview.timeline_.active_)
		{
			PreviewRequest request;
			request.meshAssetID_ = preview.timeline_.meshAssetID_;
			request.animationAssetID_ = preview.timeline_.animationAssetID_;
			request.time_ = preview.timeline_.time_;
			graphics.PreviewRender(PreviewSlot::Timeline, request, preview.timeline_.camera_, worldTimer_.TotalTime());
		}
		if (preview.modelTransform_.active_)
		{
			PreviewRequest request;
			request.meshAssetID_ = preview.modelTransform_.meshAssetID_;
			request.world_ = preview.modelTransform_.worldMatrix_;
			request.axes_ = true;
			graphics.PreviewRender(PreviewSlot::ModelTransform, request, preview.modelTransform_.camera_, worldTimer_.TotalTime());
		}
		if (preview.material_.active_)
		{
			PreviewRequest request;
			request.meshAssetID_ = preview.material_.meshAssetID_;
			request.surfaceAssetID_ = preview.material_.surfaceAssetID_;
			graphics.PreviewRender(PreviewSlot::Material, request, preview.material_.camera_, worldTimer_.TotalTime());
		}
		if (preview.skeletonController_.active_)
		{
			PreviewRequest request;
			request.meshAssetID_ = preview.skeletonController_.meshAssetID_;
			request.bones_ = true;
			request.joint_ = preview.skeletonController_.nodeIndex_;
			graphics.PreviewRender(PreviewSlot::Skeleton, request, preview.skeletonController_.camera_, worldTimer_.TotalTime());
		}
		if (preview.avatar_.active_ && preview.avatar_.mesh_)
		{
			preview.avatar_.mesh_->Update(preview.avatar_.positions_, preview.avatar_.normals_);

			PreviewRequest request;
			request.avatar_ = preview.avatar_.mesh_;
			request.regionCount_ = Min(preview.avatar_.regionCount_, static_cast<Uint32>(request.regionTextures_.size()));
			for (Uint32 regionIndex = 0;regionIndex < request.regionCount_;++regionIndex)
			{
				request.regionTextures_[regionIndex] = preview.avatar_.regionTextureIndices_[regionIndex];
			}
			graphics.PreviewRender(PreviewSlot::Avatar, request, preview.avatar_.camera_, worldTimer_.TotalTime());
		}
		if (preview.bootScreen_.active_ && preview.bootScreen_.texture_ && preview.bootScreen_.config_)
		{
			BootRequest request;
			request.progress_ = preview.bootScreen_.progress_;
			request.time_ = worldTimer_.TotalTime();
			request.config_ = preview.bootScreen_.config_;
			request.texture_ = preview.bootScreen_.texture_;
			graphics.PreviewRender(request, preview.bootScreen_.width_, preview.bootScreen_.height_);
		}
		Benchmark::End("Preview");

		Benchmark::Begin("Panels");
		graphics.Bind();

		Benchmark::Begin("MenuBarPanel");
		menuBar_.Draw();
		Benchmark::End("MenuBarPanel");
		ProcessPanels();

		Benchmark::Begin("ControlPanel");
		Float toolbarHeight = control_.Draw();
		Benchmark::End("ControlPanel");

		imgui.DockSpaceBegin(toolbarHeight);
		tools_.Draw();

		if (!tools_.Panel<FullscreenPanel>().Opened())
		{
			docks_.Draw();
		}
		imgui.DockSpaceEnd();

		if (!gameTimer.Playing() && !ImGui::GetIO().WantTextInput)
		{
			Bool ctrlPressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
			if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_Z))
			{
				context_.scene_.history_.Undo();
			}
			if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_Y))
			{
				context_.scene_.history_.Redo();
			}
			if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_F))
			{
				FocusSelection();
			}
		}

		Benchmark::Begin("ImGuiRender");
		imgui.Render(graphics.Device().Context().GetDirectList()->Get());
		Benchmark::End("ImGuiRender");
		Benchmark::End("Panels");
	}

	void Editor::Impl::PruneSelection()
	{
		auto& selection = context_.selection_.actors_;
		selection.erase(std::remove_if(selection.begin(), selection.end(), [](const Actor& actor) {return !actor;}), selection.end());
	}

	void Editor::Impl::FocusSelection()
	{
		Actor actor = context_.selection_.Primary();
		if (!actor)
		{
			return;
		}

		const Image* image = actor.GetComponent<Image>();
		const Text* text = actor.GetComponent<Text>();
		const Movie* movie = actor.GetComponent<Movie>();
		Bool canvasActor = (image && image->viewType_ == Image::ViewType::Sprite) || (text && text->viewType_ == Text::ViewType::Sprite) || (movie && movie->displayMode_ == Movie::DisplayMode::Sprite);

		const Matrix& worldMatrix = actor.WorldMatrix();
		if (canvasActor && context_.view_.canvas_.camera_)
		{
			context_.view_.canvas_.camera_->FocusOn(Vector3(100000.0f + worldMatrix._41, 100000.0f + (ScResolution::SC_CANVAS.Height - worldMatrix._42), 100000.0f));
			ImGui::SetWindowFocus("キャンバスビュー");
			return;
		}

		if (!context_.view_.editor_.camera_)
		{
			return;
		}

		Float radius = 0.0f;
		Vector3 target(worldMatrix._41, worldMatrix._42, worldMatrix._43);
		const Bounds* bounds = actor.GetComponent<Bounds>();

		if (bounds && !actor.GetComponent<Animator>())
		{
			Float worldScale = Max(Max(Vector3(worldMatrix._11, worldMatrix._12, worldMatrix._13).Length(), Vector3(worldMatrix._21, worldMatrix._22, worldMatrix._23).Length()), Vector3(worldMatrix._31, worldMatrix._32, worldMatrix._33).Length());
			radius = bounds->extent_.Length() * worldScale;
			target = Vector3::TransformPosition(bounds->center_, worldMatrix);
		}

		context_.view_.editor_.camera_->FocusOn(target, radius);
		ImGui::SetWindowFocus("エディタービュー");
	}

	void Editor::Impl::ProcessPanels()
	{
		if (AnimatorControllerRequest::requested_)
		{
			AnimatorControllerRequest::requested_ = false;
			Actor actor = context_.selection_.Primary();
			tools_.Panel<AnimatorControllerPanel>().Open(actor ? const_cast<Animator*>(actor.GetComponent<Animator>()) : nullptr);
		}
		if (TimelineRequest::requested_)
		{
			TimelineRequest::requested_ = false;
			tools_.Panel<TimelinePanel>().Open();
		}
		if (SkeletonControllerRequest::requested_)
		{
			SkeletonControllerRequest::requested_ = false;
			tools_.Panel<SkeletonControllerPanel>().Open();
		}
		if (MaterialPanelRequest::openRequested_)
		{
			MaterialPanelRequest::openRequested_ = false;
			tools_.Panel<MaterialViewerPanel>().Open();
		}
	}

	void Editor::Impl::ProcessResources()
	{
		ResourceCache& resource = *context_.world_.resource_;

		if (followedScenePath_ != context_.scene_.path_)
		{
			followedScenePath_ = context_.scene_.path_;
			resourceSync_.RequestOpenScene(followedScenePath_.FullPath());
		}

		DynamicArray<String> imported;
		resourceSync_.ConsumeImportedAsset(imported);
		if (!imported.empty())
		{
			Bool unknown = false;
			for (const String& asset : imported)
			{
				String path = String((std::filesystem::path("UserProject") / asset.w_str()).generic_wstring());
				Uint32 id = resource.Find(path);
				AssetRecord* record = id == 0 ? nullptr : resource.Record(id);
				unknown = unknown || !record || record->path_ != path;
			}
			if (unknown)
			{
				resource.Reload();
			}

			for (const String& asset : imported)
			{
				String path = String((std::filesystem::path("UserProject") / asset.w_str()).generic_wstring());
				Uint32 id = resource.Find(path);
				AssetRecord* record = id == 0 ? nullptr : resource.Record(id);
				if (record && record->path_ == path)
				{
					resourceSync_.RequestRegister(*record);
				}
			}
		}

		DynamicArray<Uint32> changed;
		if (!context_.world_.timer_->Playing())
		{
			resourceSync_.ConsumeChangedAsset(changed);
		}
		if (changed.empty())
		{
			return;
		}

		for (Uint32 id : changed)
		{
			const SharedAsset* shared = resourceSync_.GetAsset(id);
			if (!shared || resource.Record(id))
			{
				continue;
			}
			String path = String((std::filesystem::path("UserProject") / shared->path_.str()).generic_string());
			Uint32 staleId = resource.Find(path);
			AssetRecord* stale = staleId == 0 ? nullptr : resource.Record(staleId);
			if (stale && stale->path_ == path)
			{
				resource.Forget(staleId);
			}
		}

		resource.Reload();
		for (Uint32 id : changed)
		{
			resource.Reload(id);
			ProcessScene(id);
		}
	}

	void Editor::Impl::ProcessScene(Uint32 assetID)
	{
		const SharedAsset* shared = resourceSync_.GetAsset(assetID);
		if (!shared || !shared->scene_ || followedScenePath_.Empty())
		{
			return;
		}

		SceneBlueprint arriving;
		if (!arriving.Read(followedScenePath_.FullPath()))
		{
			return;
		}

		World& world = *context_.world_.world_;
		const DynamicArray<BlueprintNode>& nodes = arriving.Nodes();

		std::unordered_map<String, Actor> live;
		ActorRegistry& registry = WorldAccessor::Actors(world);
		const CollaborationRegistry* collaborations = world.GetService<CollaborationRegistry>();
		for (EntityID id = registry.Root(); id != EntityID{}; id = registry.Next(id))
		{
			const CollaborationRecord* record = collaborations ? collaborations->Record(id) : nullptr;
			if (record && !record->id_.str().empty())
			{
				live[record->id_] = WorldAccessor::Make(world, id);
			}
		}

		for (const BlueprintNode& node : nodes)
		{
			auto existing = live.find(node.collaborationId_);
			if (existing != live.end())
			{
				node.Apply(existing->second);
				continue;
			}

			Actor parent;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < nodes.size())
			{
				auto found = live.find(nodes[static_cast<Size>(node.parentIndex_)].collaborationId_);
				parent = found == live.end() ? Actor() : found->second;
			}
			live[node.collaborationId_] = node.Instantiate(world, *context_.world_.resource_, parent, false);
		}

		for (const std::pair<const String, Actor>& entry : live)
		{
			Bool present = std::ranges::any_of(nodes, [&entry](const BlueprintNode& node) { return node.collaborationId_ == entry.first; });
			if (!present)
			{
				world.DestroyActor(entry.second);
			}
		}
	}

	Editor::Editor(EditorContext& context) : impl_(MakePtr<Impl>(context))
	{
		/// No Code
	}

	Editor::~Editor() = default;

	void Editor::Tick(Float deltaTime)
	{
		impl_->Tick(deltaTime);
	}

	void Editor::Draw()
	{
		impl_->Draw();
	}
}

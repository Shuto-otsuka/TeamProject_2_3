#include <Editor/Editor/Editor.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Panel/AnimatorControllerPanel.h>
#include <Editor/Editor/Panel/AvatarPanel.h>
#include <Editor/Editor/Panel/BootScreenPanel.h>
#include <Editor/Editor/Panel/CanvasViewPanel.h>
#include <Editor/Editor/Panel/ConfigPanel.h>
#include <Editor/Editor/Panel/ContentsDrawerPanel.h>
#include <Editor/Editor/Panel/ControlPanel.h>
#include <Editor/Editor/Panel/DiagnosticsPanel.h>
#include <Editor/Editor/Panel/EditorWindowPanel.h>
#include <Editor/Editor/Panel/GameWindowPanel.h>
#include <Editor/Editor/Panel/HierarchyPanel.h>
#include <Editor/Editor/Panel/InspectorPanel.h>
#include <Editor/Editor/Panel/LayerSettingsPanel.h>
#include <Editor/Editor/Panel/MaterialViewerPanel.h>
#include <Editor/Editor/Panel/MenuBarPanel.h>
#include <Editor/Editor/Panel/ModelTransformPanel.h>
#include <Editor/Editor/Panel/ShortCutKeyPanel.h>
#include <Editor/Editor/Panel/SkeletonControllerPanel.h>
#include <Editor/Editor/Panel/SpecMemoPanel.h>
#include <Editor/Editor/Panel/TimelinePanel.h>
#include <Editor/Editor/Panel/TodoListPanel.h>
#include <Editor/Editor/Panel/VersionPanel.h>

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
#include <GraphicsEngine/Graphics.h>
#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/Renderer/BootScreenRenderer.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Animation/AnimatorControllerState.h>
#include <GraphicsEngine/Model/Material/MaterialState.h>
#include <GraphicsEngine/Model/Skeleton/SkeletonState.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Texture/Image.h>

namespace SeedCore
{
	class EditorPanels
	{
	public:
		EditorPanels(EditorContext& context, ImGuiTexture& texture);

		template<class T>
		T& Get()
		{
			return *std::get<ResourcePtr<T>>(panels_);
		}

		template<class T, class... Args>
		T& Create(Args&&... args)
		{
			auto& panel = std::get<ResourcePtr<T>>(panels_);
			panel = MakePtr<T>(std::forward<Args>(args)...);
			return *panel;
		}

	private:
		using Panels = std::tuple
			<
			ResourcePtr<HierarchyPanel>,
			ResourcePtr<InspectorPanel>,
			ResourcePtr<DiagnosticsPanel>,
			ResourcePtr<EditorWindowPanel>,
			ResourcePtr<GameWindowPanel>,
			ResourcePtr<CanvasViewPanel>,
			ResourcePtr<ContentsDrawerPanel>,
			ResourcePtr<ControlPanel>,
			ResourcePtr<MenuBarPanel>,
			ResourcePtr<ShortCutKeyPanel>,
			ResourcePtr<SpecMemoPanel>,
			ResourcePtr<TodoListPanel>,
			ResourcePtr<VersionPanel>,
			ResourcePtr<ConfigPanel>,
			ResourcePtr<LayerSettingsPanel>,
			ResourcePtr<AnimatorControllerPanel>,
			ResourcePtr<TimelinePanel>,
			ResourcePtr<SkeletonControllerPanel>,
			ResourcePtr<MaterialViewerPanel>,
			ResourcePtr<ModelTransformPanel>,
			ResourcePtr<AvatarPanel>,
			ResourcePtr<BootScreenPanel>
			>;

	private:
		Panels panels_;
	};

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

		EditorPanels panels_;
	};

	EditorPanels::EditorPanels(EditorContext& context, ImGuiTexture& texture)
	{
		Create<HierarchyPanel>(context, texture);
		Create<InspectorPanel>(context, texture);
		Create<DiagnosticsPanel>(context, texture);
		Create<EditorWindowPanel>(context, texture);
		Create<GameWindowPanel>(context, texture);
		Create<CanvasViewPanel>(context, texture);
		Create<ContentsDrawerPanel>(context, texture);
		Create<ControlPanel>(context, texture);
		Create<MenuBarPanel>(context);
		Create<ShortCutKeyPanel>(context);
		Create<SpecMemoPanel>(context);
		Create<TodoListPanel>(context, texture);
		Create<VersionPanel>(context);
		Create<ConfigPanel>(context, texture);
		Create<LayerSettingsPanel>(context);
		Create<AnimatorControllerPanel>(context, texture);
		Create<TimelinePanel>(context);
		Create<SkeletonControllerPanel>(context);
		Create<MaterialViewerPanel>(context, texture);
		Create<ModelTransformPanel>(context);
		Create<AvatarPanel>(context);
		Create<BootScreenPanel>(context);

		context.panel_.layerSettings_ = &Get<LayerSettingsPanel>();
		context.panel_.animatorController_ = &Get<AnimatorControllerPanel>();
		context.panel_.timeline_ = &Get<TimelinePanel>();
		context.panel_.skeletonController_ = &Get<SkeletonControllerPanel>();
		context.panel_.materialViewer_ = &Get<MaterialViewerPanel>();
		context.panel_.modelTransform_ = &Get<ModelTransformPanel>();
		context.panel_.avatar_ = &Get<AvatarPanel>();
		context.panel_.bootScreen_ = &Get<BootScreenPanel>();
		context.panel_.shortCutKey_ = &Get<ShortCutKeyPanel>();
		context.panel_.specMemo_ = &Get<SpecMemoPanel>();
		context.panel_.diagnostics_ = &Get<DiagnosticsPanel>();
		context.panel_.todoList_ = &Get<TodoListPanel>();
		context.panel_.version_ = &Get<VersionPanel>();
		context.panel_.config_ = &Get<ConfigPanel>();
	}

	Editor::Impl::Impl(EditorContext& context) : context_(context), texture_(context_), resourceSync_(context_.world_.resource_->ProjectRootPath()), panels_(context_, texture_)
	{
		context_.application_.resourceSync_ = &resourceSync_;
	}

	void Editor::Impl::Tick(Float deltaTime)
	{
		context_.application_.uiFrame_++;

		if (context_.config_.resizeRequested_)
		{
			context_.config_.resizeRequested_ = false;
			ScResolution::ResSize outputSize = ToResSize(context_.config_.game_->resolution_);
			context_.graphics_.graphics_->ResizeRenderTarget(static_cast<Uint32>(outputSize.Width), static_cast<Uint32>(outputSize.Height), context_.config_.game_->upscaleMode_);
		}

		if (context_.config_.recreateRequested_)
		{
			context_.config_.recreateRequested_ = false;
			context_.graphics_.graphics_->FrameGeneration(context_.config_.game_->useFrameGeneration_);
		}

		context_.graphics_.imgui_->NewFrame();
		ImGuizmo::BeginFrame();
		worldTimer_.Tick(deltaTime);
		context_.view_.editor_.camera_->Tick(deltaTime);
		context_.view_.canvas_.camera_->Resize(ScResolution::SC_CANVAS.Width, ScResolution::SC_CANVAS.Height);
		context_.view_.canvas_.camera_->Tick(deltaTime);
		context_.preview_.Tick(deltaTime);
		resourceSync_.Update();
		ProcessResources();
		resourceSync_.FetchReferenced(*context_.world_.world_);
		PruneSelection();
	}

	void Editor::Impl::Draw()
	{
		Graphics& graphics = *context_.graphics_.graphics_;
		ImGuiRenderer& imgui = *context_.graphics_.imgui_;
		World& world = *context_.world_.world_;
		GameTimer& gameTimer = *context_.world_.timer_;
		DynamicArray<Entity> selectedEntities;
		selectedEntities.reserve(context_.selection_.actors_.size());
		for (Actor actor : context_.selection_.actors_)
		{
			if (actor)
			{
				selectedEntities.push_back(ActorAccessor::GetEntity(actor));
			}
		}

		graphics.Upscale(context_.config_.game_->useDlss_, context_.config_.game_->upscaleMode_);
		graphics.VerticalSync(context_.config_.game_->vsync_);
		graphics.ShapeVisible(context_.view_.editor_.shapeVisible_);
		graphics.EditorRender(worldTimer_, *context_.view_.editor_.camera_, world, context_.view_.editor_.viewMode_, selectedEntities);
		graphics.GameRender(gameTimer, *context_.view_.game_.cameraSystem_, world);
		graphics.CanvasRender(worldTimer_, *context_.view_.canvas_.camera_, world);

		const PreviewContext& preview = context_.preview_;
		if (preview.timeline_.active_)
		{
			graphics.TimelineRender(worldTimer_, preview.timeline_.camera_, preview.timeline_.meshAssetID_, preview.timeline_.animationAssetID_, preview.timeline_.time_, Matrix::Identity);
		}
		if (preview.modelTransform_.active_)
		{
			graphics.ModelTransformRender(worldTimer_, preview.modelTransform_.camera_, preview.modelTransform_.meshAssetID_, 0, 0.0f, preview.modelTransform_.worldMatrix_);
		}
		if (preview.material_.active_)
		{
			graphics.MaterialRender(worldTimer_, preview.material_.camera_, preview.material_.meshAssetID_, preview.material_.surfaceAssetID_, Matrix::Identity);
		}
		if (preview.skeletonController_.active_)
		{
			graphics.SkeletonControllerRender(worldTimer_, preview.skeletonController_.camera_, preview.skeletonController_.meshAssetID_, 0, 0.0f, Matrix::Identity, preview.skeletonController_.nodeIndex_);
		}
		if (preview.avatar_.active_ && preview.avatar_.mesh_)
		{
			preview.avatar_.mesh_->Update(preview.avatar_.positions_, preview.avatar_.normals_);
			graphics.AvatarRender(worldTimer_, preview.avatar_.camera_, *preview.avatar_.mesh_, preview.avatar_.boneCount_, Matrix::Identity, std::span<const Uint32>(preview.avatar_.regionTextureIndices_, preview.avatar_.regionCount_));
		}
		if (preview.bootScreen_.active_ && preview.bootScreen_.renderer_ && preview.bootScreen_.config_)
		{
			preview.bootScreen_.renderer_->Render(graphics.GetContext().GetDirectList(), *preview.bootScreen_.config_, preview.bootScreen_.progress_, worldTimer_.TotalTime());
		}

		graphics.Bind();

		panels_.Get<TimelinePanel>().SetPreviewHandle(graphics.TimelineDisplayGPUHandle());
		panels_.Get<ModelTransformPanel>().SetPreviewHandle(graphics.ModelTransformDisplayGPUHandle());
		panels_.Get<SkeletonControllerPanel>().SetPreviewHandle(graphics.SkeletonControllerDisplayGPUHandle());
		panels_.Get<AvatarPanel>().SetPreviewHandle(graphics.AvatarDisplayGPUHandle());

		panels_.Get<MenuBarPanel>().Draw();
		ProcessPanels();
		Float toolbarHeight = panels_.Get<ControlPanel>().Draw();

		imgui.DockSpaceBegin(toolbarHeight);
		panels_.Get<AnimatorControllerPanel>().Draw();
		panels_.Get<TimelinePanel>().Draw();
		panels_.Get<SkeletonControllerPanel>().Draw();
		panels_.Get<MaterialViewerPanel>().Draw();
		panels_.Get<ModelTransformPanel>().Draw();
		panels_.Get<AvatarPanel>().Draw();
		panels_.Get<BootScreenPanel>().Draw();
		panels_.Get<GameWindowPanel>().Draw(graphics.GameDisplayGPUHandle(), toolbarHeight);
		panels_.Get<ShortCutKeyPanel>().Draw();
		panels_.Get<SpecMemoPanel>().Draw();
		panels_.Get<TodoListPanel>().Draw();
		panels_.Get<VersionPanel>().Draw();
		panels_.Get<ConfigPanel>().Draw();
		panels_.Get<LayerSettingsPanel>().Draw();

		if (!panels_.Get<GameWindowPanel>().Fullscreen())
		{
			panels_.Get<HierarchyPanel>().Draw();
			panels_.Get<InspectorPanel>().Draw();
			panels_.Get<DiagnosticsPanel>().Draw(graphics.GetGpuProfiler());
			panels_.Get<EditorWindowPanel>().Draw(graphics.EditorDisplayGPUHandle());
			panels_.Get<CanvasViewPanel>().Draw(graphics.CanvasDisplayGPUHandle());
			panels_.Get<ContentsDrawerPanel>().Draw();
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

		imgui.Render(graphics.GetContext().GetDirectList()->Get());
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
			target = Vector3::Transform(bounds->center_, worldMatrix);
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
			panels_.Get<AnimatorControllerPanel>().Open(actor ? const_cast<Animator*>(actor.GetComponent<Animator>()) : nullptr);
		}
		if (TimelineRequest::requested_)
		{
			TimelineRequest::requested_ = false;
			panels_.Get<TimelinePanel>().Open();
		}
		if (SkeletonControllerRequest::requested_)
		{
			SkeletonControllerRequest::requested_ = false;
			panels_.Get<SkeletonControllerPanel>().Open();
		}
		if (MaterialPanelRequest::openRequested_)
		{
			MaterialPanelRequest::openRequested_ = false;
			panels_.Get<MaterialViewerPanel>().Open();
		}
	}

	void Editor::Impl::ProcessResources()
	{
		ResourceCache& resource = *context_.world_.resource_;
		D3D12Context& d3d12 = context_.graphics_.graphics_->GetContext();

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
				Uint32 id = resource.GetAssetID(path);
				AssetRecord* record = id == 0 ? nullptr : resource.GetAsset(id);
				unknown = unknown || !record || record->path_ != path;
			}
			if (unknown)
			{
				resource.Reload(d3d12.GetDevice(), d3d12.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
			}
			for (const String& asset : imported)
			{
				String path = String((std::filesystem::path("UserProject") / asset.w_str()).generic_wstring());
				Uint32 id = resource.GetAssetID(path);
				AssetRecord* record = id == 0 ? nullptr : resource.GetAsset(id);
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
			if (!shared || resource.GetAsset(id))
			{
				continue;
			}
			String path = String((std::filesystem::path("UserProject") / shared->path_.str()).generic_string());
			Uint32 staleId = resource.GetAssetID(path);
			AssetRecord* stale = staleId == 0 ? nullptr : resource.GetAsset(staleId);
			if (stale && stale->path_ == path)
			{
				resource.Forget(staleId);
			}
		}

		resource.Reload(d3d12.GetDevice(), d3d12.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
		for (Uint32 id : changed)
		{
			resource.Reload(id, d3d12.GetDevice(), d3d12.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
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

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
	/**
	* [EN]
	* Holds one instance of every editor panel, looked up by type. Adding
	* a panel means adding it to Panels and creating it in the
	* constructor; no map or name lookup is involved.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタの各パネルを1つずつ保持し、型で引けるようにする。パネルを
	* 足すときは Panels に加えてコンストラクタで作るだけで、マップや名前
	* による検索は無い。
	*/
	class EditorPanels
	{
	public:
		/**
		* [EN]
		* Creates every panel and publishes the ones other code reaches
		* through context.panel_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全パネルを作り、他のコードが context.panel_ から触るものを
		* そこへ公開する。
		*/
		EditorPanels(EditorContext& context, ImGuiTexture& texture);

		/**
		* [EN]
		* Returns the panel of type T.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 型 T のパネルを返す。
		*/
		template<class T>
		T& Get()
		{
			return *std::get<ResourcePtr<T>>(panels_);
		}

		/**
		* [EN]
		* Creates the panel of type T from args, replacing any previous one,
		* and returns it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* args から型 T のパネルを作り（既にあれば置き換え）、それを返す。
		*/
		template<class T, class... Args>
		T& Create(Args&&... args)
		{
			auto& panel = std::get<ResourcePtr<T>>(panels_);
			panel = MakePtr<T>(std::forward<Args>(args)...);
			return *panel;
		}

	private:
		/// [EN] One slot per panel type; std::get by type finds a panel at compile time.
		/// [JP] パネルの型ごとに1枠。型で std::get すればコンパイル時にパネルが決まる。
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
		/// [EN] Every panel the editor owns.
		/// [JP] エディタが所有する全パネル。
		Panels panels_;
	};

	/**
	* [EN]
	* The editor's state behind Editor's pImpl: the panels, the shared-asset
	* sync, and the timer that drives the editor-side views.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Editor の pImpl の奥にある状態。パネル、共有アセットの同期、
	* エディタ側のビューを動かすタイマーを持つ。
	*/
	class Editor::Impl
	{
	public:
		/**
		* [EN]
		* Creates the panels and the shared-asset sync, and publishes the
		* sync through the context.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パネルと共有アセットの同期を作り、同期をコンテキスト越しに公開する。
		*/
		Impl(EditorContext& context);

		/**
		* [EN]
		* Advances the editor by one frame: pending resize requests,
		* cameras, previews and shared-asset sync.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタを1フレーム進める。保留中のリサイズ要求、カメラ、
		* プレビュー、共有アセットの同期を処理する。
		*/
		void Tick(Float deltaTime);

		/**
		* [EN]
		* Renders the editor, game and canvas views plus open previews,
		* then draws every panel and handles editor shortcuts.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタ・ゲーム・キャンバスの各ビューと開いているプレビューを
		* 描画し、続けて全パネルを描いてエディタのショートカットを処理する。
		*/
		void Draw();

	private:
		/**
		* [EN]
		* Drops actors that no longer exist from the selection.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* もう存在しない Actor を選択から外す。
		*/
		void PruneSelection();

		/**
		* [EN]
		* Moves the matching view's camera onto the primary selected actor:
		* the canvas view for sprites, the editor view for everything else.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 主選択の Actor へ、対応するビューのカメラを寄せる。スプライトなら
		* キャンバスビュー、それ以外はエディタービュー。
		*/
		void FocusSelection();

		/**
		* [EN]
		* Opens the panels that other code asked for through their request
		* flags since the last frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前のフレーム以降に、他のコードが要求フラグで求めたパネルを開く。
		*/
		void ProcessPanels();

		/**
		* [EN]
		* Brings the ResourceCache in line with the shared library: registers
		* newly imported assets, and reloads assets another member changed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ResourceCache を共有ライブラリに合わせる。新たに取り込んだアセットを
		* 登録し、他のメンバーが変えたアセットを読み直す。
		*/
		void ProcessResources();

		/**
		* [EN]
		* Merges the shared version of the open scene into the live world,
		* matching actors by collaboration ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 開いている Scene の共有版を、共同編集 ID で Actor を突き合わせて
		* 動いているワールドへ取り込む。
		*/
		void ProcessScene(Uint32 assetID);

	private:
		/// [EN] Editor-wide context shared with every panel.
		/// [JP] 全パネルと共有するエディタ全体のコンテキスト。
		EditorContext& context_;

		/// [EN] Texture registry the panels use to show images through ImGui.
		/// [JP] パネルが ImGui で画像を表示するためのテクスチャ登録。
		ImGuiTexture texture_;

		/// [EN] Keeps this project's assets in sync with the team's shared library.
		/// [JP] このプロジェクトのアセットをチームの共有ライブラリと同期させる。
		ResourceSync resourceSync_;

		/// [EN] Timer for the editor-side views, which keeps running while the game is stopped.
		/// [JP] エディタ側のビュー用のタイマー。ゲームが止まっている間も進む。
		WorldTimer worldTimer_;

		/// [EN] Scene the sync was last told is open; a different scene path means the sync must be told again.
		/// [JP] 最後に同期へ開いていると伝えた Scene。Scene のパスが変われば、伝え直す。
		FilePath followedScenePath_;

		/// [EN] Every editor panel.
		/// [JP] エディタの全パネル。
		EditorPanels panels_;
	};

	/**
	* [EN]
	* Creates every panel and publishes the ones other code reaches
	* through context.panel_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全パネルを作り、他のコードが context.panel_ から触るものを
	* そこへ公開する。
	*/
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

		/// [EN] Panels opened or driven from elsewhere (menus, inspectors, scripts) are reached through the context.
		/// [JP] 他所（メニュー、インスペクタ、スクリプト）から開いたり動かしたりするパネルは、コンテキスト越しに触る。
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

	/**
	* [EN]
	* Creates the panels and the shared-asset sync, and publishes the
	* sync through the context.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パネルと共有アセットの同期を作り、同期をコンテキスト越しに公開する。
	*/
	Editor::Impl::Impl(EditorContext& context) : context_(context), texture_(context_), resourceSync_(context_.world_.resource_->ProjectRootPath()), panels_(context_, texture_)
	{
		/// [EN] Panels read the sync's state (revisions, sharing status) through the context.
		/// [JP] パネルは同期の状態（Revision、共有の状態）をコンテキスト越しに読む。
		context_.application_.resourceSync_ = &resourceSync_;
	}

	/**
	* [EN]
	* Advances the editor by one frame: pending resize requests,
	* cameras, previews and shared-asset sync.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタを1フレーム進める。保留中のリサイズ要求、カメラ、
	* プレビュー、共有アセットの同期を処理する。
	*/
	void Editor::Impl::Tick(Float deltaTime)
	{
		context_.application_.uiFrame_++;

		/// [EN] Settings changed in the config panel are applied here, between frames, rather than in the middle of drawing.
		/// [JP] 設定パネルで変えた設定は、描画の途中ではなくフレームの間のここで反映する。
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

		/// [EN] Sync first collects what the library changed, the cache then takes it in, and finally any asset the world refers to but lacks is fetched.
		/// [JP] まず同期がライブラリの変化を集め、次にキャッシュがそれを取り込み、最後にワールドが参照しているのに手元に無いアセットを取りに行く。
		resourceSync_.Update();
		ProcessResources();
		resourceSync_.FetchReferenced(*context_.world_.world_);
		PruneSelection();
	}

	/**
	* [EN]
	* Renders the editor, game and canvas views plus open previews,
	* then draws every panel and handles editor shortcuts.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタ・ゲーム・キャンバスの各ビューと開いているプレビューを
	* 描画し、続けて全パネルを描いてエディタのショートカットを処理する。
	*/
	void Editor::Impl::Draw()
	{
		Graphics& graphics = *context_.graphics_.graphics_;
		ImGuiRenderer& imgui = *context_.graphics_.imgui_;
		World& world = *context_.world_.world_;
		GameTimer& gameTimer = *context_.world_.timer_;

		/// [EN] The selected actors' entities, which the editor view draws with a selection highlight.
		/// [JP] 選択中の Actor のエンティティ。エディタービューはこれらを選択の強調付きで描く。
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

		/// [EN] Each preview is rendered only while its panel is open.
		/// [JP] 各プレビューは、そのパネルが開いている間だけ描画する。
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

		/// [EN] The previews just rendered are handed to their panels to show as images.
		/// [JP] 今描いたプレビューを、画像として表示させるためにそれぞれのパネルへ渡す。
		panels_.Get<TimelinePanel>().SetPreviewHandle(graphics.TimelineDisplayGPUHandle());
		panels_.Get<ModelTransformPanel>().SetPreviewHandle(graphics.ModelTransformDisplayGPUHandle());
		panels_.Get<SkeletonControllerPanel>().SetPreviewHandle(graphics.SkeletonControllerDisplayGPUHandle());
		panels_.Get<AvatarPanel>().SetPreviewHandle(graphics.AvatarDisplayGPUHandle());

		/// [EN] The menu bar runs before ProcessPanels so a panel it asks for opens in this same frame.
		/// [JP] メニューバーを ProcessPanels より先に描き、そこで求めたパネルが同じフレームで開くようにする。
		panels_.Get<MenuBarPanel>().Draw();
		ProcessPanels();

		/// [EN] The dock space starts below the toolbar, so the toolbar's height is needed first.
		/// [JP] ドックスペースはツールバーの下から始まるので、先にツールバーの高さが要る。
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

		/// [EN] A fullscreen game window hides the scene-editing panels behind it.
		/// [JP] ゲームウィンドウが全画面なら、その裏のシーン編集用パネルは描かない。
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

		/// [EN] Editor shortcuts work only while stopped and while no text field has the keyboard.
		/// [JP] エディタのショートカットは、停止中かつテキスト入力欄がキーボードを持っていない間だけ効く。
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

	/**
	* [EN]
	* Drops actors that no longer exist from the selection.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* もう存在しない Actor を選択から外す。
	*/
	void Editor::Impl::PruneSelection()
	{
		auto& selection = context_.selection_.actors_;
		selection.erase(std::remove_if(selection.begin(), selection.end(), [](const Actor& actor) {return !actor;}), selection.end());
	}

	/**
	* [EN]
	* Moves the matching view's camera onto the primary selected actor:
	* the canvas view for sprites, the editor view for everything else.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 主選択の Actor へ、対応するビューのカメラを寄せる。スプライトなら
	* キャンバスビュー、それ以外はエディタービュー。
	*/
	void Editor::Impl::FocusSelection()
	{
		Actor actor = context_.selection_.Primary();
		if (!actor)
		{
			return;
		}

		/// [EN] An actor drawn as a sprite lives on the canvas, not in the 3D scene.
		/// [JP] スプライトとして描く Actor は、3D シーンではなくキャンバス上にいる。
		const Image* image = actor.GetComponent<Image>();
		const Text* text = actor.GetComponent<Text>();
		const Movie* movie = actor.GetComponent<Movie>();
		Bool canvasActor = (image && image->viewType_ == Image::ViewType::Sprite) || (text && text->viewType_ == Text::ViewType::Sprite) || (movie && movie->displayMode_ == Movie::DisplayMode::Sprite);

		const Matrix& worldMatrix = actor.WorldMatrix();
		/// [EN] The canvas camera sits at a 100000 offset with Y pointing down, so the actor's canvas position is converted into that space.
		/// [JP] キャンバスのカメラは 100000 ずらした位置にあり Y が下向きなので、Actor のキャンバス上の位置をその空間へ直す。
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

		/// [EN] Without usable bounds the camera aims at the actor's origin at its default distance.
		/// [JP] 使える Bounds が無ければ、カメラは Actor の原点を既定の距離で狙う。
		Float radius = 0.0f;
		Vector3 target(worldMatrix._41, worldMatrix._42, worldMatrix._43);
		const Bounds* bounds = actor.GetComponent<Bounds>();

		/// [EN] An actor with an Animator is framed by its origin, not by its Bounds.
		/// [JP] Animator を持つ Actor は、Bounds ではなく原点で収める。
		if (bounds && !actor.GetComponent<Animator>())
		{
			/// [EN] The largest axis scale keeps the whole bounds in view however the actor is scaled.
			/// [JP] 最も大きい軸のスケールを使い、Actor がどう拡縮されても Bounds 全体が収まるようにする。
			Float worldScale = Max(Max(Vector3(worldMatrix._11, worldMatrix._12, worldMatrix._13).Length(), Vector3(worldMatrix._21, worldMatrix._22, worldMatrix._23).Length()), Vector3(worldMatrix._31, worldMatrix._32, worldMatrix._33).Length());
			radius = bounds->extent_.Length() * worldScale;
			target = Vector3::Transform(bounds->center_, worldMatrix);
		}

		context_.view_.editor_.camera_->FocusOn(target, radius);
		ImGui::SetWindowFocus("エディタービュー");
	}

	/**
	* [EN]
	* Opens the panels that other code asked for through their request
	* flags since the last frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前のフレーム以降に、他のコードが要求フラグで求めたパネルを開く。
	*/
	void Editor::Impl::ProcessPanels()
	{
		/// [EN] Each flag is cleared as soon as it is seen, so a request opens its panel only once.
		/// [JP] 各フラグは見たらすぐに下ろし、1回の要求でパネルが開くのは1度だけにする。
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

	/**
	* [EN]
	* Brings the ResourceCache in line with the shared library: registers
	* newly imported assets, and reloads assets another member changed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ResourceCache を共有ライブラリに合わせる。新たに取り込んだアセットを
	* 登録し、他のメンバーが変えたアセットを読み直す。
	*/
	void Editor::Impl::ProcessResources()
	{
		ResourceCache& resource = *context_.world_.resource_;
		D3D12Context& d3d12 = context_.graphics_.graphics_->GetContext();

		/// [EN] The sync is told which scene is open, so changes other members make to it come down while it is being edited.
		/// [JP] どの Scene が開いているかを同期に伝え、編集中の Scene に他のメンバーが加えた変更が降りてくるようにする。
		if (followedScenePath_ != context_.scene_.path_)
		{
			followedScenePath_ = context_.scene_.path_;
			resourceSync_.RequestOpenScene(followedScenePath_.FullPath());
		}

		/// [EN] Files the sync placed under Assets since the last frame.
		/// [JP] 前のフレーム以降に、同期が Assets 以下へ置いたファイル。
		DynamicArray<String> imported;
		resourceSync_.ConsumeImportedAsset(imported);
		if (!imported.empty())
		{
			/// [EN] One full reload covers every file the cache does not know yet, instead of one per file.
			/// [JP] キャッシュがまだ知らないファイルは、1件ずつではなく1回の全体の読み直しでまとめて拾う。
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

			/// [EN] Once in the cache, each imported file is registered with the sync under its cache ID.
			/// [JP] キャッシュに入ったら、取り込んだファイルをそれぞれキャッシュの ID で同期に登録する。
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

		/// [EN] Assets another member changed; they stay queued during play so the running game is not swapped out underneath.
		/// [JP] 他のメンバーが変えたアセット。プレイ中は待たせておき、動いているゲームの下で差し替わらないようにする。
		DynamicArray<Uint32> changed;
		if (!context_.world_.timer_->Playing())
		{
			resourceSync_.ConsumeChangedAsset(changed);
		}
		if (changed.empty())
		{
			return;
		}

		/// [EN] An asset the cache knows only under another ID at the same path is forgotten, so the reload picks it up under the library's ID.
		/// [JP] 同じパスに別の ID でしかキャッシュに載っていないアセットは忘れさせ、読み直しでライブラリの ID のまま拾わせる。
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

		/// [EN] The full reload picks up new files; each changed asset is then reloaded by ID so already-loaded ones are replaced too.
		/// [JP] 全体の読み直しで新しいファイルを拾い、続けて変わったアセットを ID ごとに読み直して、読み込み済みのものも差し替える。
		resource.Reload(d3d12.GetDevice(), d3d12.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
		for (Uint32 id : changed)
		{
			resource.Reload(id, d3d12.GetDevice(), d3d12.GetDirectQueue(), context_.graphics_.graphics_->GetBC7CompressShader());
			ProcessScene(id);
		}
	}

	/**
	* [EN]
	* Merges the shared version of the open scene into the live world,
	* matching actors by collaboration ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 開いている Scene の共有版を、共同編集 ID で Actor を突き合わせて
	* 動いているワールドへ取り込む。
	*/
	void Editor::Impl::ProcessScene(Uint32 assetID)
	{
		/// [EN] Only a shared scene matters here, and only while a scene is open.
		/// [JP] ここで扱うのは共有の Scene だけで、Scene を開いている間だけ。
		const SharedAsset* shared = resourceSync_.GetAsset(assetID);
		if (!shared || !shared->scene_ || followedScenePath_.Empty())
		{
			return;
		}

		/// [EN] The scene file as it now stands after the sync wrote the other members' changes into it.
		/// [JP] 同期が他のメンバーの変更を書き込んだ後の、今の Scene ファイル。
		SceneBlueprint arriving;
		if (!arriving.Read(followedScenePath_.FullPath()))
		{
			return;
		}

		World& world = *context_.world_.world_;
		const DynamicArray<BlueprintNode>& nodes = arriving.Nodes();

		/// [EN] Live actors keyed by collaboration ID, the ID that names the same actor on every member's machine.
		/// [JP] 共同編集 ID をキーにした、今いる Actor。共同編集 ID は、どのメンバーの PC でも同じ Actor を指す。
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

		/// [EN] An actor already present takes the arriving values; a new one is created under its parent.
		/// [JP] 既にいる Actor には届いた値を当て、新しい Actor はその親の下に作る。
		for (const BlueprintNode& node : nodes)
		{
			auto existing = live.find(node.collaborationId_);
			if (existing != live.end())
			{
				node.Apply(existing->second);
				continue;
			}

			/// [EN] The parent is found through its node's collaboration ID; with no parent node the actor goes to the root.
			/// [JP] 親は、親ノードの共同編集 ID から探す。親ノードが無ければルートに置く。
			Actor parent;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < nodes.size())
			{
				auto found = live.find(nodes[static_cast<Size>(node.parentIndex_)].collaborationId_);
				parent = found == live.end() ? Actor() : found->second;
			}
			live[node.collaborationId_] = node.Instantiate(world, *context_.world_.resource_, parent, false);
		}

		/// [EN] An actor missing from the arriving scene was deleted by another member.
		/// [JP] 届いた Scene に無い Actor は、他のメンバーが削除したもの。
		for (const std::pair<const String, Actor>& entry : live)
		{
			Bool present = std::ranges::any_of(nodes, [&entry](const BlueprintNode& node) { return node.collaborationId_ == entry.first; });
			if (!present)
			{
				world.DestroyActor(entry.second);
			}
		}
	}

	/**
	* [EN]
	* Creates every panel and the shared-asset sync against context.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* context に対して全パネルと共有アセットの同期を作る。
	*/
	Editor::Editor(EditorContext& context) : impl_(MakePtr<Impl>(context))
	{
		/// No Code
	}

	/**
	* [EN]
	* Defined here, where Impl is a complete type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Impl が完全型になるここで定義する。
	*/
	Editor::~Editor() = default;

	/**
	* [EN]
	* Advances the editor by one frame: pending resize requests,
	* cameras, previews and shared-asset sync.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタを1フレーム進める。保留中のリサイズ要求、カメラ、
	* プレビュー、共有アセットの同期を処理する。
	*/
	void Editor::Tick(Float deltaTime)
	{
		impl_->Tick(deltaTime);
	}

	/**
	* [EN]
	* Renders the editor, game and canvas views plus open previews,
	* then draws every panel and handles editor shortcuts.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタ・ゲーム・キャンバスの各ビューと開いているプレビューを
	* 描画し、続けて全パネルを描いてエディタのショートカットを処理する。
	*/
	void Editor::Draw()
	{
		impl_->Draw();
	}
}

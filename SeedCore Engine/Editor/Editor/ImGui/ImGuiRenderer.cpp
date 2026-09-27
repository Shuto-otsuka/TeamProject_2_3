#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiCommon.h>
#include <FoundationEngine/Interop/ImGuiInstance.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the ImGui context with docking and multi-viewport enabled,
	* initializes the Win32 backend on hwnd and the DirectX 12 backend on
	* bindlessHeap (textures uploaded through commandQueue, one set of
	* per-frame buffers for each of numberFramesInFlight), loads the
	* Japanese font and applies the editor style. Returns false if any
	* backend fails to initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ドッキングとマルチビューポートを有効にした ImGui コンテキストを作り、
	* hwnd で Win32 バックエンドを、bindlessHeap で DirectX 12 バックエンドを
	* 初期化し(テクスチャは commandQueue でアップロードし、フレームごとの
	* バッファは numberFramesInFlight 個ぶん持つ)、日本語フォントを読み込んで
	* エディタのスタイルを適用する。いずれかのバックエンドの初期化に
	* 失敗したら false を返す。
	*/
	Bool ImGuiRenderer::Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, BindlessHeap* bindlessHeap, Int numberFramesInFlight)
	{
		bindlessHeap_ = bindlessHeap;

		/// [EN] Create the context and hand it to SeedCore.Cplusplus.dll, which links its own copy of ImGui and so keeps its own current-context pointer.
		/// [JP] コンテキストを作り、SeedCore.Cplusplus.dll へ渡す。SeedCore.Cplusplus.dll は ImGui を別に静的リンクしており、カレントコンテキストのポインタも別に持つため。
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiInstance::Bind(ImGui::GetCurrentContext());

		/// [EN] Panels dock into the main dockspace and can be dragged out into their own OS windows.
		/// [JP] パネルはメインドックスペースへドックでき、OS の別ウィンドウへ切り出すこともできる。
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (!ImGui_ImplWin32_Init(hwnd))
		{
			return false;
		}

		/// [EN] The DirectX 12 backend renders to the R8G8B8A8 back buffer without depth, and takes its texture descriptors from the bindless heap through the allocation callbacks, with the heap itself passed along as UserData.
		/// [JP] DirectX 12 バックエンドは深度なしで R8G8B8A8 のバックバッファへ描画し、テクスチャのディスクリプタは確保コールバック経由でバインドレスヒープから取る。ヒープ自体は UserData として渡す。
		ImGui_ImplDX12_InitInfo initInfo{};
		initInfo.Device = device;
		initInfo.CommandQueue = commandQueue;
		initInfo.NumFramesInFlight = numberFramesInFlight;
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
		initInfo.UserData = bindlessHeap;
		initInfo.SrvDescriptorHeap = bindlessHeap->Heap();
		initInfo.SrvDescriptorAllocFn = &ImGuiRenderer::AllocateDescriptor;
		initInfo.SrvDescriptorFreeFn = &ImGuiRenderer::FreeDescriptor;
		if (!ImGui_ImplDX12_Init(&initInfo))
		{
			return false;
		}

		if (!FontConfig(io))
		{
			return false;
		}
		StyleConfig();

		/// [EN] Build the backend's pipeline state and root signature now, so the first frame does not pay for it.
		/// [JP] バックエンドのパイプラインステートとルートシグネチャを今作っておき、最初のフレームで負担しないようにする。
		if (!ImGui_ImplDX12_CreateDeviceObjects())
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Starts an ImGui frame: the backends pick up this frame's input and
	* display state, then ImGui opens a new frame for the panels to build.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ImGui のフレームを開始する: バックエンドが今フレームの入力と表示状態を
	* 取り込み、その後 ImGui がパネルの構築先となる新しいフレームを開く。
	*/
	void ImGuiRenderer::NewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	/**
	* [EN]
	* Finishes the ImGui frame and records its draw data into cmdList with
	* the bindless heap bound, then updates and renders any panels that
	* were dragged out into their own OS windows.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ImGui のフレームを終え、バインドレスヒープをバインドした cmdList へ
	* 描画データを記録する。その後、OS の別ウィンドウへ切り出された
	* パネルを更新して描画する。
	*/
	void ImGuiRenderer::Render(ID3D12GraphicsCommandList* cmdList)
	{
		ImGui::Render();

		/// [EN] Every ImGui texture handle - its own font atlas and the engine textures the panels show - points into the bindless heap, so that heap must be the one bound.
		/// [JP] ImGui のテクスチャハンドルはすべて - 自身のフォントアトラスも、パネルが表示するエンジンのテクスチャも - バインドレスヒープを指すため、バインドするのはこのヒープでなければならない。
		ID3D12DescriptorHeap* descHeaps[] = { bindlessHeap_->Heap() };
		cmdList->SetDescriptorHeaps(1, descHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

		/// [EN] Panels dragged outside the main window live in their own OS windows, which ImGui updates and renders separately.
		/// [JP] メインウィンドウの外へ出したパネルは OS の別ウィンドウにあり、ImGui がそれらを別に更新して描画する。
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	/**
	* [EN]
	* Releases the font atlas, shuts both backends down and destroys the
	* ImGui context. Must run while the bindless heap still exists, since
	* the backend frees its descriptors back into it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォントアトラスを解放し、両バックエンドを終了して ImGui コンテキストを
	* 破棄する。バックエンドがディスクリプタをバインドレスヒープへ返すため、
	* ヒープがまだ存在する間に呼ぶこと。
	*/
	void ImGuiRenderer::Finalize()
	{
		if (ImGui::GetCurrentContext())
		{
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->Clear();
		}

		/// [EN] Shut down in reverse order of initialization: renderer backend, platform backend, then the context.
		/// [JP] 初期化と逆の順に終了する: レンダラーのバックエンド、プラットフォームのバックエンド、最後にコンテキスト。
		ImGui_ImplDX12_InvalidateDeviceObjects();
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		bindlessHeap_ = nullptr;
	}

	/**
	* [EN]
	* Opens the full-screen host window of the main dockspace, topOffset
	* pixels below the top of the main viewport (leaving room for the
	* toolbar), and submits the dockspace itself. Every panel drawn before
	* the matching DockSpaceEnd() can dock into it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メインビューポートの上端から topOffset ピクセル下(ツールバーの分を
	* 空ける)に、メインドックスペースを載せる全画面のホストウィンドウを開き、
	* ドックスペース本体を置く。対応する DockSpaceEnd() までに描画される
	* パネルはすべて、ここへドックできる。
	*/
	void ImGuiRenderer::DockSpaceBegin(Float topOffset)
	{
		/// [EN] The host window covers the main viewport's work area below topOffset.
		/// [JP] ホストウィンドウは、メインビューポートの作業領域のうち topOffset より下を覆う。
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + topOffset));
		ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - topOffset));
		ImGui::SetNextWindowViewport(viewport->ID);

		/// [EN] It is a fixed backdrop, not a window of its own: it cannot be docked, titled, collapsed, resized, moved, or brought in front of the panels.
		/// [JP] 独立したウィンドウではなく固定の背景: ドック、タイトル、折りたたみ、サイズ変更、移動ができず、パネルより手前にも出ない。
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

		/// [EN] No rounding, border or padding, so the dockspace fills the host window edge to edge.
		/// [JP] 角丸、枠線、余白を無くし、ドックスペースがホストウィンドウの端から端まで埋まるようにする。
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("MainDockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		/// [EN] Resolved inside the host window so the ID is the one the dockspace node actually uses (see DockSpaceID()).
		/// [JP] ホストウィンドウの中で解決することで、ドックスペースノードが実際に使う ID になる(DockSpaceID() 参照)。
		dockSpaceID_ = ImGui::GetID("ScDockSpace");
		ImGui::DockSpace(dockSpaceID_, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}

	/**
	* [EN]
	* Closes the host window opened by DockSpaceBegin().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DockSpaceBegin() が開いたホストウィンドウを閉じる。
	*/
	void ImGuiRenderer::DockSpaceEnd()
	{
		ImGui::End();
	}

	/**
	* [EN]
	* Returns the ImGuiID of the main dockspace ("ScDockSpace"), as
	* resolved inside DockSpaceBegin(). ImGui::GetID(const char*) hashes
	* using the current window's ID stack as a seed, so the ID is only the
	* real dockspace node's when computed inside DockSpaceBegin() - every
	* panel that docks into the main dockspace takes it from here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DockSpaceBegin() 内で解決されたメインドックスペース("ScDockSpace")の
	* ImGuiID を返す。ImGui::GetID(const char*) はカレントウィンドウの
	* ID スタックをシードにハッシュするため、DockSpaceBegin() の中で
	* 計算したときだけ実際のドックスペースノードの ID になる ――
	* メインドックスペースへドックするパネルはすべてここから取得する。
	*/
	ImGuiID ImGuiRenderer::DockSpaceID()const
	{
		return dockSpaceID_;
	}

	/**
	* [EN]
	* Sets the scale applied to every font ImGui draws, taking effect from
	* the next frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ImGui が描画する全フォントに掛かる倍率を設定する。次のフレームから
	* 反映される。
	*/
	void ImGuiRenderer::FontScale(Float scale)
	{
		fontScale_ = scale;
		ImGui::GetIO().FontGlobalScale = fontScale_;
	}

	/**
	* [EN]
	* Returns the scale applied to every font ImGui draws.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ImGui が描画する全フォントに掛かる倍率を返す。
	*/
	Float ImGuiRenderer::FontScale()const
	{
		return fontScale_;
	}

	/**
	* [EN]
	* Forwards a window message to ImGui's Win32 backend and returns its
	* result (non-zero means ImGui consumed the message). Also clears
	* ImGui's key state whenever the window regains focus, so keys
	* released while another window held focus never stay stuck down.
	* Does nothing before the ImGui context exists.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ウィンドウメッセージを ImGui の Win32 バックエンドへ渡し、その結果を
	* 返す(0 以外は ImGui がメッセージを処理したことを表す)。また、
	* ウィンドウがフォーカスを取り戻すたびに ImGui のキー状態をクリアし、
	* 別のウィンドウがフォーカスを持っている間に離されたキーが押されたまま
	* 残らないようにする。ImGui コンテキストが無い間は何もしない。
	*/
	LRESULT ImGuiRenderer::HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		/// [EN] Messages can arrive before Initialize (window creation) and after Finalize (window destruction).
		/// [JP] メッセージは Initialize より前(ウィンドウ作成時)や Finalize より後(ウィンドウ破棄時)にも届きうる。
		if (ImGui::GetCurrentContext() == nullptr)
		{
			return 0;
		}

		LRESULT result = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		/// [EN] ImGui lets a focus-gained event overwrite a focus-lost one queued in the same frame, so keys released while a modal dialog blocks the frame loop are never cleared by ImGui itself; clear them here when focus returns.
		/// [JP] ImGui は同じフレームに積まれたフォーカス喪失イベントをフォーカス復帰イベントで上書きするため、モーダルダイアログがフレームループを止めている間に離されたキーは ImGui 自身ではクリアされない。フォーカスが戻った時点でここでクリアする。
		if (msg == WM_SETFOCUS)
		{
			ImGui::GetIO().ClearInputKeys();
		}

		return result;
	}

	/**
	* [EN]
	* Descriptor allocation callback for the DirectX 12 backend: takes one
	* slot from the BindlessHeap stored in info->UserData and returns its
	* CPU and GPU handles.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DirectX 12 バックエンド用のディスクリプタ確保コールバック:
	* info->UserData に保存した BindlessHeap からスロットを 1 つ取り、
	* その CPU ハンドルと GPU ハンドルを返す。
	*/
	void ImGuiRenderer::AllocateDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
	{
		BindlessHeap* bindlessHeap = static_cast<BindlessHeap*>(info->UserData);
		Uint index = bindlessHeap->AllocateIndex();
		*cpuHandle = bindlessHeap->CPUHandle(index);
		*gpuHandle = bindlessHeap->GPUHandle(index);
	}

	/**
	* [EN]
	* Descriptor release callback for the DirectX 12 backend: returns the
	* slot behind cpuHandle to the BindlessHeap stored in info->UserData.
	* The heap defers the actual reuse until in-flight frames are done
	* with it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DirectX 12 バックエンド用のディスクリプタ解放コールバック: cpuHandle が
	* 指すスロットを、info->UserData に保存した BindlessHeap へ返す。
	* 実際の再利用は、インフライトのフレームが使い終わるまでヒープが遅らせる。
	*/
	void ImGuiRenderer::FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
	{
		/// [EN] The slot index is recovered from the CPU handle alone; the GPU handle points at the same slot.
		/// [JP] スロットのインデックスは CPU ハンドルだけから求める。GPU ハンドルは同じスロットを指している。
		BindlessHeap* bindlessHeap = static_cast<BindlessHeap*>(info->UserData);
		bindlessHeap->FreeIndex(bindlessHeap->Index(cpuHandle));
	}

	/**
	* [EN]
	* Loads Noto Sans JP with the full CJK glyph range into io's font
	* atlas and builds it. Returns whether the atlas was built.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CJK 全範囲のグリフを持つ Noto Sans JP を io のフォントアトラスへ
	* 読み込み、アトラスを構築する。構築できたかどうかを返す。
	*/
	Bool ImGuiRenderer::FontConfig(ImGuiIO& io)
	{
		/// [EN] No oversampling and no pixel snapping: the glyphs are rasterized once at their natural size and positioned at sub-pixel precision.
		/// [JP] オーバーサンプリングもピクセルスナップも行わない: グリフは本来の大きさで 1 回だけラスタライズし、サブピクセル精度で配置する。
		ImFontConfig fontConfig{};
		fontConfig.OversampleH = 1;
		fontConfig.OversampleV = 1;
		fontConfig.PixelSnapH = false;

		/// [EN] The Chinese full range covers every kanji Japanese text uses, on top of kana and ASCII.
		/// [JP] 中国語の全範囲は、かなと ASCII に加えて、日本語の文章が使う漢字をすべて含む。
		const ImWchar* japaneseFullRange = io.Fonts->GetGlyphRangesChineseFull();
		io.Fonts->AddFontFromFileTTF("../External/ImGui/Font/NotoSansJP-Regular.ttf", 17.6f, &fontConfig, japaneseFullRange);
		return io.Fonts->Build();
	}

	/**
	* [EN]
	* Applies the editor's look: compact paddings and spacings, rounded
	* corners, and a dark theme with purple accents.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタの見た目を適用する: 詰めた余白と間隔、角丸、紫をアクセントに
	* したダークテーマ。
	*/
	void ImGuiRenderer::StyleConfig()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		/// [EN] Paddings, spacings and sizes, in pixels - kept tight so dense panels fit more rows.
		/// [JP] 余白、間隔、大きさ(ピクセル単位)。密なパネルに多くの行が収まるよう詰めてある。
		style.WindowPadding = ImVec2(5, 5);
		style.FramePadding = ImVec2(7, 5);
		style.CellPadding = ImVec2(4, 2);
		style.ItemSpacing = ImVec2(7, 5);
		style.ItemInnerSpacing = ImVec2(4, 4);
		style.TouchExtraPadding = ImVec2(0, 0);
		style.IndentSpacing = 11;
		style.ScrollbarSize = 13;
		style.GrabMinSize = 9;

		/// [EN] Borders only on windows, child windows and popups; widgets and tabs stay flat.
		/// [JP] 枠線はウィンドウ、子ウィンドウ、ポップアップにだけ付け、ウィジェットとタブは平らにする。
		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.TabBorderSize = 0.0f;

		/// [EN] Corner rounding radii, in pixels.
		/// [JP] 角丸の半径(ピクセル単位)。
		style.WindowRounding = 7.0f;
		style.ChildRounding = 5.0f;
		style.FrameRounding = 5.0f;
		style.PopupRounding = 5.0f;
		style.ScrollbarRounding = 8.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 5.0f;

		/// [EN] Text and backgrounds: near-white text on dark blue-grey; child windows are transparent so they take their parent's background.
		/// [JP] 文字と背景: 暗い青灰色の上に白に近い文字。子ウィンドウは透明にして親の背景をそのまま使う。
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.22f, 0.22f, 0.24f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 0.98f);

		/// [EN] Borders: half-transparent purple, no shadow.
		/// [JP] 枠線: 半透明の紫。影は付けない。
		colors[ImGuiCol_Border] = ImVec4(0.50f, 0.40f, 0.70f, 0.50f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		/// [EN] Title and menu bars: dark, with the focused window's title in purple.
		/// [JP] タイトルバーとメニューバー: 暗めにし、フォーカス中のウィンドウのタイトルだけ紫にする。
		colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.35f, 0.25f, 0.55f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.17f, 0.80f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);

		/// [EN] Input fields and other framed widgets: grey at rest, tinted purple when hovered or active.
		/// [JP] 入力欄などの枠付きウィジェット: 通常は灰色、ホバー中や操作中は紫がかる。
		colors[ImGuiCol_FrameBg] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.45f, 0.35f, 0.65f, 0.45f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.45f, 0.35f, 0.65f, 0.60f);

		/// [EN] Tabs: muted purple, brighter when hovered or selected, dimmer when their dock node is unfocused.
		/// [JP] タブ: くすんだ紫。ホバー中や選択中は明るく、ドックノードにフォーカスが無いときは暗くする。
		colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.22f, 0.32f, 0.80f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.60f, 0.45f, 0.90f, 0.85f);
		colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.35f, 0.75f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.18f, 0.24f, 0.98f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.25f, 0.40f, 1.00f);

		/// [EN] Buttons: neutral grey at rest, the accent purple when hovered or pressed.
		/// [JP] ボタン: 通常は中間の灰色、ホバー中や押下中はアクセントの紫。
		colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.44f, 0.24f, 0.66f, 0.80f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.24f, 0.66f, 1.00f);

		/// [EN] Headers (collapsing headers, selectables, tree nodes): the accent purple, more opaque as the state intensifies.
		/// [JP] ヘッダー(折りたたみヘッダー、選択項目、ツリーノード): アクセントの紫。状態が強まるほど不透明にする。
		colors[ImGuiCol_Header] = ImVec4(0.44f, 0.24f, 0.66f, 0.40f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.24f, 0.66f, 0.70f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.24f, 0.66f, 1.00f);

		/// [EN] Separators and dock splitters: dim purple, brightening while hovered or dragged.
		/// [JP] 区切り線とドックの分割線: 暗い紫。ホバー中やドラッグ中は明るくなる。
		colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.22f, 0.55f, 0.50f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.55f, 0.35f, 0.85f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.55f, 0.35f, 0.85f, 1.00f);

		/// [EN] Check marks and slider grabs: bright purple so the value stands out.
		/// [JP] チェックマークとスライダーのつまみ: 値が目立つよう明るい紫にする。
		colors[ImGuiCol_CheckMark] = ImVec4(0.66f, 0.44f, 0.99f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.24f, 0.66f, 0.80f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.44f, 0.99f, 1.00f);

		/// [EN] Docking preview and resize grips: the accent purple, faint until interacted with.
		/// [JP] ドッキングのプレビューとリサイズのつまみ: アクセントの紫。操作するまでは薄くする。
		colors[ImGuiCol_DockingPreview] = ImVec4(0.44f, 0.24f, 0.66f, 0.70f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.24f, 0.66f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.24f, 0.66f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.24f, 0.66f, 0.95f);
	}
}

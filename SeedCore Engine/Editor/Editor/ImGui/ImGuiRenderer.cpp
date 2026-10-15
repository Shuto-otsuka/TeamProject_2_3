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
		bindlessHeap->Release(nullptr, bindlessHeap->Index(cpuHandle));
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
	* Applies dark graphite and midnight-blue surfaces, indigo highlights,
	* and clearly bordered controls for the editor's lunar interface.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 暗い黒鉛色と夜空の濃紺、鮮やかな青の選択表示、明瞭な境界線で、
	* 月面設備をイメージしたエディタスタイルを適用する。
	*/
	void ImGuiRenderer::StyleConfig()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.6f;
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.CellPadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
		style.IndentSpacing = 18.0f;
		style.ScrollbarSize = 11.0f;
		style.ScrollbarPadding = 3.0f;
		style.GrabMinSize = 10.0f;

		style.WindowRounding = 0.0f;
		style.ChildRounding = 2.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 3.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 1.0f;
		style.TabRounding = 2.0f;
		style.ImageRounding = 0.0f;
		style.MenuItemRounding = 1.0f;
		style.SelectableRounding = 0.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;
		style.TabBarBorderSize = 1.0f;
		style.TabBarOverlineSize = 2.0f;
		style.DockingSeparatorSize = 4.0f;
		style.SeparatorSize = 1.0f;
		style.SeparatorTextBorderSize = 1.0f;
		style.SeparatorTextPadding = ImVec2(8.0f, 5.0f);
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.InputTextCursorSize = 1.0f;
		style.DragDropTargetBorderSize = 1.0f;
		style.DragDropTargetRounding = 2.0f;

		const auto color = [](Uint rgb, Float alpha = 1.0f) -> ImVec4
		{
			return ImVec4(static_cast<Float>((rgb >> 16) & 0xFF) / 255.0f, static_cast<Float>((rgb >> 8) & 0xFF) / 255.0f, static_cast<Float>(rgb & 0xFF) / 255.0f, alpha);
		};

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = color(0xE4ECFA);
		colors[ImGuiCol_TextDisabled] = color(0x8392AD);
		colors[ImGuiCol_WindowBg] = color(0x131A27);
		colors[ImGuiCol_ChildBg] = color(0x131A27, 0.0f);
		colors[ImGuiCol_PopupBg] = color(0x1A2438);
		colors[ImGuiCol_Border] = color(0x5C7295);
		colors[ImGuiCol_BorderShadow] = color(0x000000, 0.0f);
		colors[ImGuiCol_FrameBg] = color(0x0D1422);
		colors[ImGuiCol_FrameBgHovered] = color(0x1F2D4B);
		colors[ImGuiCol_FrameBgActive] = color(0x263B71);
		colors[ImGuiCol_TitleBg] = color(0x0D1420);
		colors[ImGuiCol_TitleBgActive] = color(0x1D2A48);
		colors[ImGuiCol_TitleBgCollapsed] = color(0x0D1420);
		colors[ImGuiCol_MenuBarBg] = color(0x141D2E);
		colors[ImGuiCol_ScrollbarBg] = color(0x0D1420);
		colors[ImGuiCol_ScrollbarGrab] = color(0x3B5077);
		colors[ImGuiCol_ScrollbarGrabHovered] = color(0x5773AD);
		colors[ImGuiCol_ScrollbarGrabActive] = color(0x657EFF);
		colors[ImGuiCol_CheckMark] = color(0xA7BCFF);
		colors[ImGuiCol_CheckboxSelectedBg] = color(0x26386B);
		colors[ImGuiCol_SliderGrab] = color(0x657EFF);
		colors[ImGuiCol_SliderGrabActive] = color(0xA7BCFF);
		colors[ImGuiCol_Button] = color(0x1D2941);
		colors[ImGuiCol_ButtonHovered] = color(0x2D4184);
		colors[ImGuiCol_ButtonActive] = color(0x3E52AF);
		colors[ImGuiCol_Header] = color(0x293D78);
		colors[ImGuiCol_HeaderHovered] = color(0x304989);
		colors[ImGuiCol_HeaderActive] = color(0x3E52AF);
		colors[ImGuiCol_Separator] = color(0x4A5E80);
		colors[ImGuiCol_SeparatorHovered] = color(0x657EFF);
		colors[ImGuiCol_SeparatorActive] = color(0xA7BCFF);
		colors[ImGuiCol_ResizeGrip] = color(0x657EFF, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = color(0x657EFF, 0.70f);
		colors[ImGuiCol_ResizeGripActive] = color(0xA7BCFF);
		colors[ImGuiCol_InputTextCursor] = color(0xA7BCFF);
		colors[ImGuiCol_TabHovered] = color(0x2D4184);
		colors[ImGuiCol_Tab] = color(0x121B2D);
		colors[ImGuiCol_TabSelected] = color(0x293D78);
		colors[ImGuiCol_TabSelectedOverline] = color(0x8FA7FF);
		colors[ImGuiCol_TabDimmed] = color(0x101725);
		colors[ImGuiCol_TabDimmedSelected] = color(0x1D2A45);
		colors[ImGuiCol_TabDimmedSelectedOverline] = color(0x5773AD);
		colors[ImGuiCol_DockingPreview] = color(0x657EFF, 0.32f);
		colors[ImGuiCol_DockingEmptyBg] = color(0x080E17);
		colors[ImGuiCol_PlotLines] = color(0x7FA8FF);
		colors[ImGuiCol_PlotLinesHovered] = color(0xB4C9FF);
		colors[ImGuiCol_PlotHistogram] = color(0x657EFF);
		colors[ImGuiCol_PlotHistogramHovered] = color(0xA7BCFF);
		colors[ImGuiCol_TableHeaderBg] = color(0x1C2940);
		colors[ImGuiCol_TableBorderStrong] = color(0x5C7295);
		colors[ImGuiCol_TableBorderLight] = color(0x354765);
		colors[ImGuiCol_TableRowBg] = color(0x131A27, 0.0f);
		colors[ImGuiCol_TableRowBgAlt] = color(0xD5E3FF, 0.025f);
		colors[ImGuiCol_TextLink] = color(0x94B4FF);
		colors[ImGuiCol_TextSelectedBg] = color(0x5A72FF, 0.35f);
		colors[ImGuiCol_TreeLines] = color(0x4A5E80);
		colors[ImGuiCol_DragDropTarget] = color(0xA7BCFF);
		colors[ImGuiCol_DragDropTargetBg] = color(0x657EFF, 0.14f);
		colors[ImGuiCol_UnsavedMarker] = color(0x8FA7FF);
		colors[ImGuiCol_NavCursor] = color(0xA7BCFF);
		colors[ImGuiCol_NavWindowingHighlight] = color(0x94B4FF, 0.80f);
		colors[ImGuiCol_NavWindowingDimBg] = color(0x060A12, 0.55f);
		colors[ImGuiCol_ModalWindowDimBg] = color(0x060A12, 0.72f);
	}
}

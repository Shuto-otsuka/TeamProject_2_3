#pragma once
#include <FoundationEngine/Prelude.h>

struct ImGui_ImplDX12_InitInfo;

namespace SeedCore
{
	class BindlessHeap;

	/**
	* [EN]
	* Owns the editor's Dear ImGui context and drives its Win32 and DirectX 12
	* backends: per-frame begin/render, the main dockspace every panel docks
	* into, the global font scale, and routing window messages to ImGui.
	* ImGui draws out of the engine's BindlessHeap - its own textures are
	* allocated there through the backend's descriptor callbacks, and any
	* engine texture can be shown by its bindless SRV - so the editor needs
	* no separate shader-visible heap.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタの Dear ImGui コンテキストを所有し、Win32 と DirectX 12 の
	* バックエンドを動かす: フレームごとの開始と描画、全パネルがドックする
	* メインドックスペース、フォント全体の倍率、ウィンドウメッセージの ImGui
	* への受け渡し。ImGui はエンジンの BindlessHeap から描画する - ImGui 自身の
	* テクスチャはバックエンドのディスクリプタコールバック経由でそこに確保され、
	* エンジンのどのテクスチャもバインドレス SRV でそのまま表示できる - ため、
	* エディタ専用のシェーダー可視ヒープは持たない。
	*/
	class ImGuiRenderer
	{
	public:
		/**
		* [EN]
		* Constructs an uninitialized renderer; Initialize does the work.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未初期化のレンダラーを作る。実際の準備は Initialize が行う。
		*/
		ImGuiRenderer() = default;

		/**
		* [EN]
		* Destroys the renderer; Finalize must already have shut ImGui down.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レンダラーを破棄する。ImGui の終了は、先に Finalize で済ませておく。
		*/
		~ImGuiRenderer() = default;

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
		Bool Initialize(HWND hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, BindlessHeap* bindlessHeap, Int numberFramesInFlight);

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
		void NewFrame();

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
		void Render(ID3D12GraphicsCommandList* cmdList);

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
		void Finalize();

	public:
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
		void DockSpaceBegin(Float topOffset = 0.0f);

		/**
		* [EN]
		* Closes the host window opened by DockSpaceBegin().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DockSpaceBegin() が開いたホストウィンドウを閉じる。
		*/
		void DockSpaceEnd();

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
		[[nodiscard]] ImGuiID DockSpaceID()const;

	public:
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
		void FontScale(Float scale);

		/**
		* [EN]
		* Returns the scale applied to every font ImGui draws.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ImGui が描画する全フォントに掛かる倍率を返す。
		*/
		Float FontScale()const;

	public:
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
		static LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	private:
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
		static void AllocateDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);

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
		static void FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	private:
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
		Bool FontConfig(ImGuiIO& io);

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
		void StyleConfig();

	private:
		/// [EN] The engine's bindless heap ImGui draws out of and allocates its own textures from; not owned.
		/// [JP] ImGui が描画に使い、自身のテクスチャも確保する、エンジンのバインドレスヒープ。所有しない。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Scale applied to every font ImGui draws (mirrors ImGuiIO::FontGlobalScale).
		/// [JP] ImGui が描画する全フォントに掛かる倍率(ImGuiIO::FontGlobalScale と同じ値)。
		Float fontScale_ = 1.0f;

		/// [EN] ID of the main dockspace, resolved each frame in DockSpaceBegin().
		/// [JP] メインドックスペースのID。毎フレーム DockSpaceBegin() 内で解決される。
		ImGuiID dockSpaceID_ = 0;
	};
}

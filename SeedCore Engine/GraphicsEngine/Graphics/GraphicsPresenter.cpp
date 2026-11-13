#include <GraphicsEngine/Graphics/GraphicsPresenter.h>
#include <GraphicsEngine/Graphics/Graphics.h>

#include <FoundationEngine/Application/Window.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Resource/Gateway.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/SwapChain/SwapChain.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>

namespace SeedCore
{
	/**
	* [EN]
	* Keeps the device the swap chain is created on.
	* Nothing touches the GPU until Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スワップチェインを作るデバイスを受け取る。
	* Initialize まで GPU には触らない。
	*/
	GraphicsPresenter::GraphicsPresenter(RenderDevice& device) :device_(device)
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the presenter. GPU resources are released by Finalize beforehand.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示器を破棄する。GPU リソースは、その前に Finalize が解放する。
	*/
	GraphicsPresenter::~GraphicsPresenter() = default;

	/**
	* [EN]
	* Creates the swap chain at the window's client size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ウィンドウのクライアントサイズでスワップチェインを作る。
	*/
	Bool GraphicsPresenter::Initialize(const Window& window)
	{
		HWND hwnd = window.Handle();

		RECT clientRect{};
		GetClientRect(hwnd, &clientRect);
		Float width = static_cast<Float>(clientRect.right - clientRect.left);
		Float height = static_cast<Float>(clientRect.bottom - clientRect.top);

		D3D12Context& context = device_.Context();

		/// [EN] Presentation resources are created after the device and direct queue exist.
		/// [JP] 表示用リソースは、デバイスと direct キューの生成後に作成する。
		swapChain_ = MakePtr<SwapChain>(width, height);
		if (!swapChain_->Create(context.GetFactory(), context.GetDevice(), context.GetDirectQueue()->GetCommandQueue(), hwnd))
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Releases the swap chain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スワップチェインを解放する。
	*/
	void GraphicsPresenter::Finalize()
	{
		swapChain_.reset();
	}

	/**
	* [EN]
	* Applies vertical sync; only a change of frame generation recreates the swap chain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 垂直同期を反映する。スワップチェインを作り直すのは、フレーム生成が変わったときだけ。
	*/
	void GraphicsPresenter::Configure(const GraphicsSettings& settings)
	{
		swapChain_->VerticalSync(settings.verticalSync_);

		/// [EN] Frame generation hooks the swap chain at creation, so changing it recreates the swap chain while no GPU work references it.
		/// [JP] フレーム生成はスワップチェイン作成時にフックするので、変えるときは GPU が参照していない間にスワップチェインを作り直す。
		DlssManager& dlssManager = Gateway::GetDlssManager();
		if (dlssManager.FrameGenerationEnable() == settings.frameGeneration_)
		{
			return;
		}
		HWND hwnd = swapChain_->GetHwnd();
		device_.Wait();
		swapChain_->Destroy();
		dlssManager.FrameGenerationEnable(settings.frameGeneration_);
		D3D12Context& context = device_.Context();
		swapChain_->Create(context.GetFactory(), context.GetDevice(), context.GetDirectQueue()->GetCommandQueue(), hwnd);
	}

	/**
	* [EN]
	* Recreates presentation buffers after the output window size changes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 出力ウィンドウサイズの変更後に、表示用バッファを再作成する。
	*/
	void GraphicsPresenter::Resize(Uint32 width, Uint32 height)
	{
		/// [EN] Presentation buffers are replaced while frame generation is temporarily inactive.
		/// [JP] 表示用バッファは、フレーム生成を一時停止した状態で置き換える。
		DlssManager& dlssManager = Gateway::GetDlssManager();
		dlssManager.FrameGenerationSuppress(true);

		device_.Wait();

		swapChain_->Resize(device_.Context().GetDevice(), static_cast<Float>(width), static_cast<Float>(height));

		dlssManager.FrameGenerationSuppress(false);
	}

	/**
	* [EN]
	* Opens the DLSS presentation frame and makes the current back buffer writable as a render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DLSS の表示フレームを開始し、現在のバックバッファをレンダーターゲットとして書き込み可能にする。
	*/
	void GraphicsPresenter::Begin()
	{
		/// [EN] The Streamline frame token belongs to the presentation frame, not to a view, so it is acquired once here before any view tags DLSS inputs.
		/// [JP] Streamline のフレームトークンはビューではなく表示フレームに属するため、どのビューが DLSS 入力をタグ付けするより前に、ここで一度だけ取得する。
		DlssManager& dlssManager = Gateway::GetDlssManager();
		dlssManager.BeginFrame();
		dlssManager.EvaluateReflex();
		dlssManager.EvaluateFrameGeneration(1);

		/// [EN] Presentation owns the back buffer before this barrier; rendering owns it until End restores present state.
		/// [JP] このバリア前は表示処理がバックバッファを所有し、End で present 状態へ戻すまで描画処理が所有する。
		device_.Context().GetDirectList()->Barrier(swapChain_->BackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	/**
	* [EN]
	* Selects the current back buffer as the output-merger render target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のバックバッファを output-merger のレンダーターゲットとして選択する。
	*/
	void GraphicsPresenter::Bind()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = swapChain_->Handle();
		device_.Context().GetDirectList()->Get()->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);
	}

	/**
	* [EN]
	* Restores the back buffer to the present state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* バックバッファを present 状態へ戻す。
	*/
	void GraphicsPresenter::End()
	{
		/// [EN] Present requires the back buffer to leave the render-target state before command submission.
		/// [JP] 表示処理ではコマンド送信前に、バックバッファが render-target 状態を抜けている必要がある。
		device_.Context().GetDirectList()->Barrier(swapChain_->BackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	/**
	* [EN]
	* Presents the back buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* バックバッファを表示する。
	*/
	void GraphicsPresenter::Present()
	{
		/// [EN] Includes the wait for vertical sync when it is on.
		/// [JP] 垂直同期が有効なら、その待ち時間も含む。
		Benchmark::Begin("Present");
		swapChain_->Present(device_.Context().GetDevice());
		Benchmark::End("Present");
	}

	/**
	* [EN]
	* Returns the current back buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在のバックバッファを返す。
	*/
	ID3D12Resource* GraphicsPresenter::BackBuffer()const
	{
		return swapChain_->BackBuffer();
	}
}

#include <GraphicsEngine/Renderer/SubsurfaceScatteringRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the shared root signature and pipeline-state cache to the
	* subsurface scattering shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、表面下散乱
	* シェーダーへ関連付ける。
	*/
	SubsurfaceScatteringRenderer::SubsurfaceScatteringRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : subsurfaceScatteringShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once, then allocates the
	* size-dependent transmittance texture.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクトを 1 度だけ作成し、その後サイズに依存する
	* 透過率テクスチャを確保する。
	*/
	void SubsurfaceScatteringRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		/// [EN] Keep the heap and index systems; Allocate, Release and Prepare use them later.
		/// [JP] ヒープと各インデックスシステムを保持する。後で Allocate、Release、Prepare が使う。
		bindlessHeap_ = bindlessHeap;
		constantIndicesSystem_ = &constantIndicesSystem;
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;
		unorderedAccessIndicesSystem_ = &unorderedAccessIndicesSystem;
		/// [EN] Remember the render size; Allocate reads it.
		/// [JP] レンダーサイズを覚えておく。Allocate が読む。
		width_ = width;
		height_ = height;

		/// [EN] The shader and tuning buffer do not depend on the render size, so they are made only here.
		/// [JP] シェーダーと調整値のバッファはレンダーサイズに依存しないため、ここでだけ作る。
		subsurfaceScatteringShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<SubsurfaceScatteringRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Replaces the transmittance texture with one of the new size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 透過率テクスチャを新しいサイズのものに置き換える。
	*/
	void SubsurfaceScatteringRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
	{
		/// [EN] Give up the resources of the old size first.
		/// [JP] 先に古いサイズのリソースを手放す。
		Release();

		/// [EN] Remember the render size; Allocate reads it.
		/// [JP] レンダーサイズを覚えておく。Allocate が読む。
		width_ = width;
		height_ = height;

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Uploads the tuning values, stores the enabled flag for Dispatch, and
	* publishes the constant buffer and transmittance views to the index
	* systems.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 調整値をアップロードし、Dispatch 用に有効フラグを保存して、定数バッファと
	* 透過率テクスチャのビューを各インデックスシステムへ公開する。
	*/
	void SubsurfaceScatteringRenderer::Prepare(const SubsurfaceScatteringRayConstantBuffer& settings, Bool enabled)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(settings);

		/// [EN] The ray pass writes through the write view; deferred lighting reads through the read view.
		/// [JP] レイパスは書き込み用ビューで書き、ディファードライティングは読み取り用ビューで読む。
		constantIndicesSystem_->SetSubsurfaceScatteringRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetSubsurfaceScatteringTransmittanceUnorderedAccessViewIndex(transmittanceUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetSubsurfaceScatteringTransmittanceShaderResourceViewIndex(transmittanceShaderResourceViewIndex_);
	}

	/**
	* [EN]
	* Writes this view's transmittance: traced when enabled_ and the
	* pipeline exist, cleared to 0.0 otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このビューの透過率を書き込む。enabled_ でパイプラインがあればトレースし、
	* そうでなければ 0.0 でクリアする。
	*/
	void SubsurfaceScatteringRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] Both the trace and the clear write the texture, so it must be in unordered-access state first.
		/// [JP] トレースもクリアもテクスチャへ書き込むため、先に unordered-access 状態にする。
		if (transmittanceState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(transmittanceResource_.Get(), transmittanceState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			transmittanceState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		/// [EN] The pipeline is missing when the GPU lacks inline raytracing (DXR Tier 1.1); report it once.
		/// [JP] GPU がインラインレイトレーシング（DXR Tier 1.1）に非対応だとパイプラインが無い。1 度だけ報告する。
		ID3D12PipelineState* pipelineState = subsurfaceScatteringShader_.GetPipelineState();
		if (!pipelineState && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("SubsurfaceScatteringRT のコンピュート PSO 作成に失敗しています。DXR インラインレイトレ(Tier 1.1)非対応の可能性があります。透光は常に無し(0.0)として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		if (!enabled_ || !pipelineState)
		{
			/// [EN] Without a trace, 0.0 means "no light passes through", which leaves deferred lighting unchanged.
			/// [JP] トレースしない場合、0.0 は「光が透けない」を意味し、ディファードライティングを変化させない。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(transmittanceUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearIndex_), transmittanceResource_.Get(), clearValues, 0, nullptr);
		}
		else
		{
			/// [EN] The shader reads every input through the bindless heap and the shared root addresses.
			/// [JP] シェーダーはすべての入力を bindless ヒープと共有ルートアドレス経由で読む。
			ID3D12DescriptorHeap* heaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(heaps), heaps);
			/// [EN] All passes share one root signature; set it on the compute binding point.
			/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
			cmd->SetComputeRootSignature(subsurfaceScatteringShader_.GetRootSignature());
			/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
			/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
			RootSignature::BindCompute(cmd, addresses);
			cmd->SetPipelineState(pipelineState);

			/// [EN] One 8x8 thread group per 8x8 pixel tile, rounded up to cover the edges.
			/// [JP] 8x8 ピクセルのタイルごとに 8x8 のスレッドグループを 1 つ。端まで覆うよう切り上げる。
			Uint32 groupCountX = (width_ + 7) / 8;
			Uint32 groupCountY = (height_ + 7) / 8;
			cmd->Dispatch(groupCountX, groupCountY, 1);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();
		}

		/// [EN] Hand the texture over to deferred lighting as a shader resource.
		/// [JP] テクスチャをシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(transmittanceResource_.Get(), transmittanceState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		transmittanceState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the width_ x height_ R16 transmittance texture with its
	* bindless write view, its clear view and its bindless read view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の R16 透過率テクスチャと、bindless の書き込み用ビュー、
	* クリア用ビュー、bindless の読み取り用ビューを作成する。
	*/
	void SubsurfaceScatteringRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] A single 16-bit float channel is enough to hold a 0-1 transmittance.
		/// [JP] 0 から 1 の透過率を保持するには、16 ビット浮動小数点の 1 チャンネルで足りる。
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		/// [EN] One texture without mipmaps or multisampling, writable through an unordered-access view.
		/// [JP] テクスチャ 1 枚。ミップマップもマルチサンプルも無く、unordered-access ビューで書き込める。
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Width = width_;
		resourceDesc.Height = height_;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_R16_FLOAT;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		/// [EN] Create the texture in its own GPU-local heap, starting in COMMON state.
		/// [JP] テクスチャを専用の GPU ローカルなヒープに、COMMON 状態で作る。
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&transmittanceResource_));
		SC_HR_CHECK(hr, "透過率リソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		transmittanceResource_->SetName(L"SubsurfaceScattering_Transmittance");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(transmittanceResource_.Get());
#endif
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		transmittanceState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] The write view is created twice: once shader-visible for the trace, once CPU-side for the clear.
		/// [JP] 書き込み用ビューは 2 つ作る。トレース用のシェーダー可視のものと、クリア用の CPU 側のもの。
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		transmittanceUnorderedAccessViewIndex_ = bindlessHeap_->AllocateIndex();
		device->CreateUnorderedAccessView(transmittanceResource_.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(transmittanceUnorderedAccessViewIndex_));

		/// [EN] Recreate the CPU-side heap that holds the clear views, sized for every clearable resource.
		/// [JP] クリア用ビューを置く CPU 側のヒープを、クリアするリソースの数だけ作り直す。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
		clearIndex_ = clearHeap_.AllocateIndex();
		device->CreateUnorderedAccessView(transmittanceResource_.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(clearIndex_));

		/// [EN] The read view lets deferred lighting sample the texture.
		/// [JP] 読み取り用ビューで、ディファードライティングがテクスチャをサンプルできる。
		transmittanceShaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();
		/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
		/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(transmittanceResource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(transmittanceShaderResourceViewIndex_));
	}

	/**
	* [EN]
	* Frees the bindless views and hands the texture to the heap's deferred
	* release, since commands recorded in earlier frames may still use it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bindless のビューを解放し、テクスチャはヒープの遅延解放へ渡す。
	* 以前のフレームで記録したコマンドがまだ使っている可能性があるため。
	*/
	void SubsurfaceScatteringRenderer::Release()
	{
		/// [EN] Return the view slots to the bindless heap.
		/// [JP] ビューのスロットを bindless ヒープへ返す。
		bindlessHeap_->FreeIndex(transmittanceUnorderedAccessViewIndex_);
		bindlessHeap_->FreeIndex(transmittanceShaderResourceViewIndex_);

		/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
		/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
		bindlessHeap_->DeferRelease(transmittanceResource_);
		transmittanceResource_.Reset();
	}
}

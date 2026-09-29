#include <GraphicsEngine/Renderer/AmbientOcclusionRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Gateway.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the shared root signature and pipeline-state cache to the
	* occlusion and denoise shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、遮蔽と
	* デノイズのシェーダーへ関連付ける。
	*/
	AmbientOcclusionRenderer::AmbientOcclusionRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : ambientOcclusionShader_(rootSignature, pipelineStateObject), denoiseShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once, then allocates the
	* size-dependent textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクトを 1 度だけ作成し、その後サイズに依存する
	* テクスチャを確保する。
	*/
	void AmbientOcclusionRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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

		/// [EN] The shaders and tuning buffer do not depend on the render size, so they are made only here.
		/// [JP] シェーダーと調整値のバッファはレンダーサイズに依存しないため、ここでだけ作る。
		ambientOcclusionShader_.Create(shaderCache, device);
		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		denoiseShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<AmbientOcclusionRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Replaces every texture with one of the new size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべてのテクスチャを新しいサイズのものに置き換える。
	*/
	void AmbientOcclusionRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	* Advances the accumulation slots, uploads the tuning values, stores the
	* flags for Dispatch, and publishes the indices each view's passes and
	* deferred lighting read.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 蓄積のスロットを進め、調整値をアップロードし、Dispatch 用にフラグを
	* 保存して、各ビューのパスとディファードライティングが読むインデックスを
	* 公開する。
	*/
	void AmbientOcclusionRenderer::Prepare(const AmbientOcclusionRayConstantBuffer& settings, Bool enabled)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;
		/// [EN] Ask the DLSS manager whether Ray Reconstruction does the denoising this frame.
		/// [JP] 今フレームのデノイズを Ray Reconstruction が行うかを DLSS マネージャーに問い合わせる。
		useDlssRayReconstruction_ = Gateway::GetDlssManager().RayReconstructionEnable();

		/// [EN] Swap once per frame: last frame's write slot becomes this frame's history.
		/// [JP] 1 フレームに 1 回入れ替える。前フレームの書き込みスロットが今フレームの履歴になる。
		historySlot_ = 1 - historySlot_;
		/// [EN] The slot written this frame is the one that is not the history.
		/// [JP] 今フレームに書き込むのは、履歴ではない方のスロット。
		Uint32 writeSlot = 1 - historySlot_;

		/// [EN] The upload copy carries the frame counter on top of the user's tuning.
		/// [JP] アップロード用のコピーには、ユーザーの調整値に加えてフレームカウンターを載せる。
		AmbientOcclusionRayConstantBuffer uploadSettings = settings;
		uploadSettings.frameIndex_ = frameIndex_;
		/// [EN] Advance the counter for the next frame.
		/// [JP] 次のフレームのためにカウンターを進める。
		frameIndex_++;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);
		constantIndicesSystem_->SetAmbientOcclusionRayConstantIndex(tuningBuffer_->GetIndex());

		/// [EN] The trace always writes the raw texture.
		/// [JP] トレースは常に生のテクスチャへ書き込む。
		unorderedAccessIndicesSystem_->SetAmbientOcclusionRawUnorderedAccessViewIndex(rawOpennessUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetAmbientOcclusionRawShaderResourceViewIndex(rawOpennessShaderResourceViewIndex_);

		/// [EN] Array indices of the editor and game views.
		/// [JP] エディタービューとゲームビューの配列インデックス。
		constexpr Uint32 editorView = static_cast<Uint32>(RaytracingView::Editor);
		constexpr Uint32 gameView = static_cast<Uint32>(RaytracingView::Game);

		/// [EN] The denoiser writes each view's write slot either way.
		/// [JP] デノイザはどちらの場合も各ビューの書き込みスロットへ書き込む。
		AmbientOcclusionAccumulationUnorderedAccessIndices editorUnorderedAccessIndices{};
		editorUnorderedAccessIndices.accumulatedIndex_ = accumulatedUnorderedAccessViewIndex_[editorView][writeSlot];
		unorderedAccessIndicesSystem_->SetEditorAmbientOcclusionAccumulationIndices(editorUnorderedAccessIndices);

		AmbientOcclusionAccumulationUnorderedAccessIndices gameUnorderedAccessIndices{};
		gameUnorderedAccessIndices.accumulatedIndex_ = accumulatedUnorderedAccessViewIndex_[gameView][writeSlot];
		unorderedAccessIndicesSystem_->SetGameAmbientOcclusionAccumulationIndices(gameUnorderedAccessIndices);

		if (useDlssRayReconstruction_)
		{
			/// [EN] DLSS Ray Reconstruction denoises the composited frame, so both views read the raw texture directly and the accumulation chain is left alone.
			/// [JP] DLSS Ray Reconstruction が合成したフレームをデノイズするため、両ビューとも生のテクスチャを直接読み、蓄積のチェーンには触れない。
			AmbientOcclusionAccumulationShaderResourceIndices rawShaderResourceIndices{};
			rawShaderResourceIndices.historyIndex_ = rawOpennessShaderResourceViewIndex_;
			rawShaderResourceIndices.opennessIndex_ = rawOpennessShaderResourceViewIndex_;
			shaderResourceIndicesSystem_->SetEditorAmbientOcclusionAccumulationIndices(rawShaderResourceIndices);
			shaderResourceIndicesSystem_->SetGameAmbientOcclusionAccumulationIndices(rawShaderResourceIndices);
		}
		else
		{
			/// [EN] Each view's denoiser reads its history slot, and deferred lighting reads its write slot.
			/// [JP] 各ビューのデノイザは履歴のスロットを読み、ディファードライティングは書き込みスロットを読む。
			AmbientOcclusionAccumulationShaderResourceIndices editorShaderResourceIndices{};
			editorShaderResourceIndices.historyIndex_ = accumulatedShaderResourceViewIndex_[editorView][historySlot_];
			editorShaderResourceIndices.opennessIndex_ = accumulatedShaderResourceViewIndex_[editorView][writeSlot];
			shaderResourceIndicesSystem_->SetEditorAmbientOcclusionAccumulationIndices(editorShaderResourceIndices);

			AmbientOcclusionAccumulationShaderResourceIndices gameShaderResourceIndices{};
			gameShaderResourceIndices.historyIndex_ = accumulatedShaderResourceViewIndex_[gameView][historySlot_];
			gameShaderResourceIndices.opennessIndex_ = accumulatedShaderResourceViewIndex_[gameView][writeSlot];
			shaderResourceIndicesSystem_->SetGameAmbientOcclusionAccumulationIndices(gameShaderResourceIndices);
		}
	}

	/**
	* [EN]
	* Writes view's openness: traced (and denoised unless DLSS Ray
	* Reconstruction is on) when enabled_ and the needed pipelines exist,
	* otherwise the texture deferred lighting reads is cleared to 1.0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* view の開放度を書き込む。enabled_ で必要なパイプラインがあればトレース
	* し（DLSS Ray Reconstruction が無効ならデノイズも行う）、そうでなければ
	* ディファードライティングが読むテクスチャを 1.0 でクリアする。
	*/
	void AmbientOcclusionRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] Array index of this view's own buffers.
		/// [JP] このビュー専用のバッファの配列インデックス。
		Uint32 viewIndex = static_cast<Uint32>(view);
		/// [EN] The slot written this frame is the one that is not the history.
		/// [JP] 今フレームに書き込むのは、履歴ではない方のスロット。
		Uint32 writeSlot = 1 - historySlot_;

		/// [EN] The denoise pipeline only counts as required when this pass does its own denoising.
		/// [JP] デノイズのパイプラインは、このパスが自分でデノイズするときだけ必須とみなす。
		ID3D12PipelineState* ambientOcclusionPipelineState = ambientOcclusionShader_.GetPipelineState();
		ID3D12PipelineState* denoisePipelineState = denoiseShader_.GetPipelineState();
		Bool denoisePipelineRequired = !useDlssRayReconstruction_;
		Bool pipelinesReady = ambientOcclusionPipelineState && (!denoisePipelineRequired || denoisePipelineState);

		/// [EN] The pipelines are missing when the GPU lacks inline raytracing (DXR Tier 1.1); report it once.
		/// [JP] GPU がインラインレイトレーシング（DXR Tier 1.1）に非対応だとパイプラインが無い。1 度だけ報告する。
		if (!pipelinesReady && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("AmbientOcclusionRT/AmbientOcclusionDenoise のコンピュート PSO 作成に失敗しています。DXR インラインレイトレ(Tier 1.1)非対応の可能性があります。AO は常に開放(1.0)として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		/// [EN] 1.0 means fully open, i.e. no occlusion.
		/// [JP] 1.0 は完全に開放、つまり遮蔽なしを意味する。
		const Float clearValues[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		if (!enabled_ || !pipelinesReady)
		{
			if (useDlssRayReconstruction_)
			{
				/// [EN] Deferred lighting reads the raw texture on this path, so that is what gets cleared.
				/// [JP] この経路ではディファードライティングが生のテクスチャを読むため、それをクリアする。
				if (rawOpennessState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
				{
					cmdList->Barrier(rawOpennessResource_.Get(), rawOpennessState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					rawOpennessState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				}

				cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(rawOpennessUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearRawIndex_), rawOpennessResource_.Get(), clearValues, 0, nullptr);

				cmdList->Barrier(rawOpennessResource_.Get(), rawOpennessState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				rawOpennessState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
				return;
			}

			/// [EN] Otherwise deferred lighting reads this view's write slot, so that is what gets cleared.
			/// [JP] そうでなければディファードライティングはこのビューの書き込みスロットを読むため、それをクリアする。
			if (accumulatedOpennessState_[viewIndex][writeSlot] != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				cmdList->Barrier(accumulatedOpennessResource_[viewIndex][writeSlot].Get(), accumulatedOpennessState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				accumulatedOpennessState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}

			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(accumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot]), clearHeap_.CPUHandle(clearAccumulatedIndex_[viewIndex][writeSlot]), accumulatedOpennessResource_[viewIndex][writeSlot].Get(), clearValues, 0, nullptr);

			cmdList->Barrier(accumulatedOpennessResource_[viewIndex][writeSlot].Get(), accumulatedOpennessState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			accumulatedOpennessState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			return;
		}

		/// [EN] Trace the raw openness.
		/// [JP] 生の開放度をトレースする。
		if (rawOpennessState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(rawOpennessResource_.Get(), rawOpennessState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			rawOpennessState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		/// [EN] Make the bindless heap the active shader-visible heap.
		/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		/// [EN] All passes share one root signature; set it on the compute binding point.
		/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
		cmd->SetComputeRootSignature(ambientOcclusionShader_.GetRootSignature());
		/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
		/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
		RootSignature::BindCompute(cmd, addresses);
		cmd->SetPipelineState(ambientOcclusionPipelineState);

		/// [EN] One 8x8 thread group per 8x8 pixel tile, rounded up to cover the edges; the denoiser uses the same grid.
		/// [JP] 8x8 ピクセルのタイルごとに 8x8 のスレッドグループを 1 つ。端まで覆うよう切り上げる。デノイザも同じグリッドを使う。
		Uint32 groupCountX = (width_ + 7) / 8;
		Uint32 groupCountY = (height_ + 7) / 8;
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		if (useDlssRayReconstruction_)
		{
			/// [EN] DLSS Ray Reconstruction does the denoising, so the raw texture goes straight to deferred lighting.
			/// [JP] デノイズは DLSS Ray Reconstruction が行うため、生のテクスチャをそのままディファードライティングへ渡す。
			cmdList->Barrier(rawOpennessResource_.Get(), rawOpennessState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			rawOpennessState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			return;
		}

		/// [EN] Denoise: read the raw texture and the history slot, write this view's write slot.
		/// [JP] デノイズする。生のテクスチャと履歴のスロットを読み、このビューの書き込みスロットへ書く。
		cmdList->Barrier(rawOpennessResource_.Get(), rawOpennessState_, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		rawOpennessState_ = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		if (accumulatedOpennessState_[viewIndex][historySlot_] != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
		{
			cmdList->Barrier(accumulatedOpennessResource_[viewIndex][historySlot_].Get(), accumulatedOpennessState_[viewIndex][historySlot_], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			accumulatedOpennessState_[viewIndex][historySlot_] = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}

		if (accumulatedOpennessState_[viewIndex][writeSlot] != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(accumulatedOpennessResource_[viewIndex][writeSlot].Get(), accumulatedOpennessState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			accumulatedOpennessState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		cmd->SetPipelineState(denoisePipelineState);
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] Hand the denoised result over to deferred lighting as a shader resource.
		/// [JP] デノイズした結果をシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(accumulatedOpennessResource_[viewIndex][writeSlot].Get(), accumulatedOpennessState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		accumulatedOpennessState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the raw texture and the accumulation textures of every view
	* and slot, all width_ x height_ R16_FLOAT.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 生のテクスチャと、全ビュー・全スロットの蓄積テクスチャを作成する。
	* すべて width_ x height_ の R16_FLOAT。
	*/
	void AmbientOcclusionRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] One clear view for the raw texture plus one per accumulation texture.
		/// [JP] 生のテクスチャに 1 つ、蓄積テクスチャごとに 1 つのクリア用ビュー。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 + viewCount_ * accumulationSlotCount_, false);

		/// [EN] Creates one R16_FLOAT openness texture with its bindless write view, its clear view and its bindless read view. A single 16-bit float channel is enough for a 0-1 openness.
		/// [JP] bindless の書き込み用ビュー、クリア用ビュー、bindless の読み取り用ビューを持つ R16_FLOAT の開放度テクスチャを 1 つ作る。0 から 1 の開放度には 16 ビット浮動小数点の 1 チャンネルで足りる。
		auto createOpennessTexture = [this, device](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& unorderedAccessViewIndex, Uint32& shaderResourceViewIndex, Uint32& clearIndex)
		{
			/// [EN] GPU-local memory: only the GPU reads and writes these resources.
			/// [JP] GPU ローカルなメモリ。これらのリソースは GPU だけが読み書きする。
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
			HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
			SC_HR_CHECK(hr, "オープンネステクスチャの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(L"AmbientOcclusion_Openness");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

			/// [EN] The write view sees the whole resource in its own format.
			/// [JP] 書き込み用ビューは、リソース全体を自身のフォーマットで見る。
			D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
			unorderedAccessViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
			unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			/// [EN] Reserve a bindless slot and write the write view into it.
			/// [JP] bindless のスロットを確保し、書き込み用ビューを入れる。
			unorderedAccessViewIndex = bindlessHeap_->AllocateIndex();
			device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(unorderedAccessViewIndex));

			/// [EN] A second copy of the write view in the CPU-side heap, for clears.
			/// [JP] クリア用に、書き込み用ビューの 2 つ目を CPU 側のヒープに作る。
			clearIndex = clearHeap_.AllocateIndex();
			device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(clearIndex));

			/// [EN] Reserve a bindless slot for the read view.
			/// [JP] 読み取り用ビューのための bindless のスロットを確保する。
			shaderResourceViewIndex = bindlessHeap_->AllocateIndex();
			/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
			/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(shaderResourceViewIndex));
		};

		createOpennessTexture(rawOpennessResource_, rawOpennessUnorderedAccessViewIndex_, rawOpennessShaderResourceViewIndex_, clearRawIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		rawOpennessState_ = D3D12_RESOURCE_STATE_COMMON;

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				createOpennessTexture(accumulatedOpennessResource_[viewIndex][slotIndex], accumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], accumulatedShaderResourceViewIndex_[viewIndex][slotIndex], clearAccumulatedIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				accumulatedOpennessState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}
		}
	}

	/**
	* [EN]
	* Frees every texture's bindless views and hands the textures to the
	* heap's deferred release, since commands recorded in earlier frames may
	* still use them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべてのテクスチャの bindless ビューを解放し、テクスチャはヒープの
	* 遅延解放へ渡す。以前のフレームで記録したコマンドがまだ使っている
	* 可能性があるため。
	*/
	void AmbientOcclusionRenderer::Release()
	{
		/// [EN] Return the view slots to the bindless heap.
		/// [JP] ビューのスロットを bindless ヒープへ返す。
		bindlessHeap_->FreeIndex(rawOpennessUnorderedAccessViewIndex_);
		bindlessHeap_->FreeIndex(rawOpennessShaderResourceViewIndex_);
		/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
		/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
		bindlessHeap_->DeferRelease(rawOpennessResource_);
		rawOpennessResource_.Reset();

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				/// [EN] Return the view slots to the bindless heap.
				/// [JP] ビューのスロットを bindless ヒープへ返す。
				bindlessHeap_->FreeIndex(accumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex]);
				bindlessHeap_->FreeIndex(accumulatedShaderResourceViewIndex_[viewIndex][slotIndex]);
				/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
				/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
				bindlessHeap_->DeferRelease(accumulatedOpennessResource_[viewIndex][slotIndex]);
				accumulatedOpennessResource_[viewIndex][slotIndex].Reset();
			}
		}
	}
}

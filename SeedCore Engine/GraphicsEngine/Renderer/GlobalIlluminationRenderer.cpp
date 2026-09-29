#include <GraphicsEngine/Renderer/GlobalIlluminationRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Gateway.h>

#include <GraphicsEngine/D3D12/Buffer/ReservoirBuffer.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the shared root signature to the ray pass's raytracing-state
	* cache and to the denoise passes' pipeline-state cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャを、レイのパスのレイトレーシングステート
	* キャッシュと、デノイズのパスのパイプラインステートキャッシュへ
	* 関連付ける。
	*/
	GlobalIlluminationRenderer::GlobalIlluminationRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject, PipelineStateObject& pipelineStateObject) : globalIlluminationShader_(rootSignature, raytracingStateObject), denoiseShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once - pipelines, tuning buffer
	* and shader table - then allocates the size-dependent textures and
	* reservoirs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクト（パイプライン、調整値のバッファ、
	* シェーダーテーブル）を 1 度だけ作成し、その後サイズに依存する
	* テクスチャと reservoir を確保する。
	*/
	void GlobalIlluminationRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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

		/// [EN] The device is always created as ID3D12Device5 (see D3D12Device), which raytracing pipelines require.
		/// [JP] デバイスは常に ID3D12Device5 として生成される（D3D12Device 参照）。レイトレーシングパイプラインにはこれが必要。
		ID3D12Device5* device5 = static_cast<ID3D12Device5*>(device);

		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		globalIlluminationShader_.Create(shaderCache, device5);
		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		denoiseShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<GlobalIlluminationRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);

		/// [EN] Build the shader table. With only a global root signature, each record is just the 32-byte shader identifier taken from the state object.
		/// [JP] シェーダーテーブルを構築する。グローバルルートシグネチャだけなので、各レコードはステートオブジェクトから得る 32 バイトのシェーダー識別子のみ。
		ID3D12StateObject* stateObject = globalIlluminationShader_.GetStateObject();
		if (!stateObject)
		{
			return;
		}

		/// [EN] The properties interface is what looks up shader identifiers by export name.
		/// [JP] プロパティのインターフェースで、エクスポート名からシェーダー識別子を引く。
		Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		HRESULT hr = stateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties));
		if (FAILED(hr))
		{
			return;
		}

		void* rayGenIdentifier = stateObjectProperties->GetShaderIdentifier(String(GlobalIlluminationShader::rayGenExportName).w_str().c_str());
		void* missIdentifier = stateObjectProperties->GetShaderIdentifier(String(GlobalIlluminationShader::missExportName).w_str().c_str());
		void* hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(String(GlobalIlluminationShader::hitGroupName).w_str().c_str());
		if (!rayGenIdentifier || !missIdentifier || !hitGroupIdentifier)
		{
			return;
		}

		/// [EN] The table is written once from the CPU, so it lives in an upload heap.
		/// [JP] テーブルは CPU から 1 度書くだけなので、アップロードヒープに置く。
		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		/// [EN] A plain byte buffer holding the shader records.
		/// [JP] シェーダーのレコードを入れる、ただのバイトバッファ。
		D3D12_RESOURCE_DESC tableDesc{};
		tableDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		tableDesc.Width = shaderTableRecordSize_ * 3;
		tableDesc.Height = 1;
		tableDesc.DepthOrArraySize = 1;
		tableDesc.MipLevels = 1;
		tableDesc.Format = DXGI_FORMAT_UNKNOWN;
		tableDesc.SampleDesc.Count = 1;
		tableDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		/// [EN] Create the table buffer in the upload heap.
		/// [JP] テーブルのバッファをアップロードヒープに作る。
		hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &tableDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&shaderTableResource_));
		SC_HR_CHECK(hr, "GI シェーダーテーブルリソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		shaderTableResource_->SetName(L"GlobalIllumination_ShaderTable");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(shaderTableResource_.Get());
#endif

		/// [EN] Ray generation, miss and hit group in record order, each followed by zeros up to the record size.
		/// [JP] レイ生成、ミス、ヒットグループをレコード順に並べ、それぞれレコードサイズまで 0 で埋める。
		Uint8* mapped = nullptr;
		hr = shaderTableResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		SC_HR_CHECK(hr, "GI シェーダーテーブルリソースのMapに失敗しました");
		memset(mapped, 0, shaderTableRecordSize_ * 3);
		memcpy(mapped + shaderTableRecordSize_ * 0, rayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		memcpy(mapped + shaderTableRecordSize_ * 1, missIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		memcpy(mapped + shaderTableRecordSize_ * 2, hitGroupIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		/// [EN] The table is written once, so unmap it right away.
		/// [JP] テーブルは 1 度書くだけなので、すぐに Unmap する。
		shaderTableResource_->Unmap(0, nullptr);
	}

	/**
	* [EN]
	* Replaces every texture and reservoir with one of the new size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべてのテクスチャと reservoir を新しいサイズのものに置き換える。
	*/
	void GlobalIlluminationRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	* Advances the history slots, uploads the tuning values, stores the
	* flags for Dispatch, and publishes the indices each view's passes and
	* deferred lighting read.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 履歴のスロットを進め、調整値をアップロードし、Dispatch 用にフラグを
	* 保存して、各ビューのパスとディファードライティングが読むインデックスを
	* 公開する。
	*/
	void GlobalIlluminationRenderer::Prepare(const GlobalIlluminationRayConstantBuffer& settings, Bool enabled)
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

		/// [EN] The upload copy carries the frame counter and turns off the reservoir's temporal reuse when DLSS Ray Reconstruction handles the temporal side.
		/// [JP] アップロード用のコピーにはフレームカウンターを載せ、DLSS Ray Reconstruction が時間方向を担うときは reservoir の時間方向の再利用を切る。
		GlobalIlluminationRayConstantBuffer uploadSettings = settings;
		uploadSettings.frameIndex_ = frameIndex_;
		uploadSettings.temporalReuseEnabled_ = useDlssRayReconstruction_ ? 0 : 1;
		/// [EN] Advance the counter for the next frame.
		/// [JP] 次のフレームのためにカウンターを進める。
		frameIndex_++;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);

		/// [EN] The ray pass and the spatial reuse write the raw radiance and confidence; the denoiser reads them.
		/// [JP] レイのパスと空間的リユースが生の放射輝度と信頼度を書き、デノイザがそれを読む。
		constantIndicesSystem_->SetGlobalIlluminationRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetGlobalIlluminationOutputUnorderedAccessViewIndex(radianceUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetGlobalIlluminationOutputShaderResourceViewIndex(radianceShaderResourceViewIndex_);
		unorderedAccessIndicesSystem_->SetGlobalIlluminationConfidenceUnorderedAccessViewIndex(confidenceUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetGlobalIlluminationConfidenceShaderResourceViewIndex(confidenceShaderResourceViewIndex_);

		/// [EN] Array indices of the editor and game views.
		/// [JP] エディタービューとゲームビューの配列インデックス。
		constexpr Uint32 editorView = static_cast<Uint32>(RaytracingView::Editor);
		constexpr Uint32 gameView = static_cast<Uint32>(RaytracingView::Game);

		/// [EN] The read side of one view: history and final radiance, the A-Trous scratch pair, and both reservoir slots.
		/// [JP] 1 つのビューの読み取り側。履歴と最終の放射輝度、A-Trous のスクラッチ 2 枚、reservoir の両スロット。
		auto buildShaderResourceIndices = [&](Uint32 viewIndex)
		{
			GlobalIlluminationAccumulationShaderResourceIndices values{};

			if (useDlssRayReconstruction_)
			{
				/// [EN] DLSS Ray Reconstruction denoises the composited frame, so deferred lighting reads the raw radiance and the accumulation chain is left alone.
				/// [JP] DLSS Ray Reconstruction が合成したフレームをデノイズするため、ディファードライティングは生の放射輝度を読み、蓄積のチェーンには触れない。
				values.historyIndex_ = radianceShaderResourceViewIndex_;
				values.radianceIndex_ = radianceShaderResourceViewIndex_;
			}
			else
			{
				values.historyIndex_ = accumulatedShaderResourceViewIndex_[viewIndex][historySlot_];
				values.radianceIndex_ = accumulatedShaderResourceViewIndex_[viewIndex][writeSlot];
			}

			values.atrousScratch0Index_ = atrousScratchShaderResourceViewIndex_[viewIndex][0];
			values.atrousScratch1Index_ = atrousScratchShaderResourceViewIndex_[viewIndex][1];

			/// [EN] The reservoir is used on both denoise paths and follows the same slot pair as the accumulation.
			/// [JP] reservoir はどちらのデノイズ経路でも使い、蓄積と同じスロットの組に従う。
			values.reservoirHistoryIndex_ = reservoirShaderResourceViewIndex_[viewIndex][historySlot_];
			values.reservoirWriteIndex_ = reservoirShaderResourceViewIndex_[viewIndex][writeSlot];

			return values;
		};

		/// [EN] The write side of one view: this frame's accumulation slot, the A-Trous scratch pair and this frame's reservoir slot.
		/// [JP] 1 つのビューの書き込み側。今フレームの蓄積スロット、A-Trous のスクラッチ 2 枚、今フレームの reservoir のスロット。
		auto buildUnorderedAccessIndices = [&](Uint32 viewIndex)
		{
			GlobalIlluminationAccumulationUnorderedAccessIndices values{};

			values.accumulatedIndex_ = accumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.atrousScratch0Index_ = atrousScratchUnorderedAccessViewIndex_[viewIndex][0];
			values.atrousScratch1Index_ = atrousScratchUnorderedAccessViewIndex_[viewIndex][1];
			values.reservoirIndex_ = reservoirUnorderedAccessViewIndex_[viewIndex][writeSlot];

			return values;
		};

		shaderResourceIndicesSystem_->SetEditorGlobalIlluminationAccumulationIndices(buildShaderResourceIndices(editorView));
		shaderResourceIndicesSystem_->SetGameGlobalIlluminationAccumulationIndices(buildShaderResourceIndices(gameView));
		unorderedAccessIndicesSystem_->SetEditorGlobalIlluminationAccumulationIndices(buildUnorderedAccessIndices(editorView));
		unorderedAccessIndicesSystem_->SetGameGlobalIlluminationAccumulationIndices(buildUnorderedAccessIndices(gameView));
	}

	/**
	* [EN]
	* Writes view's indirect light: traced, spatially reused and (unless
	* DLSS Ray Reconstruction is on) denoised when enabled_ and every needed
	* pipeline exist, otherwise the texture deferred lighting reads and this
	* frame's reservoir are cleared.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* view の間接光を書き込む。enabled_ で必要なパイプラインがすべてあれば
	* トレースし、空間的リユースを行い、DLSS Ray Reconstruction が無効なら
	* デノイズもする。そうでなければ、ディファードライティングが読む
	* テクスチャと今フレームの reservoir をクリアする。
	*/
	void GlobalIlluminationRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view)
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

		/// [EN] Moves a resource into shader-read state if it is not there yet.
		/// [JP] リソースがまだシェーダーの読み取り状態でなければ、その状態へ移す。
		auto toRead = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state)
		{
			if (state != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			{
				cmdList->Barrier(resource.Get(), state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
		};

		/// [EN] Moves a resource into unordered-access state if it is not there yet.
		/// [JP] リソースがまだ unordered-access 状態でなければ、その状態へ移す。
		auto toWrite = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state)
		{
			if (state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				cmdList->Barrier(resource.Get(), state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
		};

		/// [EN] Clears one reservoir to zero through its shader-visible and clear-heap write views.
		/// [JP] シェーダー可視とクリア用ヒープの書き込みビューを使って、reservoir を 1 つ 0 でクリアする。
		auto clearReservoir = [&](Uint32 clearView, Uint32 clearSlot)
		{
			toWrite(reservoirResource_[clearView][clearSlot], reservoirState_[clearView][clearSlot]);

			const Uint32 zeroValues[4] = { 0, 0, 0, 0 };
			cmd->ClearUnorderedAccessViewUint(bindlessHeap_->GPUHandle(clearReservoirGpuIndex_[clearView][clearSlot]), clearHeap_.CPUHandle(clearReservoirIndex_[clearView][clearSlot]), reservoirResource_[clearView][clearSlot].Get(), zeroValues, 0, nullptr);
		};

		/// [EN] Zero both reservoir slots of every view once after allocation, so no uninitialized M_ or W_ is ever read back as history.
		/// [JP] 確保の後に 1 度だけ、全ビューの reservoir の両スロットを 0 で埋める。未初期化の M_ や W_ が履歴として読み戻されないようにする。
		if (!reservoirCleared_)
		{
			/// [EN] Mark it done, so this happens only once.
			/// [JP] 済んだ印を付け、1 度だけ行うようにする。
			reservoirCleared_ = true;

			ID3D12DescriptorHeap* clearHeaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(clearHeaps), clearHeaps);

			for (Uint32 clearView = 0; clearView < viewCount_; clearView++)
			{
				for (Uint32 clearSlot = 0; clearSlot < accumulationSlotCount_; clearSlot++)
				{
					clearReservoir(clearView, clearSlot);
				}
			}
		}

		/// [EN] The ray pipeline, shader table and spatial reuse are needed on both paths; the denoise and A-Trous pipelines only when this pass does its own denoising.
		/// [JP] レイのパイプライン、シェーダーテーブル、空間的リユースはどちらの経路でも必要。デノイズと A-Trous のパイプラインは、このパスが自分でデノイズするときだけ必要。
		ID3D12StateObject* stateObject = globalIlluminationShader_.GetStateObject();
		ID3D12PipelineState* denoisePipelineState = denoiseShader_.GetPipelineState();
		Bool denoisePipelineMissing = !denoisePipelineState || !denoiseShader_.GetATrousPipelineState(0) || !denoiseShader_.GetATrousPipelineState(1) || !denoiseShader_.GetATrousPipelineState(2);
		Bool spatialReusePipelineMissing = !denoiseShader_.GetSpatialReusePipelineState();
		Bool pipelinesReady = stateObject && shaderTableResource_ && !spatialReusePipelineMissing && (useDlssRayReconstruction_ || !denoisePipelineMissing);

		/// [EN] The pipelines are missing when the GPU lacks DispatchRays support; report it once.
		/// [JP] GPU が DispatchRays に非対応だとパイプラインが無い。1 度だけ報告する。
		if (!pipelinesReady && !stateObjectMissingLogged_)
		{
			SC_LOG_WARNING("GlobalIlluminationRT/GlobalIlluminationDenoise の RTPSO/PSO/シェーダテーブル作成に失敗しています。DXR(DispatchRays)非対応の可能性があります。間接光は常に無し(0)として扱われます。");
			stateObjectMissingLogged_ = true;
		}

		if (!enabled_ || !pipelinesReady)
		{
			/// [EN] Zero this view's reservoir write slot too, so re-enabling the pass later does not resample a reservoir many frames old.
			/// [JP] このビューの reservoir の書き込みスロットも 0 にする。後でパスを有効に戻したとき、何フレームも前の reservoir をリサンプルしないようにする。
			clearReservoir(viewIndex, writeSlot);
			toRead(reservoirResource_[viewIndex][writeSlot], reservoirState_[viewIndex][writeSlot]);

			/// [EN] 0 means no indirect light. Clear what deferred lighting actually reads: the raw radiance with DLSS Ray Reconstruction, this view's write slot otherwise.
			/// [JP] 0 は間接光なしを意味する。ディファードライティングが実際に読むものをクリアする。DLSS Ray Reconstruction の間は生の放射輝度、それ以外はこのビューの書き込みスロット。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			if (useDlssRayReconstruction_)
			{
				toWrite(radianceResource_, radianceState_);

				cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(radianceUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearRawIndex_), radianceResource_.Get(), clearValues, 0, nullptr);

				cmdList->Barrier(radianceResource_.Get(), radianceState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				radianceState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
				return;
			}

			toWrite(accumulatedRadianceResource_[viewIndex][writeSlot], accumulatedRadianceState_[viewIndex][writeSlot]);

			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(accumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot]), clearHeap_.CPUHandle(clearAccumulatedIndex_[viewIndex][writeSlot]), accumulatedRadianceResource_[viewIndex][writeSlot].Get(), clearValues, 0, nullptr);

			cmdList->Barrier(accumulatedRadianceResource_[viewIndex][writeSlot].Get(), accumulatedRadianceState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			accumulatedRadianceState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			return;
		}

		/// [EN] Ray generation reads last frame's reservoir and writes this frame's in the same dispatch, so both move to their states before the rays.
		/// [JP] レイ生成は同じディスパッチの中で前フレームの reservoir を読み、今フレームの reservoir へ書くため、両方をレイの前に所定の状態へ移す。
		toWrite(radianceResource_, radianceState_);
		toRead(reservoirResource_[viewIndex][historySlot_], reservoirState_[viewIndex][historySlot_]);
		toWrite(reservoirResource_[viewIndex][writeSlot], reservoirState_[viewIndex][writeSlot]);

		/// [EN] DispatchRays takes its root arguments from the compute binding point.
		/// [JP] DispatchRays はルート引数をコンピュートのバインドポイントから取る。
		ID3D12DescriptorHeap* heaps[] = { heap };
		/// [EN] Make the bindless heap the active shader-visible heap.
		/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		/// [EN] All passes share one root signature; set it on the compute binding point.
		/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
		cmd->SetComputeRootSignature(globalIlluminationShader_.GetRootSignature());
		/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
		/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
		RootSignature::BindCompute(cmd, addresses);
		/// [EN] Raytracing uses a state object instead of a pipeline state.
		/// [JP] レイトレーシングはパイプラインステートの代わりにステートオブジェクトを使う。
		cmd->SetPipelineState1(stateObject);

		/// [EN] One ray per pixel, with the three shader-table records laid out back to back.
		/// [JP] 1 ピクセルにつきレイ 1 本。シェーダーテーブルの 3 レコードは連続して並ぶ。
		D3D12_GPU_VIRTUAL_ADDRESS tableAddress = shaderTableResource_->GetGPUVirtualAddress();

		/// [EN] Point each shader table at its record and size the launch to the image.
		/// [JP] 各シェーダーテーブルをそのレコードへ向け、起動の大きさを画像に合わせる。
		D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
		dispatchDesc.RayGenerationShaderRecord.StartAddress = tableAddress + shaderTableRecordSize_ * 0;
		dispatchDesc.RayGenerationShaderRecord.SizeInBytes = shaderTableRecordSize_;
		dispatchDesc.MissShaderTable.StartAddress = tableAddress + shaderTableRecordSize_ * 1;
		dispatchDesc.MissShaderTable.SizeInBytes = shaderTableRecordSize_;
		dispatchDesc.MissShaderTable.StrideInBytes = shaderTableRecordSize_;
		dispatchDesc.HitGroupTable.StartAddress = tableAddress + shaderTableRecordSize_ * 2;
		dispatchDesc.HitGroupTable.SizeInBytes = shaderTableRecordSize_;
		dispatchDesc.HitGroupTable.StrideInBytes = shaderTableRecordSize_;
		dispatchDesc.Width = width_;
		dispatchDesc.Height = height_;
		dispatchDesc.Depth = 1;

		/// [EN] Launch the rays.
		/// [JP] レイを起動する。
		cmd->DispatchRays(&dispatchDesc);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] This frame's reservoir becomes readable, both for the spatial reuse below and as next frame's history.
		/// [JP] 今フレームの reservoir を読める状態にする。下の空間的リユースと、次フレームの履歴の両方のため。
		toRead(radianceResource_, radianceState_);
		toRead(reservoirResource_[viewIndex][writeSlot], reservoirState_[viewIndex][writeSlot]);

		/// [EN] One 8x8 thread group per 8x8 pixel tile, rounded up to cover the edges; every compute pass here uses the same grid.
		/// [JP] 8x8 ピクセルのタイルごとに 8x8 のスレッドグループを 1 つ。端まで覆うよう切り上げる。ここのコンピュートパスはすべて同じグリッドを使う。
		Uint32 groupCountX = (width_ + 7) / 8;
		Uint32 groupCountY = (height_ + 7) / 8;

		/// [EN] ReSTIR spatial reuse: combine each pixel's reservoir with its neighbors and rewrite the radiance and its confidence. It runs before either denoiser.
		/// [JP] ReSTIR の空間的リユース。各ピクセルの reservoir を近傍と結合し、放射輝度とその信頼度を書き直す。どちらのデノイザよりも前に走る。
		toWrite(radianceResource_, radianceState_);
		toWrite(confidenceResource_, confidenceState_);

		cmd->SetPipelineState(denoiseShader_.GetSpatialReusePipelineState());
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(radianceResource_, radianceState_);
		toRead(confidenceResource_, confidenceState_);

		if (useDlssRayReconstruction_)
		{
			/// [EN] DLSS Ray Reconstruction does the denoising, so the raw radiance goes straight to deferred lighting.
			/// [JP] デノイズは DLSS Ray Reconstruction が行うため、生の放射輝度をそのままディファードライティングへ渡す。
			cmdList->Barrier(radianceResource_.Get(), radianceState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			radianceState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			return;
		}

		/// [EN] The denoise passes share the root signature bound above, so only the pipeline changes from here on.
		/// [JP] デノイズの各パスは上でバインドしたルートシグネチャを共有するため、ここからはパイプラインだけを差し替える。
		toRead(accumulatedRadianceResource_[viewIndex][historySlot_], accumulatedRadianceState_[viewIndex][historySlot_]);

		/// [EN] Temporal blend with the history into scratch 0.
		/// [JP] 履歴との時間方向のブレンドをスクラッチ 0 へ書く。
		toWrite(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);

		cmd->SetPipelineState(denoisePipelineState);
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] A-Trous iteration 1 (step 1): scratch 0 into scratch 1.
		/// [JP] A-Trous の 1 反復目（ステップ 1）。スクラッチ 0 からスクラッチ 1 へ。
		toRead(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);
		toWrite(atrousScratchResource_[viewIndex][1], atrousScratchState_[viewIndex][1]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(0));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] A-Trous iteration 2 (step 2): scratch 1 back into scratch 0.
		/// [JP] A-Trous の 2 反復目（ステップ 2）。スクラッチ 1 からスクラッチ 0 へ戻す。
		toRead(atrousScratchResource_[viewIndex][1], atrousScratchState_[viewIndex][1]);
		toWrite(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(1));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] A-Trous iteration 3 (step 4): scratch 0 into this frame's accumulation slot, which is both what deferred lighting reads now and next frame's history.
		/// [JP] A-Trous の 3 反復目（ステップ 4）。スクラッチ 0 から今フレームの蓄積スロットへ。これが今ディファードライティングが読む値であり、次フレームの履歴にもなる。
		toRead(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);
		toWrite(accumulatedRadianceResource_[viewIndex][writeSlot], accumulatedRadianceState_[viewIndex][writeSlot]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(2));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] Hand the denoised result over to deferred lighting as a shader resource.
		/// [JP] デノイズした結果をシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(accumulatedRadianceResource_[viewIndex][writeSlot].Get(), accumulatedRadianceState_[viewIndex][writeSlot], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		accumulatedRadianceState_[viewIndex][writeSlot] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the raw radiance and confidence textures and, per view, the
	* accumulation textures, reservoirs and A-Trous scratch pair at
	* width_ x height_, and marks the new reservoirs for zeroing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の生の放射輝度と信頼度のテクスチャ、ビューごとの蓄積
	* テクスチャ、reservoir、A-Trous のスクラッチ 2 枚を作成し、新しい
	* reservoir を 0 埋めの対象にする。
	*/
	void GlobalIlluminationRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] Clear views: the raw radiance, the confidence, and one per accumulation texture and per reservoir.
		/// [JP] クリア用ビューは、生の放射輝度、信頼度、蓄積テクスチャごと、reservoir ごとに 1 つずつ。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2 + viewCount_ * accumulationSlotCount_ * 2, false);

		reservoirCleared_ = false;

		/// [EN] Creates one width_ x height_ texture of format with bindless write and read views, plus a clear view when clearIndex is given.
		/// [JP] bindless の書き込み用・読み取り用ビューと、clearIndex があればクリア用ビューを持つ、format の width_ x height_ テクスチャを 1 枚作る。
		auto createTexture = [this, device](DXGI_FORMAT format, const Wchar* name, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& unorderedAccessViewIndex, Uint32& shaderResourceViewIndex, Uint32* clearIndex)
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
			resourceDesc.Format = format;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			/// [EN] Create the texture in its own GPU-local heap, starting in COMMON state.
			/// [JP] テクスチャを専用の GPU ローカルなヒープに、COMMON 状態で作る。
			HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
			SC_HR_CHECK(hr, "GI テクスチャの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(name);
			GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

			/// [EN] The write view sees the whole resource in its own format.
			/// [JP] 書き込み用ビューは、リソース全体を自身のフォーマットで見る。
			D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
			unorderedAccessViewDesc.Format = format;
			unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			/// [EN] Reserve a bindless slot and write the write view into it.
			/// [JP] bindless のスロットを確保し、書き込み用ビューを入れる。
			unorderedAccessViewIndex = bindlessHeap_->AllocateIndex();
			device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(unorderedAccessViewIndex));

			if (clearIndex)
			{
				/// [EN] A second copy of the write view in the CPU-side heap, for clears.
				/// [JP] クリア用に、書き込み用ビューの 2 つ目を CPU 側のヒープに作る。
				*clearIndex = clearHeap_.AllocateIndex();
				device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(*clearIndex));
			}

			/// [EN] Reserve a bindless slot for the read view.
			/// [JP] 読み取り用ビューのための bindless のスロットを確保する。
			shaderResourceViewIndex = bindlessHeap_->AllocateIndex();
			/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
			/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = format;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(shaderResourceViewIndex));
		};

		/// [EN] Radiance is HDR RGB plus a validity flag; confidence is a single 0-1 channel.
		/// [JP] 放射輝度は HDR の RGB と有効フラグ、信頼度は 0 から 1 の 1 チャンネル。
		createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, L"GlobalIllumination_Radiance", radianceResource_, radianceUnorderedAccessViewIndex_, radianceShaderResourceViewIndex_, &clearRawIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		radianceState_ = D3D12_RESOURCE_STATE_COMMON;

		createTexture(DXGI_FORMAT_R16_FLOAT, L"GlobalIllumination_Confidence", confidenceResource_, confidenceUnorderedAccessViewIndex_, confidenceShaderResourceViewIndex_, &clearConfidenceIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		confidenceState_ = D3D12_RESOURCE_STATE_COMMON;

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			/// [EN] Each slot pairs an accumulation texture with a screen-sized reservoir buffer.
			/// [JP] 各スロットは、蓄積テクスチャと画面サイズの reservoir バッファの組。
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, L"GlobalIllumination_Accumulated", accumulatedRadianceResource_[viewIndex][slotIndex], accumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], accumulatedShaderResourceViewIndex_[viewIndex][slotIndex], &clearAccumulatedIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				accumulatedRadianceState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				ReservoirBuffer::Create(device, bindlessHeap_, clearHeap_, width_ * height_, reservoirElementSizeInBytes_, reservoirResource_[viewIndex][slotIndex], reservoirUnorderedAccessViewIndex_[viewIndex][slotIndex], reservoirShaderResourceViewIndex_[viewIndex][slotIndex], clearReservoirIndex_[viewIndex][slotIndex], clearReservoirGpuIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				reservoirState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}

			/// [EN] The A-Trous scratch pair is always fully overwritten, so it needs no clear view.
			/// [JP] A-Trous のスクラッチは必ず全画素上書きされるため、クリア用ビューは要らない。
			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, L"GlobalIllumination_AtrousScratch", atrousScratchResource_[viewIndex][slotIndex], atrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], atrousScratchShaderResourceViewIndex_[viewIndex][slotIndex], nullptr);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				atrousScratchState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}
		}
	}

	/**
	* [EN]
	* Frees every texture's and reservoir's bindless views and hands them to
	* the heap's deferred release, since commands recorded in earlier frames
	* may still use them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべてのテクスチャと reservoir の bindless ビューを解放し、それらは
	* ヒープの遅延解放へ渡す。以前のフレームで記録したコマンドがまだ使って
	* いる可能性があるため。
	*/
	void GlobalIlluminationRenderer::Release()
	{
		/// [EN] Frees one resource's two bindless views and defers its destruction.
		/// [JP] リソース 1 つの bindless ビュー 2 つを解放し、破棄を遅延させる。
		auto releaseResource = [this](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32 unorderedAccessViewIndex, Uint32 shaderResourceViewIndex)
		{
			/// [EN] Return both view slots to the bindless heap.
			/// [JP] 両方のビューのスロットを bindless ヒープへ返す。
			bindlessHeap_->FreeIndex(unorderedAccessViewIndex);
			bindlessHeap_->FreeIndex(shaderResourceViewIndex);
			/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
			/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
			bindlessHeap_->DeferRelease(resource);
			resource.Reset();
		};

		releaseResource(radianceResource_, radianceUnorderedAccessViewIndex_, radianceShaderResourceViewIndex_);
		releaseResource(confidenceResource_, confidenceUnorderedAccessViewIndex_, confidenceShaderResourceViewIndex_);

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				releaseResource(accumulatedRadianceResource_[viewIndex][slotIndex], accumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], accumulatedShaderResourceViewIndex_[viewIndex][slotIndex]);

				/// [EN] A reservoir also owns the shader-visible raw view used by its clear.
				/// [JP] reservoir は、クリアに使うシェーダー可視の raw ビューも持つ。
				bindlessHeap_->FreeIndex(clearReservoirGpuIndex_[viewIndex][slotIndex]);
				releaseResource(reservoirResource_[viewIndex][slotIndex], reservoirUnorderedAccessViewIndex_[viewIndex][slotIndex], reservoirShaderResourceViewIndex_[viewIndex][slotIndex]);
			}

			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				releaseResource(atrousScratchResource_[viewIndex][slotIndex], atrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], atrousScratchShaderResourceViewIndex_[viewIndex][slotIndex]);
			}
		}
	}
}

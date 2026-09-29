#include <GraphicsEngine/Renderer/ReflectionRenderer.h>

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
	ReflectionRenderer::ReflectionRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject, PipelineStateObject& pipelineStateObject) : reflectionShader_(rootSignature, raytracingStateObject), denoiseShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once - pipelines, tuning buffer,
	* instance table and shader table - then allocates the size-dependent
	* textures and reservoirs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクト（パイプライン、調整値のバッファ、
	* インスタンステーブル、シェーダーテーブル）を 1 度だけ作成し、その後
	* サイズに依存するテクスチャと reservoir を確保する。
	*/
	void ReflectionRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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
		reflectionShader_.Create(shaderCache, device5);
		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		denoiseShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<ReflectionRayConstantBuffer>>(device, bindlessHeap);
		instanceTable_ = MakePtr<ReadOnlyStructuredBuffer<ReflectionInstanceData>>(device, bindlessHeap, maxInstances_);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);

		/// [EN] Build the shader table. With only a global root signature, each record is just the 32-byte shader identifier taken from the state object.
		/// [JP] シェーダーテーブルを構築する。グローバルルートシグネチャだけなので、各レコードはステートオブジェクトから得る 32 バイトのシェーダー識別子のみ。
		ID3D12StateObject* stateObject = reflectionShader_.GetStateObject();
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

		void* rayGenIdentifier = stateObjectProperties->GetShaderIdentifier(String(ReflectionShader::rayGenExportName).w_str().c_str());
		void* missIdentifier = stateObjectProperties->GetShaderIdentifier(String(ReflectionShader::missExportName).w_str().c_str());
		void* hitGroupIdentifier = stateObjectProperties->GetShaderIdentifier(String(ReflectionShader::hitGroupName).w_str().c_str());
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
		SC_HR_CHECK(hr, "シェーダーテーブルリソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		shaderTableResource_->SetName(L"Reflection_ShaderTable");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(shaderTableResource_.Get());
#endif

		/// [EN] Ray generation, miss and hit group in record order, each followed by zeros up to the record size.
		/// [JP] レイ生成、ミス、ヒットグループをレコード順に並べ、それぞれレコードサイズまで 0 で埋める。
		Uint8* mapped = nullptr;
		hr = shaderTableResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		SC_HR_CHECK(hr, "シェーダーテーブルリソースのMapに失敗しました");
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
	void ReflectionRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	* Uploads up to maxInstances_ entries of the instance table; any extra
	* instances are dropped.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* インスタンステーブルを最大 maxInstances_ 要素までアップロードする。
	* それを超えるインスタンスは捨てる。
	*/
	void ReflectionRenderer::UpdateInstanceTable(const ReflectionInstanceData* data, Uint32 count)
	{
		instanceTable_->Update(data, count < maxInstances_ ? count : maxInstances_);
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
	void ReflectionRenderer::Prepare(const ReflectionRayConstantBuffer& settings, Bool enabled)
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
		ReflectionRayConstantBuffer uploadSettings = settings;
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
		constantIndicesSystem_->SetReflectionRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetReflectionOutputUnorderedAccessViewIndex(radianceUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetReflectionOutputShaderResourceViewIndex(radianceShaderResourceViewIndex_);
		unorderedAccessIndicesSystem_->SetReflectionConfidenceUnorderedAccessViewIndex(confidenceUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetReflectionConfidenceShaderResourceViewIndex(confidenceShaderResourceViewIndex_);

		/// [EN] The instance table is frame-ring buffered, so its read index changes every frame and has to be published every frame.
		/// [JP] インスタンステーブルはフレームリングのバッファなので読み取りインデックスが毎フレーム変わり、毎フレーム公開し直す必要がある。
		shaderResourceIndicesSystem_->SetReflectionInstanceDataIndex(instanceTable_->Index());

		/// [EN] Array indices of the editor and game views.
		/// [JP] エディタービューとゲームビューの配列インデックス。
		constexpr Uint32 editorView = static_cast<Uint32>(RaytracingView::Editor);
		constexpr Uint32 gameView = static_cast<Uint32>(RaytracingView::Game);

		/// [EN] Every buffer that carries state across frames reads the history slot and writes the other one. One slot pair drives the radiance, moments, history length and depth-normal copy together, so the geometry test and the blended radiance always come from the same frame.
		/// [JP] フレームをまたぐ状態を持つバッファは、すべて履歴のスロットを読んでもう一方へ書く。1 組のスロットが放射輝度、モーメント、履歴長、深度法線コピーをまとめて動かすため、幾何の判定とブレンドする放射輝度は常に同じフレームのものになる。
		auto buildShaderResourceIndices = [&](Uint32 viewIndex)
		{
			ReflectionAccumulationShaderResourceIndices values{};

			values.historyIndex_ = accumulatedShaderResourceViewIndex_[viewIndex][historySlot_];
			values.accumulatedIndex_ = accumulatedShaderResourceViewIndex_[viewIndex][writeSlot];

			/// [EN] With DLSS Ray Reconstruction, deferred lighting reads the raw radiance and the SVGF chain is left alone.
			/// [JP] DLSS Ray Reconstruction の間は、ディファードライティングが生の放射輝度を読み、SVGF チェーンには触れない。
			values.radianceIndex_ = useDlssRayReconstruction_ ? radianceShaderResourceViewIndex_ : denoisedShaderResourceViewIndex_[viewIndex];

			values.atrousScratch0Index_ = atrousScratchShaderResourceViewIndex_[viewIndex][0];
			values.atrousScratch1Index_ = atrousScratchShaderResourceViewIndex_[viewIndex][1];

			values.momentsHistoryIndex_ = momentsShaderResourceViewIndex_[viewIndex][historySlot_];
			values.momentsIndex_ = momentsShaderResourceViewIndex_[viewIndex][writeSlot];

			values.historyLengthHistoryIndex_ = historyLengthShaderResourceViewIndex_[viewIndex][historySlot_];
			values.historyLengthIndex_ = historyLengthShaderResourceViewIndex_[viewIndex][writeSlot];

			values.depthNormalHistoryIndex_ = depthNormalShaderResourceViewIndex_[viewIndex][historySlot_];
			values.depthNormalIndex_ = depthNormalShaderResourceViewIndex_[viewIndex][writeSlot];

			/// [EN] The reservoir is used on both denoise paths and follows the same slot pair.
			/// [JP] reservoir はどちらのデノイズ経路でも使い、同じスロットの組に従う。
			values.reservoirHistoryIndex_ = reservoirShaderResourceViewIndex_[viewIndex][historySlot_];
			values.reservoirWriteIndex_ = reservoirShaderResourceViewIndex_[viewIndex][writeSlot];

			return values;
		};

		/// [EN] The write targets of every pass for one view.
		/// [JP] 1 つのビューの、各パスの書き込み先。
		auto buildUnorderedAccessIndices = [&](Uint32 viewIndex)
		{
			ReflectionAccumulationUnorderedAccessIndices values{};

			values.accumulatedIndex_ = accumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.atrousScratch0Index_ = atrousScratchUnorderedAccessViewIndex_[viewIndex][0];
			values.atrousScratch1Index_ = atrousScratchUnorderedAccessViewIndex_[viewIndex][1];
			values.momentsIndex_ = momentsUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.historyLengthIndex_ = historyLengthUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.depthNormalIndex_ = depthNormalUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.denoisedIndex_ = denoisedUnorderedAccessViewIndex_[viewIndex];
			values.reservoirIndex_ = reservoirUnorderedAccessViewIndex_[viewIndex][writeSlot];

			return values;
		};

		shaderResourceIndicesSystem_->SetEditorReflectionAccumulationIndices(buildShaderResourceIndices(editorView));
		shaderResourceIndicesSystem_->SetGameReflectionAccumulationIndices(buildShaderResourceIndices(gameView));
		unorderedAccessIndicesSystem_->SetEditorReflectionAccumulationIndices(buildUnorderedAccessIndices(editorView));
		unorderedAccessIndicesSystem_->SetGameReflectionAccumulationIndices(buildUnorderedAccessIndices(gameView));
	}

	/**
	* [EN]
	* Writes view's reflections: traced, spatially reused and (unless DLSS
	* Ray Reconstruction is on) run through SVGF when enabled_ and every
	* needed pipeline exist, otherwise the texture deferred lighting reads,
	* the history length and this frame's reservoir are cleared.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* view の反射を書き込む。enabled_ で必要なパイプラインがすべてあれば
	* トレースし、空間的リユースを行い、DLSS Ray Reconstruction が無効なら
	* SVGF も通す。そうでなければ、ディファードライティングが読むテクスチャ、
	* 履歴長、今フレームの reservoir をクリアする。
	*/
	void ReflectionRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view)
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

		/// [EN] Clears one float texture to zero through its bindless and clear-heap write views.
		/// [JP] bindless とクリア用ヒープの書き込みビューを使って、浮動小数点のテクスチャを 1 枚 0 でクリアする。
		auto clearTexture = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state, Uint32 unorderedAccessViewIndex, Uint32 clearIndex)
		{
			toWrite(resource, state);

			const Float zeroValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(unorderedAccessViewIndex), clearHeap_.CPUHandle(clearIndex), resource.Get(), zeroValues, 0, nullptr);
		};

		/// [EN] Clears one reservoir to zero through its shader-visible and clear-heap write views.
		/// [JP] シェーダー可視とクリア用ヒープの書き込みビューを使って、reservoir を 1 つ 0 でクリアする。
		auto clearReservoir = [&](Uint32 clearView, Uint32 clearSlot)
		{
			toWrite(reservoirResource_[clearView][clearSlot], reservoirState_[clearView][clearSlot]);

			const Uint32 zeroValues[4] = { 0, 0, 0, 0 };
			cmd->ClearUnorderedAccessViewUint(bindlessHeap_->GPUHandle(clearReservoirGpuIndex_[clearView][clearSlot]), clearHeap_.CPUHandle(clearReservoirIndex_[clearView][clearSlot]), reservoirResource_[clearView][clearSlot].Get(), zeroValues, 0, nullptr);
		};

		/// [EN] Zero the whole history chain, both reservoir slots and the denoised output of every view once after allocation, so no uninitialized data is ever read back as history.
		/// [JP] 確保の後に 1 度だけ、全ビューの履歴チェーン、reservoir の両スロット、デノイズ済み出力を 0 で埋める。未初期化のデータが履歴として読み戻されないようにする。
		if (!historyCleared_)
		{
			/// [EN] Mark it done, so this happens only once.
			/// [JP] 済んだ印を付け、1 度だけ行うようにする。
			historyCleared_ = true;

			ID3D12DescriptorHeap* clearHeaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(clearHeaps), clearHeaps);

			for (Uint32 clearView = 0; clearView < viewCount_; clearView++)
			{
				for (Uint32 clearSlot = 0; clearSlot < accumulationSlotCount_; clearSlot++)
				{
					clearTexture(accumulatedRadianceResource_[clearView][clearSlot], accumulatedRadianceState_[clearView][clearSlot], accumulatedUnorderedAccessViewIndex_[clearView][clearSlot], clearAccumulatedIndex_[clearView][clearSlot]);
					clearTexture(momentsResource_[clearView][clearSlot], momentsState_[clearView][clearSlot], momentsUnorderedAccessViewIndex_[clearView][clearSlot], clearMomentsIndex_[clearView][clearSlot]);
					clearTexture(historyLengthResource_[clearView][clearSlot], historyLengthState_[clearView][clearSlot], historyLengthUnorderedAccessViewIndex_[clearView][clearSlot], clearHistoryLengthIndex_[clearView][clearSlot]);
					clearTexture(depthNormalResource_[clearView][clearSlot], depthNormalState_[clearView][clearSlot], depthNormalUnorderedAccessViewIndex_[clearView][clearSlot], clearDepthNormalIndex_[clearView][clearSlot]);
					clearReservoir(clearView, clearSlot);
				}

				clearTexture(denoisedResource_[clearView], denoisedState_[clearView], denoisedUnorderedAccessViewIndex_[clearView], clearDenoisedIndex_[clearView]);
			}
		}

		/// [EN] The ray pipeline, shader table and spatial reuse are needed on both paths; the SVGF pipelines only when this pass does its own denoising.
		/// [JP] レイのパイプライン、シェーダーテーブル、空間的リユースはどちらの経路でも必要。SVGF のパイプラインは、このパスが自分でデノイズするときだけ必要。
		ID3D12StateObject* stateObject = reflectionShader_.GetStateObject();
		ID3D12PipelineState* denoisePipelineState = denoiseShader_.GetPipelineState();
		Bool denoisePipelineMissing = !denoisePipelineState || !denoiseShader_.GetFilterMomentsPipelineState() || !denoiseShader_.GetATrousPipelineState(0) || !denoiseShader_.GetATrousPipelineState(1) || !denoiseShader_.GetATrousPipelineState(2);
		Bool spatialReusePipelineMissing = !denoiseShader_.GetSpatialReusePipelineState();
		Bool pipelinesReady = stateObject && shaderTableResource_ && !spatialReusePipelineMissing && (useDlssRayReconstruction_ || !denoisePipelineMissing);

		/// [EN] The pipelines are missing when the GPU lacks DispatchRays support; report it once.
		/// [JP] GPU が DispatchRays に非対応だとパイプラインが無い。1 度だけ報告する。
		if (!pipelinesReady && !stateObjectMissingLogged_)
		{
			SC_LOG_WARNING("ReflectionRT/ReflectionDenoise の RTPSO/PSO/シェーダテーブル作成に失敗しています。DXR(DispatchRays)非対応の可能性があります。反射は常に無し(0)として扱われます。");
			stateObjectMissingLogged_ = true;
		}

		if (!enabled_ || !pipelinesReady)
		{
			/// [EN] Zero this view's reservoir write slot too, so re-enabling the pass later does not resample a reservoir many frames old.
			/// [JP] このビューの reservoir の書き込みスロットも 0 にする。後でパスを有効に戻したとき、何フレームも前の reservoir をリサンプルしないようにする。
			clearReservoir(viewIndex, writeSlot);
			toRead(reservoirResource_[viewIndex][writeSlot], reservoirState_[viewIndex][writeSlot]);

			/// [EN] 0 means no reflection. Clear what deferred lighting actually reads: the raw radiance with DLSS Ray Reconstruction, this view's denoised output otherwise.
			/// [JP] 0 は反射なしを意味する。ディファードライティングが実際に読むものをクリアする。DLSS Ray Reconstruction の間は生の放射輝度、それ以外はこのビューのデノイズ済み出力。
			if (useDlssRayReconstruction_)
			{
				clearTexture(radianceResource_, radianceState_, radianceUnorderedAccessViewIndex_, clearRawIndex_);

				cmdList->Barrier(radianceResource_.Get(), radianceState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				radianceState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
				return;
			}

			clearTexture(denoisedResource_[viewIndex], denoisedState_[viewIndex], denoisedUnorderedAccessViewIndex_[viewIndex], clearDenoisedIndex_[viewIndex]);

			cmdList->Barrier(denoisedResource_[viewIndex].Get(), denoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			denoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

			/// [EN] Reset the history length to 0, so the next traced frame starts converging afresh instead of trusting a history that holds no real signal.
			/// [JP] 履歴長を 0 に戻す。次にトレースしたフレームが、実際の信号を持たない履歴を信用せず、最初から収束し直すようにする。
			clearTexture(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot], historyLengthUnorderedAccessViewIndex_[viewIndex][writeSlot], clearHistoryLengthIndex_[viewIndex][writeSlot]);
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
		cmd->SetComputeRootSignature(reflectionShader_.GetRootSignature());
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

		/// [EN] The reprojection reads the history side of every carried buffer. The SVGF passes share the root signature bound above, so only the pipeline changes from here on.
		/// [JP] リプロジェクションは、引き継ぐすべてのバッファの履歴側を読む。SVGF の各パスは上でバインドしたルートシグネチャを共有するため、ここからはパイプラインだけを差し替える。
		toRead(accumulatedRadianceResource_[viewIndex][historySlot_], accumulatedRadianceState_[viewIndex][historySlot_]);
		toRead(momentsResource_[viewIndex][historySlot_], momentsState_[viewIndex][historySlot_]);
		toRead(historyLengthResource_[viewIndex][historySlot_], historyLengthState_[viewIndex][historySlot_]);
		toRead(depthNormalResource_[viewIndex][historySlot_], depthNormalState_[viewIndex][historySlot_]);

		/// [EN] Pass 1, reprojection: raw and history into scratch 0, plus this frame's moments, history length and depth-normal copy.
		/// [JP] パス 1、リプロジェクション。生と履歴から、スクラッチ 0 と、今フレームのモーメント、履歴長、深度法線コピーを作る。
		toWrite(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);
		toWrite(momentsResource_[viewIndex][writeSlot], momentsState_[viewIndex][writeSlot]);
		toWrite(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot]);
		toWrite(depthNormalResource_[viewIndex][writeSlot], depthNormalState_[viewIndex][writeSlot]);

		cmd->SetPipelineState(denoisePipelineState);
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] The later passes only read the moments, history length and depth-normal copy, so they move to read state once here and stay there.
		/// [JP] 以降のパスはモーメント、履歴長、深度法線コピーを読むだけなので、ここで 1 度だけ読み取り状態へ移してそのまま保つ。
		toRead(momentsResource_[viewIndex][writeSlot], momentsState_[viewIndex][writeSlot]);
		toRead(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot]);
		toRead(depthNormalResource_[viewIndex][writeSlot], depthNormalState_[viewIndex][writeSlot]);
		toRead(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);

		/// [EN] Pass 2, FilterMoments: scratch 0 into scratch 1.
		/// [JP] パス 2、FilterMoments。スクラッチ 0 からスクラッチ 1 へ。
		toWrite(atrousScratchResource_[viewIndex][1], atrousScratchState_[viewIndex][1]);

		cmd->SetPipelineState(denoiseShader_.GetFilterMomentsPipelineState());
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(atrousScratchResource_[viewIndex][1], atrousScratchState_[viewIndex][1]);

		/// [EN] Pass 3, A-Trous step 1: scratch 1 back into scratch 0.
		/// [JP] パス 3、A-Trous ステップ 1。スクラッチ 1 からスクラッチ 0 へ戻す。
		toWrite(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(0));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(atrousScratchResource_[viewIndex][0], atrousScratchState_[viewIndex][0]);

		/// [EN] Pass 4, A-Trous step 2, the feedback tap: scratch 0 into the history write slot, which becomes next frame's history.
		/// [JP] パス 4、A-Trous ステップ 2（フィードバックタップ）。スクラッチ 0 から履歴の書き込みスロットへ。これが次フレームの履歴になる。
		toWrite(accumulatedRadianceResource_[viewIndex][writeSlot], accumulatedRadianceState_[viewIndex][writeSlot]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(1));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(accumulatedRadianceResource_[viewIndex][writeSlot], accumulatedRadianceState_[viewIndex][writeSlot]);

		/// [EN] Pass 5, A-Trous step 4: the history write slot into the view's denoised output.
		/// [JP] パス 5、A-Trous ステップ 4。履歴の書き込みスロットからビューのデノイズ済み出力へ。
		toWrite(denoisedResource_[viewIndex], denoisedState_[viewIndex]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(2));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] Hand the denoised output over to deferred lighting as a shader resource.
		/// [JP] デノイズ済みの出力をシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(denoisedResource_[viewIndex].Get(), denoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		denoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		/// [EN] The raw radiance also ends readable by pixel shaders: the SVGF chain only needs non-pixel read state, but the raw-reflection debug view samples it from DeferredLightingPS.
		/// [JP] 生の放射輝度もピクセルシェーダーから読める状態で終える。SVGF チェーンには非ピクセルの読み取り状態で足りるが、生の反射のデバッグ表示は DeferredLightingPS からサンプルするため。
		cmdList->Barrier(radianceResource_.Get(), radianceState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		radianceState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the raw radiance and confidence textures and, per view, the
	* whole SVGF chain and the reservoirs at width_ x height_, and marks the
	* new history chain for zeroing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の生の放射輝度と信頼度のテクスチャ、ビューごとの SVGF
	* チェーン一式と reservoir を作成し、新しい履歴チェーンを 0 埋めの対象に
	* する。
	*/
	void ReflectionRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] Clear views: the raw radiance, the confidence, each view's denoised output, four history buffers per view and slot, and one per reservoir.
		/// [JP] クリア用ビューは、生の放射輝度、信頼度、ビューごとのデノイズ済み出力、ビューとスロットごとに履歴のバッファ 4 つ、reservoir ごとに 1 つ。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 + 1 + viewCount_ + viewCount_ * accumulationSlotCount_ * 4 + viewCount_ * accumulationSlotCount_, false);

		historyCleared_ = false;

		/// [EN] Creates one width_ x height_ texture of format with bindless write and read views, plus a clear view when clearIndex is given. Every buffer of the chain differs only in format.
		/// [JP] bindless の書き込み用・読み取り用ビューと、clearIndex があればクリア用ビューを持つ、format の width_ x height_ テクスチャを 1 枚作る。チェーンの各バッファはフォーマットが違うだけ。
		auto createTexture = [this, device](DXGI_FORMAT format, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& unorderedAccessViewIndex, Uint32& shaderResourceViewIndex, Uint32* clearIndex)
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
			SC_HR_CHECK(hr, "反射デノイズ用テクスチャの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(L"Reflection_Denoise");
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

		/// [EN] Radiance is HDR RGB plus the hit distance; confidence is a single 0-1 channel.
		/// [JP] 放射輝度は HDR の RGB とヒット距離、信頼度は 0 から 1 の 1 チャンネル。
		createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, radianceResource_, radianceUnorderedAccessViewIndex_, radianceShaderResourceViewIndex_, &clearRawIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		radianceState_ = D3D12_RESOURCE_STATE_COMMON;

		createTexture(DXGI_FORMAT_R16_FLOAT, confidenceResource_, confidenceUnorderedAccessViewIndex_, confidenceShaderResourceViewIndex_, &clearConfidenceIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		confidenceState_ = D3D12_RESOURCE_STATE_COMMON;

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			/// [EN] The history chain and the reservoirs, two slots each.
			/// [JP] 履歴チェーンと reservoir。それぞれ 2 スロット。
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, accumulatedRadianceResource_[viewIndex][slotIndex], accumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], accumulatedShaderResourceViewIndex_[viewIndex][slotIndex], &clearAccumulatedIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				accumulatedRadianceState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				ReservoirBuffer::Create(device, bindlessHeap_, clearHeap_, width_ * height_, reservoirElementSizeInBytes_, reservoirResource_[viewIndex][slotIndex], reservoirUnorderedAccessViewIndex_[viewIndex][slotIndex], reservoirShaderResourceViewIndex_[viewIndex][slotIndex], clearReservoirIndex_[viewIndex][slotIndex], clearReservoirGpuIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				reservoirState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16G16_FLOAT, momentsResource_[viewIndex][slotIndex], momentsUnorderedAccessViewIndex_[viewIndex][slotIndex], momentsShaderResourceViewIndex_[viewIndex][slotIndex], &clearMomentsIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				momentsState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16_FLOAT, historyLengthResource_[viewIndex][slotIndex], historyLengthUnorderedAccessViewIndex_[viewIndex][slotIndex], historyLengthShaderResourceViewIndex_[viewIndex][slotIndex], &clearHistoryLengthIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				historyLengthState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				/// [EN] 32-bit, because the depth test measures differences in units of the depth derivative and FP16 depth is too coarse for that.
				/// [JP] 深度の判定は深度の勾配を単位として差を測り、FP16 の深度ではそれに対して粗すぎるため、32 ビットにする。
				createTexture(DXGI_FORMAT_R32G32B32A32_FLOAT, depthNormalResource_[viewIndex][slotIndex], depthNormalUnorderedAccessViewIndex_[viewIndex][slotIndex], depthNormalShaderResourceViewIndex_[viewIndex][slotIndex], &clearDepthNormalIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				depthNormalState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}

			/// [EN] The A-Trous scratch pair is always fully overwritten, so it needs no clear view.
			/// [JP] A-Trous のスクラッチは必ず全画素上書きされるため、クリア用ビューは要らない。
			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, atrousScratchResource_[viewIndex][slotIndex], atrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], atrousScratchShaderResourceViewIndex_[viewIndex][slotIndex], nullptr);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				atrousScratchState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}

			/// [EN] The denoised output deferred lighting samples.
			/// [JP] ディファードライティングがサンプルするデノイズ済みの出力。
			createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, denoisedResource_[viewIndex], denoisedUnorderedAccessViewIndex_[viewIndex], denoisedShaderResourceViewIndex_[viewIndex], &clearDenoisedIndex_[viewIndex]);
			/// [EN] Track the state it was created in, so the first barrier starts from the right state.
			/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
			denoisedState_[viewIndex] = D3D12_RESOURCE_STATE_COMMON;
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
	void ReflectionRenderer::Release()
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

				releaseResource(momentsResource_[viewIndex][slotIndex], momentsUnorderedAccessViewIndex_[viewIndex][slotIndex], momentsShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseResource(historyLengthResource_[viewIndex][slotIndex], historyLengthUnorderedAccessViewIndex_[viewIndex][slotIndex], historyLengthShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseResource(depthNormalResource_[viewIndex][slotIndex], depthNormalUnorderedAccessViewIndex_[viewIndex][slotIndex], depthNormalShaderResourceViewIndex_[viewIndex][slotIndex]);
			}

			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				releaseResource(atrousScratchResource_[viewIndex][slotIndex], atrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], atrousScratchShaderResourceViewIndex_[viewIndex][slotIndex]);
			}

			releaseResource(denoisedResource_[viewIndex], denoisedUnorderedAccessViewIndex_[viewIndex], denoisedShaderResourceViewIndex_[viewIndex]);
		}
	}
}

#include <GraphicsEngine/Renderer/ShadowRenderer.h>

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
	* Binds the shared root signature and pipeline-state cache to the shadow
	* and denoise shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、影と
	* デノイズのシェーダーへ関連付ける。
	*/
	ShadowRenderer::ShadowRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : shadowShader_(rootSignature, pipelineStateObject), denoiseShader_(rootSignature, pipelineStateObject)
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
	void ShadowRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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
		shadowShader_.Create(shaderCache, device);
		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		denoiseShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<ShadowRayConstantBuffer>>(device, bindlessHeap);

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
	void ShadowRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	void ShadowRenderer::Prepare(const ShadowRayConstantBuffer& settings, Bool enabled)
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

		/// [EN] The upload copy carries the frame counter and denoise mode on top of the user's tuning.
		/// [JP] アップロード用のコピーには、ユーザーの調整値に加えてフレームカウンターとデノイズモードを載せる。
		ShadowRayConstantBuffer uploadSettings = settings;
		uploadSettings.frameIndex_ = frameIndex_;
		uploadSettings.denoiseMode_ = static_cast<Uint32>(useDlssRayReconstruction_ ? ShadowDenoiseMode::DlssRR : ShadowDenoiseMode::Temporal);
		/// [EN] Advance the counter for the next frame.
		/// [JP] 次のフレームのためにカウンターを進める。
		frameIndex_++;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);
		constantIndicesSystem_->SetShadowRayConstantIndex(tuningBuffer_->GetIndex());

		/// [EN] The trace always writes the raw texture.
		/// [JP] トレースは常に生のテクスチャへ書き込む。
		unorderedAccessIndicesSystem_->SetShadowRawVisibilityUnorderedAccessViewIndex(rawVisibilityUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetShadowRawVisibilityShaderResourceViewIndex(rawVisibilityShaderResourceViewIndex_);

		/// [EN] Array indices of the editor and game views.
		/// [JP] エディタービューとゲームビューの配列インデックス。
		constexpr Uint32 editorView = static_cast<Uint32>(RaytracingView::Editor);
		constexpr Uint32 gameView = static_cast<Uint32>(RaytracingView::Game);

		/// [EN] Every buffer that carries state across frames reads the history slot and writes the other one. One slot pair drives both signals and the shared history length and depth-normal copy together, so the geometry test and the blended signal always come from the same frame.
		/// [JP] フレームをまたぐ状態を持つバッファは、すべて履歴のスロットを読んでもう一方へ書く。1 組のスロットが両信号と共有の履歴長・深度法線コピーをまとめて動かすため、幾何の判定とブレンドする信号は常に同じフレームのものになる。
		auto buildShaderResourceIndices = [&](Uint32 viewIndex)
		{
			ShadowAccumulationShaderResourceIndices values{};

			values.directionalHistoryIndex_ = directionalAccumulatedShaderResourceViewIndex_[viewIndex][historySlot_];
			values.directionalAccumulatedIndex_ = directionalAccumulatedShaderResourceViewIndex_[viewIndex][writeSlot];

			values.punctualHistoryIndex_ = punctualAccumulatedShaderResourceViewIndex_[viewIndex][historySlot_];
			values.punctualAccumulatedIndex_ = punctualAccumulatedShaderResourceViewIndex_[viewIndex][writeSlot];

			/// [EN] With DLSS Ray Reconstruction, deferred lighting reads the raw texture: the plain view for the directional visibility (.r) and the shifted view for the punctual radiance (.rgb holds raw gba).
			/// [JP] DLSS Ray Reconstruction の間は、ディファードライティングが生のテクスチャを読む。ディレクショナルの可視性（.r）は通常のビューで、パンクチュアルの放射輝度はずらしたビュー（.rgb に生の gba が入る）で読む。
			values.directionalVisibilityIndex_ = useDlssRayReconstruction_ ? rawVisibilityShaderResourceViewIndex_ : directionalDenoisedShaderResourceViewIndex_[viewIndex];
			values.punctualRadianceIndex_ = useDlssRayReconstruction_ ? rawPunctualShaderResourceViewIndex_ : punctualDenoisedShaderResourceViewIndex_[viewIndex];

			values.directionalAtrousScratch0Index_ = directionalAtrousScratchShaderResourceViewIndex_[viewIndex][0];
			values.directionalAtrousScratch1Index_ = directionalAtrousScratchShaderResourceViewIndex_[viewIndex][1];

			values.punctualAtrousScratch0Index_ = punctualAtrousScratchShaderResourceViewIndex_[viewIndex][0];
			values.punctualAtrousScratch1Index_ = punctualAtrousScratchShaderResourceViewIndex_[viewIndex][1];

			values.directionalMomentsHistoryIndex_ = directionalMomentsShaderResourceViewIndex_[viewIndex][historySlot_];
			values.directionalMomentsIndex_ = directionalMomentsShaderResourceViewIndex_[viewIndex][writeSlot];

			values.punctualMomentsHistoryIndex_ = punctualMomentsShaderResourceViewIndex_[viewIndex][historySlot_];
			values.punctualMomentsIndex_ = punctualMomentsShaderResourceViewIndex_[viewIndex][writeSlot];

			values.historyLengthHistoryIndex_ = historyLengthShaderResourceViewIndex_[viewIndex][historySlot_];
			values.historyLengthIndex_ = historyLengthShaderResourceViewIndex_[viewIndex][writeSlot];

			values.depthNormalHistoryIndex_ = depthNormalShaderResourceViewIndex_[viewIndex][historySlot_];
			values.depthNormalIndex_ = depthNormalShaderResourceViewIndex_[viewIndex][writeSlot];

			return values;
		};

		/// [EN] The write targets of every SVGF pass for one view.
		/// [JP] 1 つのビューの、SVGF 各パスの書き込み先。
		auto buildUnorderedAccessIndices = [&](Uint32 viewIndex)
		{
			ShadowAccumulationUnorderedAccessIndices values{};

			values.directionalAccumulatedIndex_ = directionalAccumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.directionalMomentsIndex_ = directionalMomentsUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.directionalAtrousScratch0Index_ = directionalAtrousScratchUnorderedAccessViewIndex_[viewIndex][0];
			values.directionalAtrousScratch1Index_ = directionalAtrousScratchUnorderedAccessViewIndex_[viewIndex][1];
			values.directionalDenoisedIndex_ = directionalDenoisedUnorderedAccessViewIndex_[viewIndex];

			values.punctualAccumulatedIndex_ = punctualAccumulatedUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.punctualMomentsIndex_ = punctualMomentsUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.punctualAtrousScratch0Index_ = punctualAtrousScratchUnorderedAccessViewIndex_[viewIndex][0];
			values.punctualAtrousScratch1Index_ = punctualAtrousScratchUnorderedAccessViewIndex_[viewIndex][1];
			values.punctualDenoisedIndex_ = punctualDenoisedUnorderedAccessViewIndex_[viewIndex];

			values.historyLengthIndex_ = historyLengthUnorderedAccessViewIndex_[viewIndex][writeSlot];
			values.depthNormalIndex_ = depthNormalUnorderedAccessViewIndex_[viewIndex][writeSlot];

			return values;
		};

		shaderResourceIndicesSystem_->SetEditorShadowAccumulationIndices(buildShaderResourceIndices(editorView));
		shaderResourceIndicesSystem_->SetGameShadowAccumulationIndices(buildShaderResourceIndices(gameView));
		unorderedAccessIndicesSystem_->SetEditorShadowAccumulationIndices(buildUnorderedAccessIndices(editorView));
		unorderedAccessIndicesSystem_->SetGameShadowAccumulationIndices(buildUnorderedAccessIndices(gameView));
	}

	/**
	* [EN]
	* Writes view's shadows: traced (and run through SVGF unless DLSS Ray
	* Reconstruction is on) when enabled_ and the needed pipelines exist,
	* otherwise the textures deferred lighting reads are cleared.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* view の影を書き込む。enabled_ で必要なパイプラインがあればトレースし
	* （DLSS Ray Reconstruction が無効なら SVGF も通す）、そうでなければ
	* ディファードライティングが読むテクスチャをクリアする。
	*/
	void ShadowRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view)
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

		/// [EN] Moves a texture into shader-read state if it is not there yet.
		/// [JP] テクスチャがまだシェーダーの読み取り状態でなければ、その状態へ移す。
		auto toRead = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state)
		{
			if (state != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			{
				cmdList->Barrier(resource.Get(), state, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
		};

		/// [EN] Moves a texture into unordered-access state if it is not there yet.
		/// [JP] テクスチャがまだ unordered-access 状態でなければ、その状態へ移す。
		auto toWrite = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state)
		{
			if (state != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				cmdList->Barrier(resource.Get(), state, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
		};

		/// [EN] Zero the whole history chain of every view and slot once after allocation, so no uninitialized texel is ever read back as history.
		/// [JP] 確保の後に 1 度だけ、全ビュー・全スロットの履歴チェーンを 0 で埋める。未初期化のテクセルが履歴として読み戻されないようにする。
		if (!historyCleared_)
		{
			/// [EN] Mark it done, so this happens only once.
			/// [JP] 済んだ印を付け、1 度だけ行うようにする。
			historyCleared_ = true;

			ID3D12DescriptorHeap* clearHeaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(clearHeaps), clearHeaps);

			const Float zeroValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			/// [EN] Clears one texture to zero through its bindless and clear-heap write views.
			/// [JP] bindless とクリア用ヒープの書き込みビューを使って、テクスチャを 1 枚 0 でクリアする。
			auto clearTexture = [&](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES& state, Uint32 unorderedAccessViewIndex, Uint32 clearIndex)
			{
				toWrite(resource, state);
				cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(unorderedAccessViewIndex), clearHeap_.CPUHandle(clearIndex), resource.Get(), zeroValues, 0, nullptr);
			};

			for (Uint32 clearView = 0; clearView < viewCount_; clearView++)
			{
				for (Uint32 clearSlot = 0; clearSlot < accumulationSlotCount_; clearSlot++)
				{
					clearTexture(directionalAccumulatedResource_[clearView][clearSlot], directionalAccumulatedState_[clearView][clearSlot], directionalAccumulatedUnorderedAccessViewIndex_[clearView][clearSlot], clearDirectionalAccumulatedIndex_[clearView][clearSlot]);
					clearTexture(punctualAccumulatedResource_[clearView][clearSlot], punctualAccumulatedState_[clearView][clearSlot], punctualAccumulatedUnorderedAccessViewIndex_[clearView][clearSlot], clearPunctualAccumulatedIndex_[clearView][clearSlot]);
					clearTexture(directionalMomentsResource_[clearView][clearSlot], directionalMomentsState_[clearView][clearSlot], directionalMomentsUnorderedAccessViewIndex_[clearView][clearSlot], clearDirectionalMomentsIndex_[clearView][clearSlot]);
					clearTexture(punctualMomentsResource_[clearView][clearSlot], punctualMomentsState_[clearView][clearSlot], punctualMomentsUnorderedAccessViewIndex_[clearView][clearSlot], clearPunctualMomentsIndex_[clearView][clearSlot]);
					clearTexture(historyLengthResource_[clearView][clearSlot], historyLengthState_[clearView][clearSlot], historyLengthUnorderedAccessViewIndex_[clearView][clearSlot], clearHistoryLengthIndex_[clearView][clearSlot]);
					clearTexture(depthNormalResource_[clearView][clearSlot], depthNormalState_[clearView][clearSlot], depthNormalUnorderedAccessViewIndex_[clearView][clearSlot], clearDepthNormalIndex_[clearView][clearSlot]);
				}
			}
		}

		/// [EN] The denoise pipeline only counts as required when this pass does its own denoising.
		/// [JP] デノイズのパイプラインは、このパスが自分でデノイズするときだけ必須とみなす。
		ID3D12PipelineState* shadowPipelineState = shadowShader_.GetPipelineState();
		ID3D12PipelineState* denoisePipelineState = denoiseShader_.GetPipelineState();
		Bool denoisePipelineRequired = !useDlssRayReconstruction_;
		Bool pipelinesReady = shadowPipelineState && (!denoisePipelineRequired || denoisePipelineState);

		/// [EN] The pipelines are missing when the GPU lacks inline raytracing (DXR Tier 1.1). The pass then falls back to fully lit instead of binding a null pipeline, and the warning tells an intentional "no shadow" apart from a missing feature.
		/// [JP] GPU がインラインレイトレーシング（DXR Tier 1.1）に非対応だとパイプラインが無い。その場合は null のパイプラインをバインドせず常に照射へ倒し、この警告で意図した「影なし」と機能の欠如を見分けられるようにする。
		if (!pipelinesReady && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("ShadowRT/ShadowDenoise のコンピュート PSO 作成に失敗しています。DXR インラインレイトレ(Tier 1.1)非対応の可能性があります。影は常に照射(1.0)として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		if (!enabled_ || !pipelinesReady)
		{
			/// [EN] Clear what deferred lighting actually reads. Directional visibility is a multiplier, so it clears to lit (1.0); punctual radiance is an added term, so it clears to 0.0 (adding 1.0 would add flat white).
			/// [JP] ディファードライティングが実際に読むものをクリアする。ディレクショナルの可視性は乗数なので照射（1.0）、パンクチュアルの放射輝度は加算項なので 0.0 でクリアする（1.0 だと平らな白を足してしまう）。
			if (useDlssRayReconstruction_)
			{
				/// [EN] On this path deferred lighting reads the raw texture: r is directional, gba punctual.
				/// [JP] この経路ではディファードライティングが生のテクスチャを読む。r がディレクショナル、gba がパンクチュアル。
				toWrite(rawVisibilityResource_, rawVisibilityState_);

				const Float rawClearValues[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
				cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(rawVisibilityUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearRawIndex_), rawVisibilityResource_.Get(), rawClearValues, 0, nullptr);

				cmdList->Barrier(rawVisibilityResource_.Get(), rawVisibilityState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				rawVisibilityState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
				return;
			}

			/// [EN] Otherwise deferred lighting reads this view's two denoised outputs.
			/// [JP] そうでなければディファードライティングはこのビューの 2 つのデノイズ済み出力を読む。
			toWrite(directionalDenoisedResource_[viewIndex], directionalDenoisedState_[viewIndex]);

			const Float litValues[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(directionalDenoisedUnorderedAccessViewIndex_[viewIndex]), clearHeap_.CPUHandle(clearDirectionalDenoisedIndex_[viewIndex]), directionalDenoisedResource_[viewIndex].Get(), litValues, 0, nullptr);

			cmdList->Barrier(directionalDenoisedResource_[viewIndex].Get(), directionalDenoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			directionalDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

			toWrite(punctualDenoisedResource_[viewIndex], punctualDenoisedState_[viewIndex]);

			const Float noRadianceValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(punctualDenoisedUnorderedAccessViewIndex_[viewIndex]), clearHeap_.CPUHandle(clearPunctualDenoisedIndex_[viewIndex]), punctualDenoisedResource_[viewIndex].Get(), noRadianceValues, 0, nullptr);

			cmdList->Barrier(punctualDenoisedResource_[viewIndex].Get(), punctualDenoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			punctualDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

			/// [EN] Reset the history length to 0, so the next traced frame starts converging afresh instead of trusting a history that holds no real signal.
			/// [JP] 履歴長を 0 に戻す。次にトレースしたフレームが、実際の信号を持たない履歴を信用せず、最初から収束し直すようにする。
			toWrite(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot]);

			const Float zeroValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(historyLengthUnorderedAccessViewIndex_[viewIndex][writeSlot]), clearHeap_.CPUHandle(clearHistoryLengthIndex_[viewIndex][writeSlot]), historyLengthResource_[viewIndex][writeSlot].Get(), zeroValues, 0, nullptr);
			return;
		}

		/// [EN] Trace the raw visibility and punctual radiance.
		/// [JP] 生の可視性とパンクチュアルの放射輝度をトレースする。
		toWrite(rawVisibilityResource_, rawVisibilityState_);

		ID3D12DescriptorHeap* heaps[] = { heap };
		/// [EN] Make the bindless heap the active shader-visible heap.
		/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		/// [EN] All passes share one root signature; set it on the compute binding point.
		/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
		cmd->SetComputeRootSignature(shadowShader_.GetRootSignature());
		/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
		/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
		RootSignature::BindCompute(cmd, addresses);
		cmd->SetPipelineState(shadowPipelineState);

		/// [EN] One 8x8 thread group per 8x8 pixel tile, rounded up to cover the edges; every SVGF pass uses the same grid.
		/// [JP] 8x8 ピクセルのタイルごとに 8x8 のスレッドグループを 1 つ。端まで覆うよう切り上げる。SVGF の各パスも同じグリッドを使う。
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
			cmdList->Barrier(rawVisibilityResource_.Get(), rawVisibilityState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			rawVisibilityState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			return;
		}

		toRead(rawVisibilityResource_, rawVisibilityState_);

		/// [EN] The reprojection reads the history side of every carried buffer.
		/// [JP] リプロジェクションは、引き継ぐすべてのバッファの履歴側を読む。
		toRead(directionalAccumulatedResource_[viewIndex][historySlot_], directionalAccumulatedState_[viewIndex][historySlot_]);
		toRead(punctualAccumulatedResource_[viewIndex][historySlot_], punctualAccumulatedState_[viewIndex][historySlot_]);
		toRead(directionalMomentsResource_[viewIndex][historySlot_], directionalMomentsState_[viewIndex][historySlot_]);
		toRead(punctualMomentsResource_[viewIndex][historySlot_], punctualMomentsState_[viewIndex][historySlot_]);
		toRead(historyLengthResource_[viewIndex][historySlot_], historyLengthState_[viewIndex][historySlot_]);
		toRead(depthNormalResource_[viewIndex][historySlot_], depthNormalState_[viewIndex][historySlot_]);

		/// [EN] Pass 1, reprojection: raw and history into scratch 0 of both signals, plus this frame's moments, history length and depth-normal copy.
		/// [JP] パス 1、リプロジェクション。生と履歴から、両信号のスクラッチ 0 と、今フレームのモーメント、履歴長、深度法線コピーを作る。
		toWrite(directionalAtrousScratchResource_[viewIndex][0], directionalAtrousScratchState_[viewIndex][0]);
		toWrite(punctualAtrousScratchResource_[viewIndex][0], punctualAtrousScratchState_[viewIndex][0]);
		toWrite(directionalMomentsResource_[viewIndex][writeSlot], directionalMomentsState_[viewIndex][writeSlot]);
		toWrite(punctualMomentsResource_[viewIndex][writeSlot], punctualMomentsState_[viewIndex][writeSlot]);
		toWrite(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot]);
		toWrite(depthNormalResource_[viewIndex][writeSlot], depthNormalState_[viewIndex][writeSlot]);

		cmd->SetPipelineState(denoisePipelineState);
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] The later passes only read the moments, history length and depth-normal copy, so they move to read state once here and stay there.
		/// [JP] 以降のパスはモーメント、履歴長、深度法線コピーを読むだけなので、ここで 1 度だけ読み取り状態へ移してそのまま保つ。
		toRead(directionalMomentsResource_[viewIndex][writeSlot], directionalMomentsState_[viewIndex][writeSlot]);
		toRead(punctualMomentsResource_[viewIndex][writeSlot], punctualMomentsState_[viewIndex][writeSlot]);
		toRead(historyLengthResource_[viewIndex][writeSlot], historyLengthState_[viewIndex][writeSlot]);
		toRead(depthNormalResource_[viewIndex][writeSlot], depthNormalState_[viewIndex][writeSlot]);
		toRead(directionalAtrousScratchResource_[viewIndex][0], directionalAtrousScratchState_[viewIndex][0]);
		toRead(punctualAtrousScratchResource_[viewIndex][0], punctualAtrousScratchState_[viewIndex][0]);

		/// [EN] Pass 2, FilterMoments: scratch 0 into scratch 1 for both signals.
		/// [JP] パス 2、FilterMoments。両信号のスクラッチ 0 からスクラッチ 1 へ。
		toWrite(directionalAtrousScratchResource_[viewIndex][1], directionalAtrousScratchState_[viewIndex][1]);
		toWrite(punctualAtrousScratchResource_[viewIndex][1], punctualAtrousScratchState_[viewIndex][1]);

		cmd->SetPipelineState(denoiseShader_.GetFilterMomentsPipelineState());
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(directionalAtrousScratchResource_[viewIndex][1], directionalAtrousScratchState_[viewIndex][1]);
		toRead(punctualAtrousScratchResource_[viewIndex][1], punctualAtrousScratchState_[viewIndex][1]);

		/// [EN] Pass 3, A-Trous step 1: scratch 1 back into scratch 0.
		/// [JP] パス 3、A-Trous ステップ 1。スクラッチ 1 からスクラッチ 0 へ戻す。
		toWrite(directionalAtrousScratchResource_[viewIndex][0], directionalAtrousScratchState_[viewIndex][0]);
		toWrite(punctualAtrousScratchResource_[viewIndex][0], punctualAtrousScratchState_[viewIndex][0]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(0));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(directionalAtrousScratchResource_[viewIndex][0], directionalAtrousScratchState_[viewIndex][0]);
		toRead(punctualAtrousScratchResource_[viewIndex][0], punctualAtrousScratchState_[viewIndex][0]);

		/// [EN] Pass 4, A-Trous step 2, the feedback tap: scratch 0 into the history write slot, which becomes next frame's history.
		/// [JP] パス 4、A-Trous ステップ 2（フィードバックタップ）。スクラッチ 0 から履歴の書き込みスロットへ。これが次フレームの履歴になる。
		toWrite(directionalAccumulatedResource_[viewIndex][writeSlot], directionalAccumulatedState_[viewIndex][writeSlot]);
		toWrite(punctualAccumulatedResource_[viewIndex][writeSlot], punctualAccumulatedState_[viewIndex][writeSlot]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(1));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		toRead(directionalAccumulatedResource_[viewIndex][writeSlot], directionalAccumulatedState_[viewIndex][writeSlot]);
		toRead(punctualAccumulatedResource_[viewIndex][writeSlot], punctualAccumulatedState_[viewIndex][writeSlot]);

		/// [EN] Pass 5, A-Trous step 4: the history write slot into each signal's denoised output.
		/// [JP] パス 5、A-Trous ステップ 4。履歴の書き込みスロットから各信号のデノイズ済み出力へ。
		toWrite(directionalDenoisedResource_[viewIndex], directionalDenoisedState_[viewIndex]);
		toWrite(punctualDenoisedResource_[viewIndex], punctualDenoisedState_[viewIndex]);

		cmd->SetPipelineState(denoiseShader_.GetATrousPipelineState(2));
		cmd->Dispatch(groupCountX, groupCountY, 1);
		/// [EN] Count the dispatch in the profiler's statistics.
		/// [JP] このディスパッチをプロファイラーの統計に数える。
		ProfilerStats::AddDrawCall();

		/// [EN] Hand the denoised outputs over to deferred lighting as shader resources.
		/// [JP] デノイズ済みの出力をシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(directionalDenoisedResource_[viewIndex].Get(), directionalDenoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		directionalDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		cmdList->Barrier(punctualDenoisedResource_[viewIndex].Get(), punctualDenoisedState_[viewIndex], D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		punctualDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		/// [EN] The raw texture also ends readable by pixel shaders: the denoise chain only needs non-pixel read state, but the raw-shadow debug view samples it from DeferredLightingPS.
		/// [JP] 生のテクスチャもピクセルシェーダーから読める状態で終える。デノイズのチェーンには非ピクセルの読み取り状態で足りるが、生の影のデバッグ表示は DeferredLightingPS からサンプルするため。
		cmdList->Barrier(rawVisibilityResource_.Get(), rawVisibilityState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		rawVisibilityState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the raw texture and, per view, the whole SVGF chain at
	* width_ x height_, and marks the new history chain for zeroing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の生のテクスチャと、ビューごとの SVGF チェーン一式を
	* 作成し、新しい履歴チェーンを 0 埋めの対象にする。
	*/
	void ShadowRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] One clear view for the raw texture, two per view for the denoised outputs, and six per view and slot for the history chain.
		/// [JP] クリア用ビューは、生のテクスチャに 1 つ、ビューごとのデノイズ済み出力に 2 つ、ビューとスロットごとの履歴チェーンに 6 つ。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1 + viewCount_ * 2 + viewCount_ * accumulationSlotCount_ * 6, false);

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
			SC_HR_CHECK(hr, "シャドウデノイズ用テクスチャの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(L"Shadow_Denoise");
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

		/// [EN] Raw output: r is directional visibility, gba punctual radiance.
		/// [JP] 生の出力。r がディレクショナルの可視性、gba がパンクチュアルの放射輝度。
		createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, rawVisibilityResource_, rawVisibilityUnorderedAccessViewIndex_, rawVisibilityShaderResourceViewIndex_, &clearRawIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		rawVisibilityState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] The shifted read view of the raw texture maps destination r, g, b to source g, b, a, so .rgb read through it is the punctual radiance.
		/// [JP] 生のテクスチャのずらした読み取り用ビューは、読み出し先の r, g, b を元の g, b, a に対応させる。このビューで読む .rgb がパンクチュアルの放射輝度になる。
		rawPunctualShaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC rawPunctualShaderResourceViewDesc{};
		rawPunctualShaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rawPunctualShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		rawPunctualShaderResourceViewDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(1, 2, 3, 3);
		rawPunctualShaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(rawVisibilityResource_.Get(), &rawPunctualShaderResourceViewDesc, bindlessHeap_->CPUHandle(rawPunctualShaderResourceViewIndex_));

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			/// [EN] The history chain: both signals' history and moments, and the shared history length and depth-normal copy.
			/// [JP] 履歴チェーン。両信号の履歴とモーメント、共有の履歴長と深度法線コピー。
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16_FLOAT, directionalAccumulatedResource_[viewIndex][slotIndex], directionalAccumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalAccumulatedShaderResourceViewIndex_[viewIndex][slotIndex], &clearDirectionalAccumulatedIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				directionalAccumulatedState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, punctualAccumulatedResource_[viewIndex][slotIndex], punctualAccumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualAccumulatedShaderResourceViewIndex_[viewIndex][slotIndex], &clearPunctualAccumulatedIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				punctualAccumulatedState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16G16_FLOAT, directionalMomentsResource_[viewIndex][slotIndex], directionalMomentsUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalMomentsShaderResourceViewIndex_[viewIndex][slotIndex], &clearDirectionalMomentsIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				directionalMomentsState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16G16_FLOAT, punctualMomentsResource_[viewIndex][slotIndex], punctualMomentsUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualMomentsShaderResourceViewIndex_[viewIndex][slotIndex], &clearPunctualMomentsIndex_[viewIndex][slotIndex]);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				punctualMomentsState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

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

			/// [EN] The A-Trous scratch pairs are always fully overwritten, so they need no clear view.
			/// [JP] A-Trous のスクラッチは必ず全画素上書きされるため、クリア用ビューは要らない。
			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				createTexture(DXGI_FORMAT_R16G16_FLOAT, directionalAtrousScratchResource_[viewIndex][slotIndex], directionalAtrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalAtrousScratchShaderResourceViewIndex_[viewIndex][slotIndex], nullptr);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				directionalAtrousScratchState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;

				createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, punctualAtrousScratchResource_[viewIndex][slotIndex], punctualAtrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualAtrousScratchShaderResourceViewIndex_[viewIndex][slotIndex], nullptr);
				/// [EN] Track the state it was created in, so the first barrier starts from the right state.
				/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
				punctualAtrousScratchState_[viewIndex][slotIndex] = D3D12_RESOURCE_STATE_COMMON;
			}

			/// [EN] The denoised outputs deferred lighting samples.
			/// [JP] ディファードライティングがサンプルするデノイズ済みの出力。
			createTexture(DXGI_FORMAT_R16_FLOAT, directionalDenoisedResource_[viewIndex], directionalDenoisedUnorderedAccessViewIndex_[viewIndex], directionalDenoisedShaderResourceViewIndex_[viewIndex], &clearDirectionalDenoisedIndex_[viewIndex]);
			/// [EN] Track the state it was created in, so the first barrier starts from the right state.
			/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
			directionalDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_COMMON;

			createTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, punctualDenoisedResource_[viewIndex], punctualDenoisedUnorderedAccessViewIndex_[viewIndex], punctualDenoisedShaderResourceViewIndex_[viewIndex], &clearPunctualDenoisedIndex_[viewIndex]);
			/// [EN] Track the state it was created in, so the first barrier starts from the right state.
			/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
			punctualDenoisedState_[viewIndex] = D3D12_RESOURCE_STATE_COMMON;
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
	void ShadowRenderer::Release()
	{
		/// [EN] Frees one texture's two bindless views and defers its destruction.
		/// [JP] テクスチャ 1 枚の bindless ビュー 2 つを解放し、破棄を遅延させる。
		auto releaseTexture = [this](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32 unorderedAccessViewIndex, Uint32 shaderResourceViewIndex)
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

		/// [EN] The raw texture also owns the shifted punctual read view.
		/// [JP] 生のテクスチャは、ずらしたパンクチュアル用の読み取りビューも持つ。
		bindlessHeap_->FreeIndex(rawPunctualShaderResourceViewIndex_);
		releaseTexture(rawVisibilityResource_, rawVisibilityUnorderedAccessViewIndex_, rawVisibilityShaderResourceViewIndex_);

		for (Uint32 viewIndex = 0; viewIndex < viewCount_; viewIndex++)
		{
			for (Uint32 slotIndex = 0; slotIndex < accumulationSlotCount_; slotIndex++)
			{
				releaseTexture(directionalAccumulatedResource_[viewIndex][slotIndex], directionalAccumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalAccumulatedShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(punctualAccumulatedResource_[viewIndex][slotIndex], punctualAccumulatedUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualAccumulatedShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(directionalMomentsResource_[viewIndex][slotIndex], directionalMomentsUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalMomentsShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(punctualMomentsResource_[viewIndex][slotIndex], punctualMomentsUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualMomentsShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(historyLengthResource_[viewIndex][slotIndex], historyLengthUnorderedAccessViewIndex_[viewIndex][slotIndex], historyLengthShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(depthNormalResource_[viewIndex][slotIndex], depthNormalUnorderedAccessViewIndex_[viewIndex][slotIndex], depthNormalShaderResourceViewIndex_[viewIndex][slotIndex]);
			}

			for (Uint32 slotIndex = 0; slotIndex < 2; slotIndex++)
			{
				releaseTexture(directionalAtrousScratchResource_[viewIndex][slotIndex], directionalAtrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], directionalAtrousScratchShaderResourceViewIndex_[viewIndex][slotIndex]);
				releaseTexture(punctualAtrousScratchResource_[viewIndex][slotIndex], punctualAtrousScratchUnorderedAccessViewIndex_[viewIndex][slotIndex], punctualAtrousScratchShaderResourceViewIndex_[viewIndex][slotIndex]);
			}

			releaseTexture(directionalDenoisedResource_[viewIndex], directionalDenoisedUnorderedAccessViewIndex_[viewIndex], directionalDenoisedShaderResourceViewIndex_[viewIndex]);
			releaseTexture(punctualDenoisedResource_[viewIndex], punctualDenoisedUnorderedAccessViewIndex_[viewIndex], punctualDenoisedShaderResourceViewIndex_[viewIndex]);
		}
	}
}

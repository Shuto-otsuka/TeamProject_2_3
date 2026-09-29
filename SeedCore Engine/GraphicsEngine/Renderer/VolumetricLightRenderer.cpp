#include <GraphicsEngine/Renderer/VolumetricLightRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Math/Halton.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the shared root signature and pipeline-state cache to the froxel
	* shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、froxel の
	* シェーダーへ関連付ける。
	*/
	VolumetricLightRenderer::VolumetricLightRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : volumetricLightShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates everything the pass uses. All volumes have the fixed froxel
	* size, so nothing here is ever recreated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パスが使うものをすべて作成する。ボリュームはすべて固定の froxel サイズ
	* なので、ここで作ったものを作り直すことはない。
	*/
	void VolumetricLightRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		/// [EN] Keep the heap and index systems; Prepare and Dispatch use them later.
		/// [JP] ヒープと各インデックスシステムを保持する。後で Prepare と Dispatch が使う。
		bindlessHeap_ = bindlessHeap;
		constantIndicesSystem_ = &constantIndicesSystem;
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;
		unorderedAccessIndicesSystem_ = &unorderedAccessIndicesSystem;

		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		volumetricLightShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<VolumetricLightRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Recreate the CPU-side heap that holds the clear views, sized for every clearable resource.
		/// [JP] クリア用ビューを置く CPU 側のヒープを、クリアするリソースの数だけ作り直す。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);

		/// [EN] Creates one froxel-sized RGBA16F volume with a bindless write view, plus an optional read view and an optional clear view.
		/// [JP] bindless の書き込み用ビューと、必要に応じて読み取り用ビューとクリア用ビューを持つ、froxel サイズの RGBA16F ボリュームを 1 つ作る。
		auto createVolume = [this, device, bindlessHeap](Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& unorderedAccessViewIndex, Uint32* shaderResourceViewIndex, Uint32* clearIndex)
		{
			/// [EN] GPU-local memory: only the GPU reads and writes these resources.
			/// [JP] GPU ローカルなメモリ。これらのリソースは GPU だけが読み書きする。
			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			/// [EN] One texture without mipmaps or multisampling, writable through an unordered-access view.
			/// [JP] テクスチャ 1 枚。ミップマップもマルチサンプルも無く、unordered-access ビューで書き込める。
			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			resourceDesc.Width = froxelDimensionX_;
			resourceDesc.Height = froxelDimensionY_;
			resourceDesc.DepthOrArraySize = static_cast<Uint16>(froxelDimensionZ_);
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			/// [EN] Create the texture in its own GPU-local heap, starting in COMMON state.
			/// [JP] テクスチャを専用の GPU ローカルなヒープに、COMMON 状態で作る。
			HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
			SC_HR_CHECK(hr, "ボリュームリソースの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(L"VolumetricLight");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

			/// [EN] The write view sees the whole resource in its own format.
			/// [JP] 書き込み用ビューは、リソース全体を自身のフォーマットで見る。
			D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
			unorderedAccessViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			unorderedAccessViewDesc.Texture3D.WSize = froxelDimensionZ_;

			/// [EN] Reserve a bindless slot and write the write view into it.
			/// [JP] bindless のスロットを確保し、書き込み用ビューを入れる。
			unorderedAccessViewIndex = bindlessHeap->AllocateIndex();
			device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap->CPUHandle(unorderedAccessViewIndex));

			if (clearIndex)
			{
				/// [EN] A second copy of the write view in the CPU-side heap, for clears.
				/// [JP] クリア用に、書き込み用ビューの 2 つ目を CPU 側のヒープに作る。
				*clearIndex = clearHeap_.AllocateIndex();
				device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(*clearIndex));
			}

			if (shaderResourceViewIndex)
			{
				*shaderResourceViewIndex = bindlessHeap->AllocateIndex();
				/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
				/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
				D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
				shaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				shaderResourceViewDesc.Texture3D.MipLevels = 1;
				device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, bindlessHeap->CPUHandle(*shaderResourceViewIndex));
			}
		};

		/// [EN] The density volume is only touched through its write view, so it needs no read or clear view.
		/// [JP] 密度ボリュームは書き込み用ビューでしか扱わないため、読み取り用もクリア用も要らない。
		createVolume(densityVolumeResource_, densityVolumeUnorderedAccessViewIndex_, nullptr, nullptr);

		/// [EN] Each view gets its own pair of scattering volumes and its own dispatch constants, starting with no history.
		/// [JP] 各ビューに専用の散乱ボリュームの組とディスパッチ用定数を作り、履歴なしの状態から始める。
		for (View* view : { &editorView_, &gameView_ })
		{
			for (Uint32 slot = 0; slot < scatteringSlotCount_; slot++)
			{
				createVolume(view->scatteringVolumeResource_[slot], view->scatteringVolumeUnorderedAccessViewIndex_[slot], &view->scatteringVolumeShaderResourceViewIndex_[slot], nullptr);
				view->scatteringVolumeState_[slot] = D3D12_RESOURCE_STATE_COMMON;
			}

			view->writeSlot_ = 0;
			view->frameIndex_ = 0;
			view->historyValid_ = false;
			view->constantBuffer_ = MakePtr<ConstantBuffer<VolumetricLightDispatchConstantBuffer>>(device, bindlessHeap);
		}

		/// [EN] The integration volume is read by the composite and cleared when the pass is off, so it gets both extra views.
		/// [JP] 積分ボリュームは合成が読み、パスが無効ならクリアするため、両方の追加ビューを持たせる。
		createVolume(integrationVolumeResource_, integrationVolumeUnorderedAccessViewIndex_, &integrationVolumeShaderResourceViewIndex_, &clearIntegrationIndex_);
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		integrationVolumeState_ = D3D12_RESOURCE_STATE_COMMON;
	}

	/**
	* [EN]
	* Uploads the tuning values with the grid dimensions filled in, stores
	* the enabled flag for Dispatch, and publishes the density and
	* integration volume views to the index systems.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グリッドの次元を埋めた調整値をアップロードし、Dispatch 用に有効フラグを
	* 保存して、密度と積分のボリュームのビューを各インデックスシステムへ公開する。
	*/
	void VolumetricLightRenderer::Prepare(const VolumetricLightRayConstantBuffer& settings, Bool enabled)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;

		/// [EN] The upload copy carries the fixed grid dimensions on top of the user's tuning.
		/// [JP] アップロード用のコピーには、ユーザーの調整値に加えて固定のグリッド次元を載せる。
		VolumetricLightRayConstantBuffer uploadSettings = settings;
		uploadSettings.froxelDimensionX_ = froxelDimensionX_;
		uploadSettings.froxelDimensionY_ = froxelDimensionY_;
		uploadSettings.froxelDimensionZ_ = froxelDimensionZ_;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);

		/// [EN] The per-view scattering volumes are passed through the dispatch constants instead, since they differ per view.
		/// [JP] ビューごとの散乱ボリュームはビューによって異なるため、代わりにディスパッチ用定数で渡す。
		constantIndicesSystem_->SetVolumetricLightRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetVolumetricLightDensityUnorderedAccessViewIndex(densityVolumeUnorderedAccessViewIndex_);
		unorderedAccessIndicesSystem_->SetVolumetricLightIntegrationUnorderedAccessViewIndex(integrationVolumeUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetVolumetricLightIntegrationShaderResourceViewIndex(integrationVolumeShaderResourceViewIndex_);
	}

	/**
	* [EN]
	* Runs the three froxel passes for view when enabled_ and every pipeline
	* exist, or clears the integration volume to "no fog" otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* enabled_ で全パイプラインがあれば view に対して froxel の 3 パスを実行し、
	* そうでなければ積分ボリュームを「フォグなし」でクリアする。
	*/
	void VolumetricLightRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] The temporal state that belongs to the view being rendered.
		/// [JP] 描画中のビューに属する時間方向の状態。
		View& target = view == RaytracingView::Editor ? editorView_ : gameView_;

		/// [EN] The density volume is only accessed as an unordered-access view, so it moves into that state once and stays there.
		/// [JP] 密度ボリュームは unordered-access ビューとしてしか扱わないため、1 度だけその状態へ移し、そのまま保つ。
		if (!workingVolumesTransitioned_)
		{
			cmdList->Barrier(densityVolumeResource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			workingVolumesTransitioned_ = true;
		}

		/// [EN] Both the passes and the clear write the integration volume.
		/// [JP] パスもクリアも積分ボリュームへ書き込む。
		if (integrationVolumeState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(integrationVolumeResource_.Get(), integrationVolumeState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			integrationVolumeState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		/// [EN] All three pipelines are needed; report a missing one once.
		/// [JP] 3 つのパイプラインがすべて必要。欠けていれば 1 度だけ報告する。
		ID3D12PipelineState* injectionPipeline = volumetricLightShader_.GetInjectionPipelineState();
		ID3D12PipelineState* scatteringPipeline = volumetricLightShader_.GetScatteringPipelineState();
		ID3D12PipelineState* integrationPipeline = volumetricLightShader_.GetIntegrationPipelineState();

		Bool pipelinesReady = injectionPipeline && scatteringPipeline && integrationPipeline;
		if (!pipelinesReady && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("FogInjection/VolumetricLightScattering/FroxelIntegration のコンピュート PSO 作成に失敗しています。フォグ/体積光は常に無し として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		if (!enabled_ || !pipelinesReady)
		{
			/// [EN] Zero scattering and full transmittance make the composite leave the image unchanged.
			/// [JP] 散乱 0 と完全透過なら、合成しても画像は変わらない。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(integrationVolumeUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearIntegrationIndex_), integrationVolumeResource_.Get(), clearValues, 0, nullptr);

			/// [EN] The history did not advance this frame, so the next enabled frame must not blend with it.
			/// [JP] 今フレームは履歴が進んでいないため、次に有効になったフレームでは履歴と混ぜてはいけない。
			target.historyValid_ = false;
		}
		else
		{
			/// [EN] Write one scattering volume while reading the other as history.
			/// [JP] 一方の散乱ボリュームに書き込み、もう一方を履歴として読む。
			Uint32 writeSlot = target.writeSlot_;
			Uint32 historySlot = 1 - writeSlot;

			if (target.scatteringVolumeState_[writeSlot] != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				cmdList->Barrier(target.scatteringVolumeResource_[writeSlot].Get(), target.scatteringVolumeState_[writeSlot], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				target.scatteringVolumeState_[writeSlot] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}

			if (target.scatteringVolumeState_[historySlot] != D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
			{
				cmdList->Barrier(target.scatteringVolumeResource_[historySlot].Get(), target.scatteringVolumeState_[historySlot], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				target.scatteringVolumeState_[historySlot] = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}

			/// [EN] Jitter the samples with a Halton (2, 3, 5) point, so the accumulated history covers each froxel evenly.
			/// [JP] Halton (2, 3, 5) の点でサンプルをずらし、蓄積した履歴が各 froxel を均等に覆うようにする。
			Uint32 sampleIndex = (target.frameIndex_ % froxelJitterSequenceLength_) + 1;

			VolumetricLightDispatchConstantBuffer constants{};
			constants.scatteringWriteUnorderedAccessViewIndex_ = target.scatteringVolumeUnorderedAccessViewIndex_[writeSlot];
			constants.scatteringHistoryShaderResourceViewIndex_ = target.scatteringVolumeShaderResourceViewIndex_[historySlot];
			constants.historyValid_ = target.historyValid_ ? 1 : 0;
			constants.frameIndex_ = target.frameIndex_;
			constants.jitter_ = Vector3(HaltonRadicalInverse(sampleIndex, 2) - 0.5f, HaltonRadicalInverse(sampleIndex, 3) - 0.5f, HaltonRadicalInverse(sampleIndex, 5) - 0.5f);
			target.constantBuffer_->Update(constants);

			/// [EN] The shaders read the shared inputs through the bindless heap and root addresses, and this view's constants through root parameter 3.
			/// [JP] シェーダーは共有の入力を bindless ヒープとルートアドレスで、このビューの定数をルートパラメーター 3 で読む。
			ID3D12DescriptorHeap* heaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(heaps), heaps);
			/// [EN] All passes share one root signature; set it on the compute binding point.
			/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
			cmd->SetComputeRootSignature(volumetricLightShader_.GetRootSignature());
			/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
			/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
			RootSignature::BindCompute(cmd, addresses);
			Uint dispatchBufferIndex = target.constantBuffer_->GetIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);

			/// [EN] Injection and scattering run one 4x4x4 thread group per 4x4x4 froxel block.
			/// [JP] 注入と散乱は、4x4x4 froxel のブロックごとに 4x4x4 のスレッドグループを 1 つ使う。
			Uint32 groupsX = (froxelDimensionX_ + 3) / 4;
			Uint32 groupsY = (froxelDimensionY_ + 3) / 4;
			Uint32 groupsZ = (froxelDimensionZ_ + 3) / 4;

			/// [EN] Each pass reads what the previous one wrote, so an unordered-access barrier separates them.
			/// [JP] 各パスは直前のパスが書いたものを読むため、unordered-access バリアで区切る。
			D3D12_RESOURCE_BARRIER unorderedAccessBarrier{};
			unorderedAccessBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;

			/// [EN] Pass 1: fill the density volume with the fog medium.
			/// [JP] パス 1: 密度ボリュームをフォグの媒質で埋める。
			cmd->SetPipelineState(injectionPipeline);
			cmd->Dispatch(groupsX, groupsY, groupsZ);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();

			unorderedAccessBarrier.UAV.pResource = densityVolumeResource_.Get();
			cmd->ResourceBarrier(1, &unorderedAccessBarrier);

			/// [EN] Pass 2: light each froxel and blend it with the history.
			/// [JP] パス 2: 各 froxel を照らし、履歴と混ぜる。
			cmd->SetPipelineState(scatteringPipeline);
			cmd->Dispatch(groupsX, groupsY, groupsZ);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();

			unorderedAccessBarrier.UAV.pResource = target.scatteringVolumeResource_[writeSlot].Get();
			cmd->ResourceBarrier(1, &unorderedAccessBarrier);

			/// [EN] Pass 3: integrate front to back; each thread walks one whole depth column, so the grid is 2D.
			/// [JP] パス 3: 手前から奥へ積分する。各スレッドが奥行きの列を 1 本まるごと進むため、グリッドは 2 次元。
			cmd->SetPipelineState(integrationPipeline);
			cmd->Dispatch((froxelDimensionX_ + 7) / 8, (froxelDimensionY_ + 7) / 8, 1);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();

			/// [EN] What was just written becomes next frame's history.
			/// [JP] いま書いたものが次フレームの履歴になる。
			target.historyValid_ = true;
			target.writeSlot_ = historySlot;
			target.frameIndex_++;
		}

		/// [EN] Hand the integration volume over to the composite as a shader resource.
		/// [JP] 積分ボリュームをシェーダーリソースとして合成へ渡す。
		cmdList->Barrier(integrationVolumeResource_.Get(), integrationVolumeState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		integrationVolumeState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}
}

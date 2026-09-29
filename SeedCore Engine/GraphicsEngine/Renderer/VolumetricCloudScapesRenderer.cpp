#include <GraphicsEngine/Renderer/VolumetricCloudScapesRenderer.h>

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
	* Binds the shared root signature and pipeline-state cache to the cloud
	* shaders.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、雲の
	* シェーダーへ関連付ける。
	*/
	VolumetricCloudScapesRenderer::VolumetricCloudScapesRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : cloudShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once - shaders, tuning buffer and
	* the two noise volumes - then allocates the size-dependent cloud
	* texture.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクト（シェーダー、調整値のバッファ、ノイズ
	* ボリューム 2 つ）を 1 度だけ作成し、その後サイズに依存する雲テクスチャを
	* 確保する。
	*/
	void VolumetricCloudScapesRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		/// [EN] Keep the heap and index systems; Allocate, Release and Prepare use them later.
		/// [JP] ヒープと各インデックスシステムを保持する。後で Allocate、Release、Prepare が使う。
		bindlessHeap_ = bindlessHeap;
		constantIndicesSystem_ = &constantIndicesSystem;
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;
		unorderedAccessIndicesSystem_ = &unorderedAccessIndicesSystem;

		/// [EN] Round up, so the reduced texture still covers the whole screen when the size is not a multiple of the divisor.
		/// [JP] 切り上げるため、サイズが割る数の倍数でなくても縮小したテクスチャで画面全体を覆える。
		width_ = (width + resolutionDivisor_ - 1) / resolutionDivisor_;
		height_ = (height + resolutionDivisor_ - 1) / resolutionDivisor_;

		/// [EN] Compile the shader, or take it from the shader cache.
		/// [JP] シェーダーをコンパイルする（シェーダーキャッシュにあればそれを使う）。
		cloudShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<VolumetricCloudScapesRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Creates one size^3 R16_FLOAT noise volume with a write view for the bake and a read view for the raymarch.
		/// [JP] 焼き込み用の書き込みビューとレイマーチ用の読み取りビューを持つ、size^3 の R16_FLOAT ノイズボリュームを 1 つ作る。
		auto createNoiseVolume = [device, bindlessHeap](Uint32 size, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, Uint32& unorderedAccessViewIndex, Uint32& shaderResourceViewIndex)
		{
			/// [EN] GPU-local memory: only the GPU reads and writes these resources.
			/// [JP] GPU ローカルなメモリ。これらのリソースは GPU だけが読み書きする。
			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			/// [EN] One texture without mipmaps or multisampling, writable through an unordered-access view.
			/// [JP] テクスチャ 1 枚。ミップマップもマルチサンプルも無く、unordered-access ビューで書き込める。
			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			resourceDesc.Width = size;
			resourceDesc.Height = size;
			resourceDesc.DepthOrArraySize = static_cast<Uint16>(size);
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_R16_FLOAT;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			/// [EN] Create the texture in its own GPU-local heap, starting in COMMON state.
			/// [JP] テクスチャを専用の GPU ローカルなヒープに、COMMON 状態で作る。
			HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
			SC_HR_CHECK(hr, "ノイズテクスチャの生成に失敗しました");
			/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
			/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
			resource->SetName(L"VolumetricCloudScapes_Noise");
			GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

			/// [EN] The write view sees the whole resource in its own format.
			/// [JP] 書き込み用ビューは、リソース全体を自身のフォーマットで見る。
			D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
			unorderedAccessViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
			unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			unorderedAccessViewDesc.Texture3D.WSize = size;

			/// [EN] Reserve a bindless slot and write the write view into it.
			/// [JP] bindless のスロットを確保し、書き込み用ビューを入れる。
			unorderedAccessViewIndex = bindlessHeap->AllocateIndex();
			device->CreateUnorderedAccessView(resource.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap->CPUHandle(unorderedAccessViewIndex));

			/// [EN] Reserve a bindless slot for the read view.
			/// [JP] 読み取り用ビューのための bindless のスロットを確保する。
			shaderResourceViewIndex = bindlessHeap->AllocateIndex();
			/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
			/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R16_FLOAT;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Texture3D.MipLevels = 1;
			device->CreateShaderResourceView(resource.Get(), &shaderResourceViewDesc, bindlessHeap->CPUHandle(shaderResourceViewIndex));
		};

		/// [EN] The noise volumes have fixed sizes and are baked once, so they are created here and never resized.
		/// [JP] ノイズボリュームは固定サイズで 1 度だけ焼き込むため、ここで作り、サイズ変更では作り直さない。
		createNoiseVolume(shapeNoiseSize_, shapeNoiseResource_, shapeNoiseUnorderedAccessViewIndex_, shapeNoiseShaderResourceViewIndex_);
		createNoiseVolume(detailNoiseSize_, detailNoiseResource_, detailNoiseUnorderedAccessViewIndex_, detailNoiseShaderResourceViewIndex_);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Replaces the cloud texture with one for the new render size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 雲テクスチャを新しいレンダーサイズのものに置き換える。
	*/
	void VolumetricCloudScapesRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
	{
		/// [EN] Give up the resources of the old size first.
		/// [JP] 先に古いサイズのリソースを手放す。
		Release();

		/// [EN] Same rounded-up reduction as in Create.
		/// [JP] Create と同じく切り上げて縮小する。
		width_ = (width + resolutionDivisor_ - 1) / resolutionDivisor_;
		height_ = (height + resolutionDivisor_ - 1) / resolutionDivisor_;

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Uploads the tuning values with the renderer-owned fields filled in,
	* stores the enabled flag for Dispatch, and publishes the cloud texture
	* and noise volume views to the index systems.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レンダラーが持つフィールドを埋めた調整値をアップロードし、Dispatch 用に
	* 有効フラグを保存して、雲テクスチャとノイズボリュームのビューを各
	* インデックスシステムへ公開する。
	*/
	void VolumetricCloudScapesRenderer::Prepare(const VolumetricCloudScapesRayConstantBuffer& settings, Bool enabled)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;

		/// [EN] The upload copy carries the frame counter and the enabled flag on top of the user's tuning.
		/// [JP] アップロード用のコピーには、ユーザーの調整値に加えてフレームカウンターと有効フラグを載せる。
		VolumetricCloudScapesRayConstantBuffer uploadSettings = settings;
		uploadSettings.frameIndex_ = frameIndex_;
		uploadSettings.proceduralSkyEnabled_ = enabled ? 1 : 0;
		/// [EN] Advance the counter for the next frame.
		/// [JP] 次のフレームのためにカウンターを進める。
		frameIndex_++;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);

		/// [EN] The raymarch writes the cloud texture and deferred lighting reads it; the noise volumes are written by the bake and read by the raymarch.
		/// [JP] 雲テクスチャはレイマーチが書いてディファードライティングが読む。ノイズボリュームは焼き込みが書いてレイマーチが読む。
		constantIndicesSystem_->SetCloudRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetCloudOutputUnorderedAccessViewIndex(cloudUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetCloudOutputShaderResourceViewIndex(cloudShaderResourceViewIndex_);
		unorderedAccessIndicesSystem_->SetCloudShapeNoiseUnorderedAccessViewIndex(shapeNoiseUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetCloudShapeNoiseShaderResourceViewIndex(shapeNoiseShaderResourceViewIndex_);
		unorderedAccessIndicesSystem_->SetCloudDetailNoiseUnorderedAccessViewIndex(detailNoiseUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetCloudDetailNoiseShaderResourceViewIndex(detailNoiseShaderResourceViewIndex_);
	}

	/**
	* [EN]
	* Writes this view's clouds: raymarched when enabled_ and the pipeline
	* exist (baking the noise first if needed), cleared to 0 otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このビューの雲を書き込む。enabled_ でパイプラインがあればレイマーチし
	* （必要なら先にノイズを焼き込む）、そうでなければ 0 でクリアする。
	*/
	void VolumetricCloudScapesRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] Both the raymarch and the clear write the texture, so it must be in unordered-access state first.
		/// [JP] レイマーチもクリアもテクスチャへ書き込むため、先に unordered-access 状態にする。
		if (cloudState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(cloudResource_.Get(), cloudState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cloudState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		/// [EN] Report a missing pipeline once.
		/// [JP] パイプラインが無いことを 1 度だけ報告する。
		ID3D12PipelineState* pipelineState = cloudShader_.GetPipelineState();
		if (!pipelineState && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("VolumetricCloudScapesRT のコンピュート PSO 作成に失敗しています。雲は常に無し(0)として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		if (!enabled_ || !pipelineState)
		{
			/// [EN] Zero coverage composites nothing over the sky.
			/// [JP] カバレッジ 0 なら、空の上には何も合成されない。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(cloudUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearIndex_), cloudResource_.Get(), clearValues, 0, nullptr);
		}
		else
		{
			/// [EN] The shaders read every input through the bindless heap and the shared root addresses.
			/// [JP] シェーダーはすべての入力を bindless ヒープと共有ルートアドレス経由で読む。
			ID3D12DescriptorHeap* heaps[] = { heap };
			/// [EN] Make the bindless heap the active shader-visible heap.
			/// [JP] bindless ヒープを有効なシェーダー可視ヒープにする。
			cmd->SetDescriptorHeaps(_countof(heaps), heaps);
			/// [EN] All passes share one root signature; set it on the compute binding point.
			/// [JP] すべてのパスは 1 つのルートシグネチャを共有する。コンピュートのバインドポイントに設定する。
			cmd->SetComputeRootSignature(cloudShader_.GetRootSignature());
			/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
			/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
			RootSignature::BindCompute(cmd, addresses);

			/// [EN] Bake both noise volumes once, then keep them as shader resources for every later raymarch.
			/// [JP] 両方のノイズボリュームを 1 度だけ焼き込み、以後のレイマーチのためにシェーダーリソースのままにする。
			if (!noiseBaked_)
			{
				ID3D12PipelineState* shapeBakePipeline = cloudShader_.GetShapeBakePipelineState();
				ID3D12PipelineState* detailBakePipeline = cloudShader_.GetDetailBakePipelineState();

				if (shapeBakePipeline && detailBakePipeline)
				{
					cmdList->Barrier(shapeNoiseResource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					cmdList->Barrier(detailNoiseResource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

					/// [EN] One 4x4x4 thread group per 4x4x4 texel block.
					/// [JP] 4x4x4 テクセルのブロックごとに 4x4x4 のスレッドグループを 1 つ。
					cmd->SetPipelineState(shapeBakePipeline);
					cmd->Dispatch(shapeNoiseSize_ / 4, shapeNoiseSize_ / 4, shapeNoiseSize_ / 4);

					cmd->SetPipelineState(detailBakePipeline);
					cmd->Dispatch(detailNoiseSize_ / 4, detailNoiseSize_ / 4, detailNoiseSize_ / 4);

					cmdList->Barrier(shapeNoiseResource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
					cmdList->Barrier(detailNoiseResource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

					/// [EN] Mark it done, so this happens only once.
					/// [JP] 済んだ印を付け、1 度だけ行うようにする。
					noiseBaked_ = true;
				}
			}

			cmd->SetPipelineState(pipelineState);

			/// [EN] One 8x8 thread group per 8x8 pixel tile of the cloud texture, rounded up to cover the edges.
			/// [JP] 雲テクスチャの 8x8 ピクセルのタイルごとに 8x8 のスレッドグループを 1 つ。端まで覆うよう切り上げる。
			Uint32 groupCountX = (width_ + 7) / 8;
			Uint32 groupCountY = (height_ + 7) / 8;
			cmd->Dispatch(groupCountX, groupCountY, 1);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();
		}

		/// [EN] Hand the texture over to deferred lighting as a shader resource.
		/// [JP] テクスチャをシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(cloudResource_.Get(), cloudState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		cloudState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the width_ x height_ RGBA16 cloud texture with its bindless
	* write view, its clear view and its bindless read view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の RGBA16 雲テクスチャと、bindless の書き込み用ビュー、
	* クリア用ビュー、bindless の読み取り用ビューを作成する。
	*/
	void VolumetricCloudScapesRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] HDR in-scattered radiance plus coverage, stored as 16-bit float RGBA.
		/// [JP] HDR の内散乱の放射輝度とカバレッジを、16 ビット浮動小数点の RGBA で保持する。
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
		resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		/// [EN] Create the texture in its own GPU-local heap, starting in COMMON state.
		/// [JP] テクスチャを専用の GPU ローカルなヒープに、COMMON 状態で作る。
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&cloudResource_));
		SC_HR_CHECK(hr, "クラウドリソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		cloudResource_->SetName(L"VolumetricCloudScapes_Cloud");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(cloudResource_.Get());
#endif
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		cloudState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] The write view is created twice: once shader-visible for the raymarch, once CPU-side for the clear.
		/// [JP] 書き込み用ビューは 2 つ作る。レイマーチ用のシェーダー可視のものと、クリア用の CPU 側のもの。
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		cloudUnorderedAccessViewIndex_ = bindlessHeap_->AllocateIndex();
		device->CreateUnorderedAccessView(cloudResource_.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(cloudUnorderedAccessViewIndex_));

		/// [EN] Recreate the CPU-side heap that holds the clear views, sized for every clearable resource.
		/// [JP] クリア用ビューを置く CPU 側のヒープを、クリアするリソースの数だけ作り直す。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
		clearIndex_ = clearHeap_.AllocateIndex();
		device->CreateUnorderedAccessView(cloudResource_.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(clearIndex_));

		/// [EN] The read view lets deferred lighting sample the texture.
		/// [JP] 読み取り用ビューで、ディファードライティングがテクスチャをサンプルできる。
		cloudShaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();
		/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
		/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(cloudResource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(cloudShaderResourceViewIndex_));
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
	void VolumetricCloudScapesRenderer::Release()
	{
		/// [EN] Return the view slots to the bindless heap.
		/// [JP] ビューのスロットを bindless ヒープへ返す。
		bindlessHeap_->FreeIndex(cloudUnorderedAccessViewIndex_);
		bindlessHeap_->FreeIndex(cloudShaderResourceViewIndex_);

		/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
		/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
		bindlessHeap_->DeferRelease(cloudResource_);
		cloudResource_.Reset();
	}
}

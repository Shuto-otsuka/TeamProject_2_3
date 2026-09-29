#include <GraphicsEngine/Renderer/VolumetricStarRenderer.h>

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
	* Binds the shared root signature and pipeline-state cache to the star
	* shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとパイプラインステートキャッシュを、星シェーダーへ
	* 関連付ける。
	*/
	VolumetricStarRenderer::VolumetricStarRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : starShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once, then allocates the
	* size-dependent star texture.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクトを 1 度だけ作成し、その後サイズに依存する
	* 星テクスチャを確保する。
	*/
	void VolumetricStarRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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
		starShader_.Create(shaderCache, device);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<VolumetricStarRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);
	}

	/**
	* [EN]
	* Replaces the star texture with one of the new size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 星テクスチャを新しいサイズのものに置き換える。
	*/
	void VolumetricStarRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	* Steps the shooting-star simulation by deltaTime, then uploads the
	* tuning values together with the live slots and the enabled flag.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 流れ星のシミュレーションを deltaTime だけ進め、動いているスロットと
	* 有効フラグを入れた調整値をアップロードする。
	*/
	void VolumetricStarRenderer::Prepare(const VolumetricStarRayConstantBuffer& settings, Bool enabled, Float deltaTime, Float nightFactor)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;

		/// [EN] Uniform random value in [minValue, maxValue] drawn from randomEngine_.
		/// [JP] randomEngine_ から引く、[minValue, maxValue] の一様乱数。
		auto randomRange = [this](Float minValue, Float maxValue)
		{
			std::uniform_real_distribution<Float> distribution(minValue, maxValue);
			return distribution(randomEngine_);
		};

		/// [EN] Move every flying star along its streak; a star that reaches the end frees its slot (progress 0).
		/// [JP] 飛んでいる流れ星をすべて筋に沿って進める。終点に達した流れ星はスロットを空ける（progress 0）。
		for (Uint32 slot = 0; slot < volumetricStarMaxShootingStars_; slot++)
		{
			if (shootingStars_[slot].progress_ <= 0.0f)
			{
				continue;
			}

			shootingStars_[slot].progress_ += deltaTime / Max(shootingStarDuration_[slot], 0.01f);
			if (shootingStars_[slot].progress_ >= 1.0f)
			{
				shootingStars_[slot].progress_ = 0.0f;
			}
		}

		/// [EN] At night, roll once per frame for a new star in the first free slot; the chance scales with how dark it is and with the frame time.
		/// [JP] 夜の間は、毎フレーム 1 回だけ最初の空きスロットに新しい流れ星を出すか抽選する。確率は暗さとフレーム時間に比例する。
		if (enabled && nightFactor > 0.0f)
		{
			for (Uint32 slot = 0; slot < volumetricStarMaxShootingStars_; slot++)
			{
				if (shootingStars_[slot].progress_ > 0.0f)
				{
					continue;
				}

				Float chance = settings.shootingStarChancePerSecond_ * nightFactor * deltaTime;
				if (randomRange(0.0f, 1.0f) < chance)
				{
					/// [EN] The start is a random direction above the horizon.
					/// [JP] 始点は地平線より上のランダムな方向。
					Vector3 start = Vector3(randomRange(-1.0f, 1.0f), randomRange(0.2f, 1.0f), randomRange(-1.0f, 1.0f));
					start.Normalize();

					/// [EN] The end is the start rotated 50-110 degrees about a random axis, which guarantees a long arc across the sky; two independent random points would often land close together.
					/// [JP] 終点は始点をランダムな軸まわりに 50 から 110 度回転させたもので、空を大きく横切る弧になる。独立な 2 点だと近い位置に重なりやすい。
					Vector3 randomVector = Vector3(randomRange(-1.0f, 1.0f), randomRange(-1.0f, 1.0f), randomRange(-1.0f, 1.0f));
					Vector3 axis = start.Cross(randomVector);
					if (axis.LengthSquared() < 0.0001f)
					{
						axis = Vector3(1.0f, 0.0f, 0.0f);
					}
					axis.Normalize();

					Float travelAngle = DirectX::XMConvertToRadians(randomRange(50.0f, 110.0f));
					Vector3 end = Vector3::Transform(start, Matrix::CreateFromAxisAngle(axis, travelAngle));

					/// [EN] Keep the end above the horizon so the whole streak stays in the sky.
					/// [JP] 筋全体が空に収まるよう、終点を地平線より上に保つ。
					if (end.y < 0.1f)
					{
						end.y = 0.1f;
					}
					end.Normalize();

					/// [EN] A tiny positive progress marks the slot as flying; each star gets its own brightness and a 1-2 second flight.
					/// [JP] ごく小さい正の progress でスロットを飛行中にする。流れ星ごとに明るさと 1 から 2 秒の飛行時間を決める。
					shootingStars_[slot].startDirection_ = start;
					shootingStars_[slot].endDirection_ = end;
					shootingStars_[slot].progress_ = 0.0001f;
					shootingStars_[slot].brightness_ = randomRange(0.6f, 1.0f);
					shootingStarDuration_[slot] = randomRange(1.0f, 2.0f);
				}

				break;
			}
		}

		/// [EN] The upload copy carries the renderer-owned values - enabled flag and live slots - on top of the user's tuning.
		/// [JP] アップロード用のコピーには、ユーザーの調整値に加えて、レンダラーが持つ値（有効フラグと動いているスロット）を載せる。
		VolumetricStarRayConstantBuffer uploadSettings = settings;
		uploadSettings.enabled_ = enabled ? 1 : 0;
		for (Uint32 slot = 0; slot < volumetricStarMaxShootingStars_; slot++)
		{
			uploadSettings.activeShootingStars_[slot] = shootingStars_[slot];
		}

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(uploadSettings);

		/// [EN] The star pass writes through the write view; deferred lighting reads through the read view.
		/// [JP] 星パスは書き込み用ビューで書き、ディファードライティングは読み取り用ビューで読む。
		constantIndicesSystem_->SetStarRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetStarOutputUnorderedAccessViewIndex(starUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetStarOutputShaderResourceViewIndex(starShaderResourceViewIndex_);
	}

	/**
	* [EN]
	* Writes this view's stars: rendered when enabled_ and the pipeline
	* exist, cleared to 0 otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このビューの星を書き込む。enabled_ でパイプラインがあれば描画し、
	* そうでなければ 0 でクリアする。
	*/
	void VolumetricStarRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] Both the render and the clear write the texture, so it must be in unordered-access state first.
		/// [JP] 描画もクリアもテクスチャへ書き込むため、先に unordered-access 状態にする。
		if (starState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(starResource_.Get(), starState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			starState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		/// [EN] Report a missing pipeline once.
		/// [JP] パイプラインが無いことを 1 度だけ報告する。
		ID3D12PipelineState* pipelineState = starShader_.GetPipelineState();
		if (!pipelineState && !pipelineStateMissingLogged_)
		{
			SC_LOG_WARNING("VolumetricStarRT のコンピュート PSO 作成に失敗しています。星は常に無し(0)として扱われます。");
			pipelineStateMissingLogged_ = true;
		}

		if (!enabled_ || !pipelineState)
		{
			/// [EN] Zero coverage composites nothing over the sky.
			/// [JP] カバレッジ 0 なら、空の上には何も合成されない。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(starUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearIndex_), starResource_.Get(), clearValues, 0, nullptr);
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
			cmd->SetComputeRootSignature(starShader_.GetRootSignature());
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
		cmdList->Barrier(starResource_.Get(), starState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		starState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the width_ x height_ RGBA16 star texture with its bindless
	* write view, its clear view and its bindless read view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の RGBA16 星テクスチャと、bindless の書き込み用ビュー、
	* クリア用ビュー、bindless の読み取り用ビューを作成する。
	*/
	void VolumetricStarRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] HDR premultiplied color plus coverage, stored as 16-bit float RGBA.
		/// [JP] HDR の事前乗算済みの色とカバレッジを、16 ビット浮動小数点の RGBA で保持する。
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
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&starResource_));
		SC_HR_CHECK(hr, "スターリソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		starResource_->SetName(L"VolumetricStar");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(starResource_.Get());
#endif
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		starState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] The write view is created twice: once shader-visible for the render, once CPU-side for the clear.
		/// [JP] 書き込み用ビューは 2 つ作る。描画用のシェーダー可視のものと、クリア用の CPU 側のもの。
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		starUnorderedAccessViewIndex_ = bindlessHeap_->AllocateIndex();
		device->CreateUnorderedAccessView(starResource_.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(starUnorderedAccessViewIndex_));

		/// [EN] Recreate the CPU-side heap that holds the clear views, sized for every clearable resource.
		/// [JP] クリア用ビューを置く CPU 側のヒープを、クリアするリソースの数だけ作り直す。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
		clearIndex_ = clearHeap_.AllocateIndex();
		device->CreateUnorderedAccessView(starResource_.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(clearIndex_));

		/// [EN] The read view lets deferred lighting sample the texture.
		/// [JP] 読み取り用ビューで、ディファードライティングがテクスチャをサンプルできる。
		starShaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();
		/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
		/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(starResource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(starShaderResourceViewIndex_));
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
	void VolumetricStarRenderer::Release()
	{
		/// [EN] Return the view slots to the bindless heap.
		/// [JP] ビューのスロットを bindless ヒープへ返す。
		bindlessHeap_->FreeIndex(starUnorderedAccessViewIndex_);
		bindlessHeap_->FreeIndex(starShaderResourceViewIndex_);

		/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
		/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
		bindlessHeap_->DeferRelease(starResource_);
		starResource_.Reset();
	}
}

#include <GraphicsEngine/Renderer/RefractionRenderer.h>

#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Log/Warning.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the shared root signature and raytracing-state cache to the
	* refraction shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有のルートシグネチャとレイトレーシングステートキャッシュを、屈折
	* シェーダーへ関連付ける。
	*/
	RefractionRenderer::RefractionRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject) : refractionShader_(rootSignature, raytracingStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the size-independent objects once - pipeline, tuning buffer
	* and shader table - then allocates the size-dependent output texture.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サイズに依存しないオブジェクト（パイプライン、調整値のバッファ、
	* シェーダーテーブル）を 1 度だけ作成し、その後サイズに依存する出力
	* テクスチャを確保する。
	*/
	void RefractionRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
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
		refractionShader_.Create(shaderCache, device5);

		/// [EN] The tuning values live in a constant buffer the shaders find through its bindless index.
		/// [JP] 調整値は、シェーダーが bindless インデックスで見つける定数バッファに置く。
		tuningBuffer_ = MakePtr<ConstantBuffer<RefractionRayConstantBuffer>>(device, bindlessHeap);

		/// [EN] Create the size-dependent resources at the current size.
		/// [JP] サイズに依存するリソースを現在のサイズで作る。
		Allocate(device);

		/// [EN] Build the shader table. Only the ray-generation record is needed; the miss and hit-group tables stay empty.
		/// [JP] シェーダーテーブルを構築する。必要なのはレイ生成のレコードだけで、ミスとヒットグループのテーブルは空のままにする。
		ID3D12StateObject* stateObject = refractionShader_.GetStateObject();
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

		/// [EN] The shader identifier is what a shader-table record points DispatchRays at.
		/// [JP] シェーダー識別子は、シェーダーテーブルのレコードが DispatchRays に示す実行先である。
		void* rayGenIdentifier = stateObjectProperties->GetShaderIdentifier(String(RefractionShader::rayGenExportName).w_str().c_str());
		if (!rayGenIdentifier)
		{
			SC_LOG_ERROR("屈折RTPSOからシェーダ識別子(RefractionRayGeneration)を取得できませんでした");
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
		tableDesc.Width = shaderTableRecordSize_;
		tableDesc.Height = 1;
		tableDesc.DepthOrArraySize = 1;
		tableDesc.MipLevels = 1;
		tableDesc.Format = DXGI_FORMAT_UNKNOWN;
		tableDesc.SampleDesc.Count = 1;
		tableDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		/// [EN] Create the table buffer in the upload heap.
		/// [JP] テーブルのバッファをアップロードヒープに作る。
		hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &tableDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&shaderTableResource_));
		SC_HR_CHECK(hr, "屈折シェーダーテーブルリソースの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		shaderTableResource_->SetName(L"Refraction_ShaderTable");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(shaderTableResource_.Get());
#endif

		/// [EN] The record is the identifier followed by zeros up to the record size.
		/// [JP] レコードは識別子と、レコードサイズまでの 0 で構成する。
		Uint8* mapped = nullptr;
		hr = shaderTableResource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
		SC_HR_CHECK(hr, "屈折シェーダーテーブルリソースのMapに失敗しました");
		memset(mapped, 0, shaderTableRecordSize_);
		memcpy(mapped, rayGenIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		/// [EN] The table is written once, so unmap it right away.
		/// [JP] テーブルは 1 度書くだけなので、すぐに Unmap する。
		shaderTableResource_->Unmap(0, nullptr);
	}

	/**
	* [EN]
	* Replaces the output texture with one of the new size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 出力テクスチャを新しいサイズのものに置き換える。
	*/
	void RefractionRenderer::Resize(ID3D12Device* device, Uint32 width, Uint32 height)
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
	* publishes the constant buffer and output views to the index systems.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 調整値をアップロードし、Dispatch 用に有効フラグを保存して、定数バッファと
	* 出力テクスチャのビューを各インデックスシステムへ公開する。
	*/
	void RefractionRenderer::Prepare(const RefractionRayConstantBuffer& settings, Bool enabled)
	{
		/// [EN] Remember whether the pass runs this frame; Dispatch reads it.
		/// [JP] 今フレームにパスを実行するかを覚えておく。Dispatch が読む。
		enabled_ = enabled;

		/// [EN] Copy the tuning values into this frame's constant buffer.
		/// [JP] 調整値を今フレームの定数バッファへ写す。
		tuningBuffer_->Update(settings);

		/// [EN] The ray pass writes through the write view; deferred lighting reads through the read view.
		/// [JP] レイパスは書き込み用ビューで書き、ディファードライティングは読み取り用ビューで読む。
		constantIndicesSystem_->SetRefractionRayConstantIndex(tuningBuffer_->GetIndex());
		unorderedAccessIndicesSystem_->SetRefractionOutputUnorderedAccessViewIndex(outputUnorderedAccessViewIndex_);
		shaderResourceIndicesSystem_->SetRefractionOutputShaderResourceViewIndex(outputShaderResourceViewIndex_);
	}

	/**
	* [EN]
	* Writes this view's refraction: traced when enabled_ and the pipeline
	* and shader table exist, cleared to 0 otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このビューの屈折を書き込む。enabled_ でパイプラインとシェーダーテーブルが
	* あればトレースし、そうでなければ 0 でクリアする。
	*/
	void RefractionRenderer::Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		/// [EN] The raw command list, for the calls D3D12CommandList does not wrap.
		/// [JP] D3D12CommandList が包んでいない呼び出しに使う、生のコマンドリスト。
		ID3D12GraphicsCommandList6* cmd = cmdList->Get();

		/// [EN] The pipeline or table is missing when the GPU lacks DispatchRays support; report it once.
		/// [JP] GPU が DispatchRays に非対応だとパイプラインかテーブルが無い。1 度だけ報告する。
		ID3D12StateObject* stateObject = refractionShader_.GetStateObject();
		if ((!stateObject || !shaderTableResource_) && !stateObjectMissingLogged_)
		{
			SC_LOG_WARNING("RefractionRT の RTPSO/シェーダテーブル作成に失敗しています。DXR(DispatchRays)非対応の可能性があります。屈折は常に無し(0)として扱われます。");
			stateObjectMissingLogged_ = true;
		}

		/// [EN] Both the trace and the clear write the texture, so it must be in unordered-access state first.
		/// [JP] トレースもクリアもテクスチャへ書き込むため、先に unordered-access 状態にする。
		if (outputState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(outputResource_.Get(), outputState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			outputState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		if (!enabled_ || !stateObject || !shaderTableResource_)
		{
			/// [EN] Without a trace, 0 means "no refracted light", which leaves deferred lighting unchanged.
			/// [JP] トレースしない場合、0 は「屈折光なし」を意味し、ディファードライティングを変化させない。
			const Float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			cmd->ClearUnorderedAccessViewFloat(bindlessHeap_->GPUHandle(outputUnorderedAccessViewIndex_), clearHeap_.CPUHandle(clearOutputIndex_), outputResource_.Get(), clearValues, 0, nullptr);
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
			cmd->SetComputeRootSignature(refractionShader_.GetRootSignature());
			/// [EN] Bind the shared per-frame root arguments (constant and index buffers).
			/// [JP] フレーム共通のルート引数（定数とインデックスのバッファ）をバインドする。
			RootSignature::BindCompute(cmd, addresses);
			/// [EN] Raytracing uses a state object instead of a pipeline state.
			/// [JP] レイトレーシングはパイプラインステートの代わりにステートオブジェクトを使う。
			cmd->SetPipelineState1(stateObject);

			/// [EN] One ray per pixel; only the ray-generation record is set, the miss and hit-group tables stay zero.
			/// [JP] 1 ピクセルにつきレイ 1 本。設定するのはレイ生成のレコードだけで、ミスとヒットグループのテーブルは 0 のまま。
			D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
			dispatchDesc.RayGenerationShaderRecord.StartAddress = shaderTableResource_->GetGPUVirtualAddress();
			dispatchDesc.RayGenerationShaderRecord.SizeInBytes = shaderTableRecordSize_;
			dispatchDesc.Width = width_;
			dispatchDesc.Height = height_;
			dispatchDesc.Depth = 1;

			/// [EN] Launch the rays.
			/// [JP] レイを起動する。
			cmd->DispatchRays(&dispatchDesc);
			/// [EN] Count the dispatch in the profiler's statistics.
			/// [JP] このディスパッチをプロファイラーの統計に数える。
			ProfilerStats::AddDrawCall();
		}

		/// [EN] Hand the texture over to deferred lighting as a shader resource.
		/// [JP] テクスチャをシェーダーリソースとしてディファードライティングへ渡す。
		cmdList->Barrier(outputResource_.Get(), outputState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		outputState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	}

	/**
	* [EN]
	* Creates the width_ x height_ RGBA16 output texture with its bindless
	* write view, its clear view and its bindless read view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_ x height_ の RGBA16 出力テクスチャと、bindless の書き込み用ビュー、
	* クリア用ビュー、bindless の読み取り用ビューを作成する。
	*/
	void RefractionRenderer::Allocate(ID3D12Device* device)
	{
		/// [EN] Refracted radiance is HDR color, so it is stored as 16-bit float RGBA.
		/// [JP] 屈折後の放射輝度は HDR の色なので、16 ビット浮動小数点の RGBA で保持する。
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
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outputResource_));
		SC_HR_CHECK(hr, "屈折放射輝度テクスチャの生成に失敗しました");
		/// [EN] In debug builds, name it so it can be identified in PIX and crash dumps.
		/// [JP] デバッグビルドでは、PIX やクラッシュダンプで見分けられるよう名前を付ける。
#ifdef _DEBUG
		outputResource_->SetName(L"Refraction_Output");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(outputResource_.Get());
#endif
		/// [EN] Track the state it was created in, so the first barrier starts from the right state.
		/// [JP] 作成時の状態を記録し、最初のバリアが正しい状態から始まるようにする。
		outputState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] The write view is created twice: once shader-visible for the trace, once CPU-side for the clear.
		/// [JP] 書き込み用ビューは 2 つ作る。トレース用のシェーダー可視のものと、クリア用の CPU 側のもの。
		D3D12_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		unorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

		outputUnorderedAccessViewIndex_ = bindlessHeap_->AllocateIndex();
		device->CreateUnorderedAccessView(outputResource_.Get(), nullptr, &unorderedAccessViewDesc, bindlessHeap_->CPUHandle(outputUnorderedAccessViewIndex_));

		/// [EN] Recreate the CPU-side heap that holds the clear views, sized for every clearable resource.
		/// [JP] クリア用ビューを置く CPU 側のヒープを、クリアするリソースの数だけ作り直す。
		clearHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, false);
		clearOutputIndex_ = clearHeap_.AllocateIndex();
		device->CreateUnorderedAccessView(outputResource_.Get(), nullptr, &unorderedAccessViewDesc, clearHeap_.CPUHandle(clearOutputIndex_));

		/// [EN] The read view lets deferred lighting sample the texture.
		/// [JP] 読み取り用ビューで、ディファードライティングがテクスチャをサンプルできる。
		outputShaderResourceViewIndex_ = bindlessHeap_->AllocateIndex();
		/// [EN] The read view sees the whole resource in its own format, with the channels unchanged.
		/// [JP] 読み取り用ビューは、リソース全体を自身のフォーマットで、チャンネルをそのまま見る。
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(outputResource_.Get(), &shaderResourceViewDesc, bindlessHeap_->CPUHandle(outputShaderResourceViewIndex_));
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
	void RefractionRenderer::Release()
	{
		/// [EN] Return the view slots to the bindless heap.
		/// [JP] ビューのスロットを bindless ヒープへ返す。
		bindlessHeap_->FreeIndex(outputUnorderedAccessViewIndex_);
		bindlessHeap_->FreeIndex(outputShaderResourceViewIndex_);

		/// [EN] Keep the resource alive until the GPU has finished with it; then drop this reference.
		/// [JP] GPU が使い終えるまでリソースを生かしておき、その後この参照を手放す。
		bindlessHeap_->DeferRelease(outputResource_);
		outputResource_.Reset();
	}
}

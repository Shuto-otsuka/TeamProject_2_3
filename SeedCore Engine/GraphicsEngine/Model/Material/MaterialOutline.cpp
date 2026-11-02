#include <GraphicsEngine/Model/Material/MaterialOutline.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/PipelineState/RasterizerState.h>
#include <GraphicsEngine/D3D12/PipelineState/BlendState.h>
#include <GraphicsEngine/D3D12/PipelineState/DepthStencilState.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the native-resolution resolve texture and transparent record,
	* the max-width buffer and the four pipeline states. width/height are
	* the native (VisibilityBuffer) resolution.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ネイティブ解像度の resolve テクスチャと半透明の記録、最大幅の
	* バッファ、4 つのパイプラインステートを作る。width/height は
	* ネイティブ(VisibilityBuffer)の解像度。
	*/
	void MaterialOutline::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, Uint32 width, Uint32 height)
	{
		HRESULT hr{ S_OK };

		width_ = width;
		height_ = height;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		/// [EN] Resolve texture: one material outline (packed color, width bits) per native pixel.
		/// [JP] resolve テクスチャ。ネイティブの 1 ピクセルにつき、マテリアルの縁取り 1 つ(詰めた色、幅のビット列)。
		D3D12_RESOURCE_DESC resolveDesc{};
		resolveDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resolveDesc.Width = width;
		resolveDesc.Height = height;
		resolveDesc.DepthOrArraySize = 1;
		resolveDesc.MipLevels = 1;
		resolveDesc.Format = DXGI_FORMAT_R32G32_UINT;
		resolveDesc.SampleDesc.Count = 1;
		resolveDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resolveDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resolveResource_));
		SC_HR_CHECK(hr, "マテリアルのアウトラインの resolve テクスチャの生成に失敗しました");
#if SC_DEBUG
		resolveResource_->SetName(L"MaterialOutline_Resolve");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(resolveResource_.Get());
#endif
		resolveState_ = D3D12_RESOURCE_STATE_COMMON;

		resolveUnorderedAccessViewIndex_ = bindlessHeap->AllocateIndex();
		D3D12_UNORDERED_ACCESS_VIEW_DESC resolveUnorderedAccessViewDesc{};
		resolveUnorderedAccessViewDesc.Format = DXGI_FORMAT_R32G32_UINT;
		resolveUnorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		device->CreateUnorderedAccessView(resolveResource_.Get(), nullptr, &resolveUnorderedAccessViewDesc, bindlessHeap->CPUHandle(resolveUnorderedAccessViewIndex_));

		resolveShaderResourceViewIndex_ = bindlessHeap->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC resolveShaderResourceViewDesc{};
		resolveShaderResourceViewDesc.Format = DXGI_FORMAT_R32G32_UINT;
		resolveShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		resolveShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		resolveShaderResourceViewDesc.Texture2D.MipLevels = 1;
		device->CreateShaderResourceView(resolveResource_.Get(), &resolveShaderResourceViewDesc, bindlessHeap->CPUHandle(resolveShaderResourceViewIndex_));

		/// [EN] Transparent record: the nearest outlined transparent fragment per native pixel. Created straight in UNORDERED_ACCESS, the only state it is ever used in.
		/// [JP] 半透明の記録。ネイティブの 1 ピクセルにつき、一番手前の縁取りのある半透明のフラグメント。使うのは UNORDERED_ACCESS の状態だけなので、最初からその状態で作る。
		D3D12_RESOURCE_DESC transparentDesc{};
		transparentDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		transparentDesc.Width = width;
		transparentDesc.Height = height;
		transparentDesc.DepthOrArraySize = 1;
		transparentDesc.MipLevels = 1;
		transparentDesc.Format = DXGI_FORMAT_R32_UINT;
		transparentDesc.SampleDesc.Count = 1;
		transparentDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &transparentDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&transparentResource_));
		SC_HR_CHECK(hr, "マテリアルのアウトラインの半透明の記録の生成に失敗しました");
#if SC_DEBUG
		transparentResource_->SetName(L"MaterialOutline_Transparent");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(transparentResource_.Get());
#endif

		transparentUnorderedAccessViewIndex_ = bindlessHeap->AllocateIndex();
		D3D12_UNORDERED_ACCESS_VIEW_DESC transparentUnorderedAccessViewDesc{};
		transparentUnorderedAccessViewDesc.Format = DXGI_FORMAT_R32_UINT;
		transparentUnorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		device->CreateUnorderedAccessView(transparentResource_.Get(), nullptr, &transparentUnorderedAccessViewDesc, bindlessHeap->CPUHandle(transparentUnorderedAccessViewIndex_));

		/// [EN] Max-width buffer: a single uint holding asuint() of the widest outline on screen.
		/// [JP] 最大幅のバッファ。画面上で一番太い縁取りの asuint() を持つ uint 1 つ。
		D3D12_RESOURCE_DESC maxWidthDesc{};
		maxWidthDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		maxWidthDesc.Width = sizeof(Uint32);
		maxWidthDesc.Height = 1;
		maxWidthDesc.DepthOrArraySize = 1;
		maxWidthDesc.MipLevels = 1;
		maxWidthDesc.Format = DXGI_FORMAT_UNKNOWN;
		maxWidthDesc.SampleDesc.Count = 1;
		maxWidthDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		maxWidthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &maxWidthDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&maxWidthResource_));
		SC_HR_CHECK(hr, "マテリアルのアウトラインの最大幅バッファの生成に失敗しました");
#if SC_DEBUG
		maxWidthResource_->SetName(L"MaterialOutline_MaxWidth");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(maxWidthResource_.Get());
#endif
		maxWidthState_ = D3D12_RESOURCE_STATE_COMMON;

		maxWidthUnorderedAccessViewIndex_ = bindlessHeap->AllocateIndex();
		D3D12_UNORDERED_ACCESS_VIEW_DESC maxWidthUnorderedAccessViewDesc{};
		maxWidthUnorderedAccessViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		maxWidthUnorderedAccessViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		maxWidthUnorderedAccessViewDesc.Buffer.NumElements = 1;
		maxWidthUnorderedAccessViewDesc.Buffer.StructureByteStride = sizeof(Uint32);
		device->CreateUnorderedAccessView(maxWidthResource_.Get(), nullptr, &maxWidthUnorderedAccessViewDesc, bindlessHeap->CPUHandle(maxWidthUnorderedAccessViewIndex_));

		maxWidthShaderResourceViewIndex_ = bindlessHeap->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC maxWidthShaderResourceViewDesc{};
		maxWidthShaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		maxWidthShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		maxWidthShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		maxWidthShaderResourceViewDesc.Buffer.NumElements = 1;
		maxWidthShaderResourceViewDesc.Buffer.StructureByteStride = sizeof(Uint32);
		device->CreateShaderResourceView(maxWidthResource_.Get(), &maxWidthShaderResourceViewDesc, bindlessHeap->CPUHandle(maxWidthShaderResourceViewIndex_));

		constantBuffer_ = MakePtr<StaticConstantBuffer<MaterialOutlineConstantBuffer>>(device, bindlessHeap);

		Handle<RootSignature> rootSignatureHandle = rootSignature.GetOrCreate(device);
		rootSignature_ = rootSignature.Get(rootSignatureHandle);

		/// [EN] Compute pipelines: empty the transparent record, clear the max width, then resolve each pixel's material outline.
		/// [JP] コンピュートのパイプライン。半透明の記録を空にし、最大幅を 0 に戻し、その後各ピクセルのマテリアルの縁取りを求める。
		clearTransparentShader_ = shaderCache.GetOrCreateComputeShader(String("../GraphicsEngine/Model/Material/MaterialOutlineCS.hlsl"), String("ClearTransparent"));
		resetMaxWidthShader_ = shaderCache.GetOrCreateComputeShader(String("../GraphicsEngine/Model/Material/MaterialOutlineCS.hlsl"), String("ResetMaxWidth"));
		resolveShader_ = shaderCache.GetOrCreateComputeShader(String("../GraphicsEngine/Model/Material/MaterialOutlineCS.hlsl"), String("Resolve"));

		PipelineStateKey computeKey{};
		memset(&computeKey, 0, sizeof(computeKey));
		computeKey.rootSignature_ = rootSignature_->Get();
		computeKey.computeShader_ = shaderCache.GetComputeShader(clearTransparentShader_)->Bytecode();
		clearTransparentPipelineState_ = pipelineStateObject.Get(pipelineStateObject.GetOrCreate(device, computeKey));

		computeKey.computeShader_ = shaderCache.GetComputeShader(resetMaxWidthShader_)->Bytecode();
		resetMaxWidthPipelineState_ = pipelineStateObject.Get(pipelineStateObject.GetOrCreate(device, computeKey));

		computeKey.computeShader_ = shaderCache.GetComputeShader(resolveShader_)->Bytecode();
		resolvePipelineState_ = pipelineStateObject.Get(pipelineStateObject.GetOrCreate(device, computeKey));

		/// [EN] Composite pipeline: fullscreen triangle (the same mesh/vertex shaders the selection outline uses), alpha-blended onto the R8G8B8A8_UNORM display texture with no depth.
		/// [JP] 合成のパイプライン。フルスクリーンの三角形(選択アウトラインと同じメッシュ/頂点シェーダ)を、深度なしで R8G8B8A8_UNORM の表示用テクスチャへアルファブレンドする。
		compositePixelShader_ = shaderCache.GetOrCreatePixelShader(String("../GraphicsEngine/Model/Material/MaterialOutlinePS.hlsl"));

		PipelineStateKey compositeKey{};
		memset(&compositeKey, 0, sizeof(compositeKey));
		compositeKey.rootSignature_ = rootSignature_->Get();
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			compositeMeshShader_ = shaderCache.GetOrCreateMeshShader(String("../GraphicsEngine/Shape/Outline/OutlineMS.hlsl"));
			compositeKey.meshShader_ = shaderCache.GetMeshShader(compositeMeshShader_)->Bytecode();
			compositeKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}
		else
		{
			compositeVertexShader_ = shaderCache.GetOrCreateVertexShader(String("../GraphicsEngine/Shape/HUD/FullscreenVS.hlsl"));
			compositeKey.vertexShader_ = shaderCache.GetVertexShader(compositeVertexShader_)->Bytecode();
			compositeKey.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
		compositeKey.pixelShader_ = shaderCache.GetPixelShader(compositePixelShader_)->Bytecode();
		compositeKey.rasterizerDesc_ = RasterizerState::Get(RasterizerStateType::SolidNoneLHS);
		compositeKey.blendDesc_ = BlendState::Get(BlendStateType::Alpha);
		compositeKey.depthStencilDesc_ = DepthStencilState::Get(DepthStencilStateType::DepthOff);
		compositeKey.renderTargetViewFormat_[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		compositeKey.renderTargetViewCount_ = 1;
		compositeKey.depthStencilViewFormat_ = DXGI_FORMAT_UNKNOWN;
		compositePipelineState_ = pipelineStateObject.Get(pipelineStateObject.GetOrCreate(device, compositeKey));
	}

	/**
	* [EN]
	* Releases the resources and their bindless indices.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* リソースと、その bindless インデックスを解放する。
	*/
	void MaterialOutline::Destroy(BindlessHeap* bindlessHeap)
	{
		bindlessHeap->Release(std::move(resolveResource_), { resolveUnorderedAccessViewIndex_, resolveShaderResourceViewIndex_ });
		bindlessHeap->Release(std::move(transparentResource_), transparentUnorderedAccessViewIndex_);
		bindlessHeap->Release(std::move(maxWidthResource_), { maxWidthUnorderedAccessViewIndex_, maxWidthShaderResourceViewIndex_ });
		constantBuffer_ = nullptr;
	}

	/**
	* [EN]
	* Recreates the resources for a new native resolution.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 新しいネイティブ解像度に合わせてリソースを作り直す。
	*/
	void MaterialOutline::Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, Uint32 width, Uint32 height)
	{
		Destroy(bindlessHeap);
		Create(device, bindlessHeap, shaderCache, rootSignature, pipelineStateObject, width, height);
	}

	/**
	* [EN]
	* Empties the transparent record. Call before the transparent pass of
	* the view that Draw will resolve, so only this frame's fragments are in
	* it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 半透明の記録を空にする。Draw で解決するビューの半透明のパスより前に
	* 呼び、このフレームのフラグメントだけが入るようにする。
	*/
	void MaterialOutline::ClearTransparent(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		auto* cmd = cmdList->Get();

		MaterialOutlineConstantBuffer constants{};
		constants.resolveUnorderedAccessIndex_ = resolveUnorderedAccessViewIndex_;
		constants.resolveShaderResourceIndex_ = resolveShaderResourceViewIndex_;
		constants.maxWidthUnorderedAccessIndex_ = maxWidthUnorderedAccessViewIndex_;
		constants.maxWidthShaderResourceIndex_ = maxWidthShaderResourceViewIndex_;
		constants.transparentUnorderedAccessIndex_ = transparentUnorderedAccessViewIndex_;
		constantBuffer_->Update(constants);
		Uint dispatchBufferIndex = constantBuffer_->Index();

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetComputeRootSignature(rootSignature_->Get());
		RootSignature::BindCompute(cmd, addresses);
		cmd->SetComputeRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);

		cmd->SetPipelineState(clearTransparentPipelineState_.Get());
		cmd->Dispatch((width_ + 7) / 8, (height_ + 7) / 8, 1);

		/// [EN] The transparent pass writes into the record right after, so the clear must finish first.
		/// [JP] 直後に半透明のパスが記録へ書き込むので、先にクリアを終わらせる。
		D3D12_RESOURCE_BARRIER transparentBarrier{};
		transparentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		transparentBarrier.UAV.pResource = transparentResource_.Get();
		cmd->ResourceBarrier(1, &transparentBarrier);
	}

	/**
	* [EN]
	* Resolves this frame's material outlines and composites them onto
	* renderTargetView (the R8G8B8A8_UNORM tone-mapped display texture,
	* already in RENDER_TARGET). The caller must have the VisibilityBuffer
	* and depth shader-readable (GeometryBuffer::End / EndDepth) and
	* addresses must be the view whose GeometryBuffer and model instances
	* are being outlined.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このフレームのマテリアルの縁取りを求め、renderTargetView(既に
	* RENDER_TARGET の、R8G8B8A8_UNORM のトーンマップ済み表示用テクスチャ)
	* へ合成する。呼び出し側は VisibilityBuffer と深度をシェーダから読める
	* 状態(GeometryBuffer::End / EndDepth)にしておく必要があり、addresses
	* は縁取る GeometryBuffer とモデルのインスタンスを持つビューのもので
	* なければならない。
	*/
	void MaterialOutline::Draw(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, const RootAddresses& addresses)
	{
		auto* cmd = cmdList->Get();

		MaterialOutlineConstantBuffer constants{};
		constants.resolveUnorderedAccessIndex_ = resolveUnorderedAccessViewIndex_;
		constants.resolveShaderResourceIndex_ = resolveShaderResourceViewIndex_;
		constants.maxWidthUnorderedAccessIndex_ = maxWidthUnorderedAccessViewIndex_;
		constants.maxWidthShaderResourceIndex_ = maxWidthShaderResourceViewIndex_;
		constants.transparentUnorderedAccessIndex_ = transparentUnorderedAccessViewIndex_;
		constantBuffer_->Update(constants);
		Uint dispatchBufferIndex = constantBuffer_->Index();

		/// [EN] The transparent pass wrote the record as a UAV; Resolve reads it as one, so only an ordering barrier is needed.
		/// [JP] 半透明のパスは記録を UAV として書いた。Resolve も UAV として読むので、要るのは順序を保つバリアだけ。
		D3D12_RESOURCE_BARRIER transparentBarrier{};
		transparentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		transparentBarrier.UAV.pResource = transparentResource_.Get();
		cmd->ResourceBarrier(1, &transparentBarrier);

		if (resolveState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(resolveResource_.Get(), resolveState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			resolveState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		if (maxWidthState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			cmdList->Barrier(maxWidthResource_.Get(), maxWidthState_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			maxWidthState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetComputeRootSignature(rootSignature_->Get());
		RootSignature::BindCompute(cmd, addresses);
		cmd->SetComputeRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);

		cmd->SetPipelineState(resetMaxWidthPipelineState_.Get());
		cmd->Dispatch(1, 1, 1);

		/// [EN] Resolve accumulates into the max width the reset just cleared, so the two must not overlap.
		/// [JP] Resolve は直前に 0 へ戻した最大幅へ積み上げるので、2 つが重なって実行されてはならない。
		D3D12_RESOURCE_BARRIER maxWidthBarrier{};
		maxWidthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		maxWidthBarrier.UAV.pResource = maxWidthResource_.Get();
		cmd->ResourceBarrier(1, &maxWidthBarrier);

		cmd->SetPipelineState(resolvePipelineState_.Get());
		cmd->Dispatch((width_ + 7) / 8, (height_ + 7) / 8, 1);
		ProfilerStats::AddDrawCall();

		cmdList->Barrier(resolveResource_.Get(), resolveState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		resolveState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		cmdList->Barrier(maxWidthResource_.Get(), maxWidthState_, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		maxWidthState_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);
		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		cmd->SetGraphicsRootSignature(rootSignature_->Get());
		RootSignature::BindGraphics(cmd, addresses);
		cmd->SetGraphicsRoot32BitConstants(3, 1, &dispatchBufferIndex, 0);
		cmd->SetPipelineState(compositePipelineState_.Get());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(1, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(3, 1, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	/**
	* [EN]
	* Bindless UAV of the transparent record, for the transparent pass to
	* write outlined fragments into (UnorderedAccessIndicesSystem::
	* SetOITMaterialOutlineIndex).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 半透明のパスが縁取りのあるフラグメントを書き込む、半透明の記録の
	* bindless UAV(UnorderedAccessIndicesSystem::
	* SetOITMaterialOutlineIndex)。
	*/
	Uint32 MaterialOutline::TransparentUnorderedAccessViewIndex()const
	{
		return transparentUnorderedAccessViewIndex_;
	}
}

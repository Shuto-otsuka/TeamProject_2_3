#include <GraphicsEngine/Renderer/ColliderRenderer.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	ColliderRenderer::ColliderRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : colliderLineShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	void ColliderRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem)
	{
		bindlessHeap_ = bindlessHeap;
		constantIndicesSystem_ = &constantIndicesSystem;

		spatialInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		planarInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		spatialInstanceConstantsBuffer_ = MakePtr<StaticConstantBuffer<ColliderConstantBuffer>>(device, bindlessHeap);
		planarInstanceConstantsBuffer_ = MakePtr<StaticConstantBuffer<ColliderConstantBuffer>>(device, bindlessHeap);

		colliderLineShader_.Create(shaderCache, device);
	}

	void ColliderRenderer::Upload(std::span<const ColliderDesc> colliders)
	{
		spatialInstances_.clear();
		planarInstances_.clear();

		for (const ColliderDesc& collider : colliders)
		{
			DynamicArray<ColliderStructuredBuffer>& instances = (collider.kind_ == ColliderKind::Rect || collider.kind_ == ColliderKind::Circle) ? planarInstances_ : spatialInstances_;
			if (instances.size() >= maxInstanceCount_)
			{
				continue;
			}

			ColliderStructuredBuffer instance{};
			instance.position_ = collider.position_;
			instance.shapeKind_ = static_cast<Uint32>(collider.kind_);
			instance.rotation_ = collider.rotation_;
			instance.dimensions_ = collider.dimensions_;
			instance.color_ = collider.color_;

			instances.push_back(instance);
		}

		if (!spatialInstances_.empty())
		{
			Uint spatialInstanceCount = static_cast<Uint>(spatialInstances_.size());
			if (spatialInstanceCount > maxInstanceCount_)
			{
				spatialInstanceCount = maxInstanceCount_;
			}

			spatialInstanceBuffer_->Update(spatialInstances_.data(), spatialInstanceCount);

			ColliderConstantBuffer constants{};
			constants.instanceBufferIndex_ = spatialInstanceBuffer_->Index();
			constants.instanceCount_ = spatialInstanceCount;
			constants.groupsPerInstance_ = groupsPerInstance_;
			spatialInstanceConstantsBuffer_->Update(constants);

			constantIndicesSystem_->SetEditorColliderIndex(spatialInstanceConstantsBuffer_->Index());
		}

		if (!planarInstances_.empty())
		{
			Uint planarInstanceCount = static_cast<Uint>(planarInstances_.size());
			if (planarInstanceCount > maxInstanceCount_)
			{
				planarInstanceCount = maxInstanceCount_;
			}

			planarInstanceBuffer_->Update(planarInstances_.data(), planarInstanceCount);

			ColliderConstantBuffer constants{};
			constants.instanceBufferIndex_ = planarInstanceBuffer_->Index();
			constants.instanceCount_ = planarInstanceCount;
			constants.groupsPerInstance_ = groupsPerInstance_;
			planarInstanceConstantsBuffer_->Update(constants);

			constantIndicesSystem_->SetCanvasColliderIndex(planarInstanceConstantsBuffer_->Index());
		}
	}

	void ColliderRenderer::Draw3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (spatialInstances_.empty())
		{
			return;
		}

		Uint instanceCount = static_cast<Uint>(spatialInstances_.size());
		if (instanceCount > maxInstanceCount_)
		{
			instanceCount = maxInstanceCount_;
		}

		auto* cmd = cmdList->Get();

		/// [JP] 呼び出し側が渡した色 ＋ 深度（読み取りのみ）を bind する —
		///      ModelRenderer::DrawWireframe/DrawMeshlet と同じパターン。
		///      呼び出し側が depthStencilView の元となる深度リソースを
		///      既に読み取り可能な状態にしている前提。
		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, &depthStencilView);

		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(colliderLineShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		cmd->SetPipelineState(colliderLineShader_.GetPipelineStateDebugOverlay());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			/// [JP] 1インスタンスにつき groupsPerInstance_ 個のスレッドグループを
			///      割り当てる — カプセルは1グループ(threadsPerGroup_ スレッド)
			///      より線が多いため（ColliderLineMS.hlsl 参照）。
			cmd->DispatchMesh(instanceCount * groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(groupsPerInstance_ * threadsPerGroup_ * verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void ColliderRenderer::Draw2D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (planarInstances_.empty())
		{
			return;
		}

		Uint instanceCount = static_cast<Uint>(planarInstances_.size());
		if (instanceCount > maxInstanceCount_)
		{
			instanceCount = maxInstanceCount_;
		}

		auto* cmd = cmdList->Get();

		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);

		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(colliderLineShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		cmd->SetPipelineState(colliderLineShader_.GetPipelineStateCanvas());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(instanceCount * groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(groupsPerInstance_ * threadsPerGroup_ * verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}
}

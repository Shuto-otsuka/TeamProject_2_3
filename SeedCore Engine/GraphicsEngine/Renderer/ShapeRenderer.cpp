#include <GraphicsEngine/Renderer/ShapeRenderer.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>

namespace SeedCore
{
	ShapeRenderer::ShapeRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : wireframeShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Creates the three batches' instance buffers and constants, and the
	* shader's pipeline states.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 3つのバッチのインスタンスバッファと定数、シェーダーのパイプライン
	* ステートを作る。
	*/
	void ShapeRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem)
	{
		constantIndicesSystem_ = &constantIndicesSystem;

		worldInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		gameInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		canvasInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		worldConstantsBuffer_ = MakePtr<StaticConstantBuffer<ColliderConstantBuffer>>(device, bindlessHeap);
		gameConstantsBuffer_ = MakePtr<StaticConstantBuffer<ColliderConstantBuffer>>(device, bindlessHeap);
		canvasConstantsBuffer_ = MakePtr<StaticConstantBuffer<ColliderConstantBuffer>>(device, bindlessHeap);

		wireframeShader_.Create(shaderCache, device);
	}

	/**
	* [EN]
	* Packs the wireframe shapes into the GPU layout, sorting them into the
	* world, game and canvas batches (entries past maxInstanceCount_ in a
	* batch are dropped), uploads each non-empty batch with its constants,
	* and registers the world constants in the editor's primitive
	* wireframe slot, the game constants in the game's, and the canvas
	* constants in the canvas's.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワイヤーフレームの形を GPU 用の並びに詰め、ワールド、ゲーム、Canvas
	* のバッチへ振り分ける（バッチで maxInstanceCount_ を超えた分は破棄
	* する）。空でない各バッチを定数と一緒にアップロードし、ワールドの定数を
	* エディターの、ゲームの定数をゲームの、Canvas の定数を Canvas の
	* 基本形状ワイヤーフレームの枠へ登録する。
	*/
	void ShapeRenderer::Upload(std::span<const ShapeDesc> shapes)
	{
		worldInstances_.clear();
		gameInstances_.clear();
		canvasInstances_.clear();

		for (const ShapeDesc& shape : shapes)
		{
			/// [EN] Filled shapes have no drawing path yet.
			/// [JP] 面で描く形は、まだ描く手段がない。
			if (shape.style_ != ShapeStyle::Wireframe)
			{
				continue;
			}

			ColliderStructuredBuffer instance{};
			instance.position_ = shape.position_;
			instance.shapeKind_ = static_cast<Uint32>(shape.kind_);
			instance.rotation_ = shape.rotation_;
			instance.dimensions_ = shape.dimensions_;
			instance.color_ = shape.color_;
			instance.headLength_ = shape.headLength_;

			if (shape.space_ == ShapeSpace::Canvas)
			{
				if (canvasInstances_.size() < maxInstanceCount_)
				{
					canvasInstances_.push_back(instance);
				}
				continue;
			}

			/// [EN] Every world shape is drawn in the editor view; those scoped to the game are drawn in the game view as well.
			/// [JP] ワールドの形はすべてエディタービューに描き、Game のものはゲームビューにも描く。
			if (worldInstances_.size() < maxInstanceCount_)
			{
				worldInstances_.push_back(instance);
			}
			if (shape.scope_ == ShapeScope::Game && gameInstances_.size() < maxInstanceCount_)
			{
				gameInstances_.push_back(instance);
			}
		}

		if (!worldInstances_.empty())
		{
			Uint worldInstanceCount = static_cast<Uint>(worldInstances_.size());
			worldInstanceBuffer_->Update(worldInstances_.data(), worldInstanceCount);

			ColliderConstantBuffer constants{};
			constants.instanceBufferIndex_ = worldInstanceBuffer_->Index();
			constants.instanceCount_ = worldInstanceCount;
			constants.groupsPerInstance_ = ColliderRenderer::groupsPerInstance_;
			worldConstantsBuffer_->Update(constants);

			constantIndicesSystem_->SetEditorPrimitiveWireframeIndex(worldConstantsBuffer_->Index());
		}

		if (!gameInstances_.empty())
		{
			Uint gameInstanceCount = static_cast<Uint>(gameInstances_.size());
			gameInstanceBuffer_->Update(gameInstances_.data(), gameInstanceCount);

			ColliderConstantBuffer constants{};
			constants.instanceBufferIndex_ = gameInstanceBuffer_->Index();
			constants.instanceCount_ = gameInstanceCount;
			constants.groupsPerInstance_ = ColliderRenderer::groupsPerInstance_;
			gameConstantsBuffer_->Update(constants);

			constantIndicesSystem_->SetGamePrimitiveWireframeIndex(gameConstantsBuffer_->Index());
		}

		if (!canvasInstances_.empty())
		{
			Uint canvasInstanceCount = static_cast<Uint>(canvasInstances_.size());
			canvasInstanceBuffer_->Update(canvasInstances_.data(), canvasInstanceCount);

			ColliderConstantBuffer constants{};
			constants.instanceBufferIndex_ = canvasInstanceBuffer_->Index();
			constants.instanceCount_ = canvasInstanceCount;
			constants.groupsPerInstance_ = ColliderRenderer::groupsPerInstance_;
			canvasConstantsBuffer_->Update(constants);

			constantIndicesSystem_->SetCanvasPrimitiveWireframeIndex(canvasConstantsBuffer_->Index());
		}
	}

	/**
	* [EN]
	* Draws the world batch into the editor view, depth-tested against the
	* given depth view. Must be called with the editor's root addresses.
	* Does nothing if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールドのバッチを、指定した深度ビューで深度テストしながら
	* エディタービューへ描く。エディターのルートアドレスで呼ぶこと。
	* バッチが空なら何もしない。
	*/
	void ShapeRenderer::DrawEditor3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (worldInstances_.empty())
		{
			return;
		}

		DrawWorld(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses, static_cast<Uint>(worldInstances_.size()));
	}

	/**
	* [EN]
	* Draws the game batch into the game view, depth-tested against the
	* given depth view. Must be called with the game's root addresses.
	* Does nothing if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームのバッチを、指定した深度ビューで深度テストしながら
	* ゲームビューへ描く。ゲームのルートアドレスで呼ぶこと。
	* バッチが空なら何もしない。
	*/
	void ShapeRenderer::DrawGame3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (gameInstances_.empty())
		{
			return;
		}

		DrawWorld(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses, static_cast<Uint>(gameInstances_.size()));
	}

	/**
	* [EN]
	* Draws the canvas batch onto the canvas, over everything without
	* depth. Must be called with the canvas's root addresses. Does nothing
	* if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Canvas のバッチを、深度を使わずに Canvas の上から描く。Canvas の
	* ルートアドレスで呼ぶこと。バッチが空なら何もしない。
	*/
	void ShapeRenderer::Draw2D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (canvasInstances_.empty())
		{
			return;
		}

		Uint instanceCount = static_cast<Uint>(canvasInstances_.size());

		auto* cmd = cmdList->Get();

		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);

		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(wireframeShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		cmd->SetPipelineState(wireframeShader_.GetPipelineStateCanvas());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(instanceCount * ColliderRenderer::groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(ColliderRenderer::groupsPerInstance_ * ColliderRenderer::threadsPerGroup_ * ColliderRenderer::verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	/**
	* [EN]
	* Records the draw of instanceCount world instances into the given
	* target, with the primitive wireframe slot of the view whose root
	* addresses are passed. Shared by DrawEditor3D and DrawGame3D.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールドのインスタンス instanceCount 個を、指定したターゲットへ描く
	* 記録をする。使う基本形状ワイヤーフレームの枠は、渡したルート
	* アドレスのビューのもの。DrawEditor3D と DrawGame3D で共有する。
	*/
	void ShapeRenderer::DrawWorld(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint instanceCount)
	{
		auto* cmd = cmdList->Get();

		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, &depthStencilView);

		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(wireframeShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		cmd->SetPipelineState(wireframeShader_.GetPipelineState());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(instanceCount * ColliderRenderer::groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(ColliderRenderer::groupsPerInstance_ * ColliderRenderer::threadsPerGroup_ * ColliderRenderer::verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}
}

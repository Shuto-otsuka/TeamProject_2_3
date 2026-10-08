#include <GraphicsEngine/Renderer/ShapeRenderer.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/Texture/Texture.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>

namespace SeedCore
{
	ShapeRenderer::ShapeRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : wireframeShader_(rootSignature, pipelineStateObject), solidShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	/**
	* [EN]
	* Builds the unit meshes, creates the wireframe and solid buffers and
	* the solid culling buffer, and builds both shaders' pipeline states.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 単位メッシュを作り、ワイヤーフレームと面のバッファ、面のカリングの
	* バッファを作り、2つのシェーダーのパイプラインステートを作る。
	*/
	void ShapeRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderResourceIndicesSystem& shaderResourceIndicesSystem)
	{
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;
		bindlessHeap_ = bindlessHeap;

		worldInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>>(device, bindlessHeap, maxWireframeInstanceCount_);
		gameInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>>(device, bindlessHeap, maxWireframeInstanceCount_);
		canvasInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>>(device, bindlessHeap, maxWireframeInstanceCount_);

		/// [EN] The unit meshes are built once; their buffers are sized to them exactly.
		/// [JP] 単位メッシュは一度だけ作り、そのバッファはちょうどの大きさで作る。
		primitiveMesh_.Build();
		solidInstanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<PrimitiveSolidStructuredBuffer>>(device, bindlessHeap, maxSolidInstanceCount_);
		solidVertexBuffer_ = MakePtr<ReadOnlyStructuredBuffer<PrimitiveVertex>>(device, bindlessHeap, static_cast<Uint>(primitiveMesh_.Vertices().size()));
		solidMeshletBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletDesc>>(device, bindlessHeap, static_cast<Uint>(primitiveMesh_.Meshlets().size()));
		solidMeshletBoundBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MeshletBound>>(device, bindlessHeap, static_cast<Uint>(primitiveMesh_.MeshletBounds().size()));
		solidVertexIndicesBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Uint32>>(device, bindlessHeap, static_cast<Uint>(primitiveMesh_.VertexIndices().size()));
		solidPrimitiveIndicesBuffer_ = MakePtr<ReadOnlyByteAddressBuffer>(device, bindlessHeap, static_cast<Uint>(primitiveMesh_.PrimitiveIndices().size()));

		/// [EN] Every instance's meshlets may all survive culling, so the visible lists hold the most instances times the most meshlets per shape.
		/// [JP] 全インスタンスのメッシュレットが全部カリングを通り抜けることもあるので、見える一覧は「最大のインスタンス数 × 形1つの最大のメッシュレット数」の大きさにする。
		solidCullingBuffer_.Create(device, bindlessHeap);
		solidCullingBuffer_.Reserve(maxSolidInstanceCount_ * PrimitiveMesh::maxMeshletsPerShape_);

		wireframeShader_.Create(shaderCache, device);
		solidShader_.Create(shaderCache, device);
	}

	/**
	* [EN]
	* Packs the shapes and the colliders into the GPU layouts (entries past
	* a batch's maximum are dropped), uploads each non-empty batch, and
	* registers the wireframe batches in each view's primitive wireframe
	* slot and the solid buffers in the primitive solid slots. Textures of
	* solid shapes are resolved to bindless indices here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形とコライダーを GPU 用の並びに詰め（バッチの最大数を超えた分は
	* 破棄する）、空でない各バッチをアップロードする。ワイヤーフレームの
	* バッチは各ビューの基本形状ワイヤーフレームの枠に、面のバッファは
	* 基本形状の面の枠に登録する。面の形のテクスチャは、ここで bindless の
	* 番号に直す。
	*/
	void ShapeRenderer::Upload(std::span<const ShapeDesc> shapes, std::span<const ShapeDesc> colliders, LoaderSystem& loader, TextureResource& textureResource)
	{
		worldInstances_.clear();
		gameInstances_.clear();
		canvasInstances_.clear();
		solidInstances_.clear();

		for (const ShapeDesc& shape : shapes)
		{
			AppendShape(shape, loader, textureResource);
		}
		for (const ShapeDesc& collider : colliders)
		{
			AppendShape(collider, loader, textureResource);
		}
		streamingFrame_++;

		/// [EN] Each view's table points at its own wireframe batch.
		/// [JP] 各ビューの表は、それぞれのワイヤーフレームのバッチを指す。
		if (!worldInstances_.empty())
		{
			worldInstanceBuffer_->Update(worldInstances_.data(), static_cast<Uint>(worldInstances_.size()));
			shaderResourceIndicesSystem_->SetEditorPrimitiveWireframeIndex(worldInstanceBuffer_->Index());
		}
		if (!gameInstances_.empty())
		{
			gameInstanceBuffer_->Update(gameInstances_.data(), static_cast<Uint>(gameInstances_.size()));
			shaderResourceIndicesSystem_->SetGamePrimitiveWireframeIndex(gameInstanceBuffer_->Index());
		}
		if (!canvasInstances_.empty())
		{
			canvasInstanceBuffer_->Update(canvasInstances_.data(), static_cast<Uint>(canvasInstances_.size()));
			shaderResourceIndicesSystem_->SetCanvasPrimitiveWireframeIndex(canvasInstanceBuffer_->Index());
		}

		if (!solidInstances_.empty())
		{
			/// [EN] The buffers are frame-ring buffers that fill only the current frame's slot, so the unchanging unit meshes are written every frame as well.
			/// [JP] バッファはフレームリングで、今のフレームの枠にしか書かないので、変わらない単位メッシュも毎フレーム書く。
			solidInstanceBuffer_->Update(solidInstances_.data(), static_cast<Uint>(solidInstances_.size()));
			solidVertexBuffer_->Update(primitiveMesh_.Vertices().data(), static_cast<Uint>(primitiveMesh_.Vertices().size()));
			solidMeshletBuffer_->Update(primitiveMesh_.Meshlets().data(), static_cast<Uint>(primitiveMesh_.Meshlets().size()));
			solidMeshletBoundBuffer_->Update(primitiveMesh_.MeshletBounds().data(), static_cast<Uint>(primitiveMesh_.MeshletBounds().size()));
			solidVertexIndicesBuffer_->Update(primitiveMesh_.VertexIndices().data(), static_cast<Uint>(primitiveMesh_.VertexIndices().size()));
			solidPrimitiveIndicesBuffer_->Update(primitiveMesh_.PrimitiveIndices().data(), static_cast<Uint>(primitiveMesh_.PrimitiveIndices().size()));

			shaderResourceIndicesSystem_->SetPrimitiveSolidInstanceIndex(solidInstanceBuffer_->Index());
			shaderResourceIndicesSystem_->SetPrimitiveSolidVertexIndex(solidVertexBuffer_->Index());
			shaderResourceIndicesSystem_->SetPrimitiveSolidMeshletIndex(solidMeshletBuffer_->Index());
			shaderResourceIndicesSystem_->SetPrimitiveSolidMeshletBoundIndex(solidMeshletBoundBuffer_->Index());
			shaderResourceIndicesSystem_->SetPrimitiveSolidVertexIndicesIndex(solidVertexIndicesBuffer_->Index());
			shaderResourceIndicesSystem_->SetPrimitiveSolidPrimitiveIndicesIndex(solidPrimitiveIndicesBuffer_->Index());
		}
	}

	/**
	* [EN]
	* Draws the solid batch and then the world wireframe batch into the
	* editor view, depth-tested against the given depth view, so the lines
	* sit over the surfaces. Must be called with the editor's root
	* addresses. Empty batches are skipped.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 面のバッチ、続いてワールドのワイヤーフレームのバッチを、指定した
	* 深度ビューで深度テストしながらエディタービューへ描く。線が面の上に
	* 乗るよう、この順にする。エディターのルートアドレスで呼ぶこと。空の
	* バッチは飛ばす。
	*/
	void ShapeRenderer::DrawEditor3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		DrawSolid(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses);
		if (!worldInstances_.empty())
		{
			DrawWireframe(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses, static_cast<Uint>(worldInstances_.size()));
		}
	}

	/**
	* [EN]
	* Draws the solid batch into the game view, depth-tested against the
	* given depth view. Filled shapes are part of the scene, so this is
	* drawn before the hudless capture. Must be called with the game's
	* root addresses. Does nothing if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 面のバッチを、指定した深度ビューで深度テストしながらゲームビューへ
	* 描く。面の形はシーンの一部なので、hudless の取得より前に描く。
	* ゲームのルートアドレスで呼ぶこと。バッチが空なら何もしない。
	*/
	void ShapeRenderer::DrawGameSolid(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		DrawSolid(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses);
	}

	/**
	* [EN]
	* Draws the game wireframe batch into the game view, depth-tested
	* against the given depth view. Wireframes are a debug display, so
	* this is drawn after the hudless capture. Must be called with the
	* game's root addresses. Does nothing if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームのワイヤーフレームのバッチを、指定した深度ビューで深度テスト
	* しながらゲームビューへ描く。ワイヤーフレームはデバッグ表示なので、
	* hudless の取得より後に描く。ゲームのルートアドレスで呼ぶこと。
	* バッチが空なら何もしない。
	*/
	void ShapeRenderer::DrawGameWireframe(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (gameInstances_.empty())
		{
			return;
		}

		DrawWireframe(cmdList, renderTargetView, depthStencilView, viewport, heap, addresses, static_cast<Uint>(gameInstances_.size()));
	}

	/**
	* [EN]
	* Draws the canvas wireframe batch onto the canvas, over everything
	* without depth. Must be called with the canvas's root addresses. Does
	* nothing if the batch is empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Canvas のワイヤーフレームのバッチを、深度を使わずに Canvas の上から
	* 描く。Canvas のルートアドレスで呼ぶこと。バッチが空なら何もしない。
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
			cmd->DispatchMesh(instanceCount * PrimitiveWireframeShader::groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(PrimitiveWireframeShader::groupsPerInstance_ * PrimitiveWireframeShader::linesPerGroup_ * PrimitiveWireframeShader::verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	/**
	* [EN]
	* Packs one shape into the batch it belongs to: solid shapes in the
	* world with a unit mesh into the solid batch, wireframe shapes into
	* the canvas batch or the world batch (and the game batch when scoped
	* to the game).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形1つを、それが入るバッチに詰める。単位メッシュがあるワールドの面の
	* 形は面のバッチへ、ワイヤーフレームの形は Canvas のバッチか
	* ワールドのバッチ（Game のものはゲームのバッチにも）へ入れる。
	*/
	void ShapeRenderer::AppendShape(const ShapeDesc& shape, LoaderSystem& loader, TextureResource& textureResource)
	{
		if (shape.style_ == ShapeStyle::Solid)
		{
			/// [EN] Filled shapes are drawn in the 3D views only, and only kinds that have a unit mesh.
			/// [JP] 面で描く形は 3D のビューにだけ描き、単位メッシュがある種類だけを扱う。
			const Meshlet::MeshletRange& meshletRange = primitiveMesh_.MeshletRanges()[static_cast<Size>(shape.kind_)];
			if (shape.space_ != ShapeSpace::World || meshletRange.meshletCount_ == 0 || solidInstances_.size() >= maxSolidInstanceCount_)
			{
				return;
			}

			PrimitiveSolidStructuredBuffer instance{};
			instance.position_ = shape.position_;
			instance.rotation_ = shape.rotation_;
			instance.dimensions_ = shape.dimensions_;
			instance.color_ = shape.color_;
			instance.uvScale_ = shape.uvScale_;
			instance.uvOffset_ = shape.uvOffset_;
			instance.shapeKind_ = static_cast<Uint32>(shape.kind_);
			instance.doubleSided_ = shape.doubleSided_ ? 1 : 0;
			instance.meshletOffset_ = meshletRange.meshletOffset_;
			instance.meshletCount_ = meshletRange.meshletCount_;

			/// [EN] A texture that is not loaded or resident yet leaves the shape with its color alone until it is.
			/// [JP] まだロードも常駐もしていないテクスチャは、そうなるまで形を色だけで描く。
			if (shape.textureID_ != 0)
			{
				Handle<Texture> textureHandle = textureResource.GetHandle(shape.textureID_);
				if (!textureHandle.empty())
				{
					Texture* texture = textureResource.Resolve(loader, bindlessHeap_, textureHandle, streamingFrame_);
					if (texture)
					{
						instance.textureIndex_ = texture->ShaderResourceViewIndex();
					}
				}
			}

			solidInstances_.push_back(instance);
			return;
		}

		PrimitiveWireframeStructuredBuffer instance{};
		instance.position_ = shape.position_;
		instance.rotation_ = shape.rotation_;
		instance.dimensions_ = shape.dimensions_;
		instance.color_ = shape.color_;
		instance.shapeKind_ = static_cast<Uint32>(shape.kind_);
		instance.headLength_ = shape.headLength_;
		instance.lineWidth_ = shape.lineWidth_;

		if (shape.space_ == ShapeSpace::Canvas)
		{
			if (canvasInstances_.size() < maxWireframeInstanceCount_)
			{
				canvasInstances_.push_back(instance);
			}
			return;
		}

		/// [EN] Every world shape is drawn in the editor view; those scoped to the game are drawn in the game view as well.
		/// [JP] ワールドの形はすべてエディタービューに描き、Game のものはゲームビューにも描く。
		if (worldInstances_.size() < maxWireframeInstanceCount_)
		{
			worldInstances_.push_back(instance);
		}
		if (shape.scope_ == ShapeScope::Game && gameInstances_.size() < maxWireframeInstanceCount_)
		{
			gameInstances_.push_back(instance);
		}
	}

	/**
	* [EN]
	* Records the draw of instanceCount world wireframe instances into the
	* given target, with the primitive wireframe slot of the view whose
	* root addresses are passed. Shared by DrawEditor3D and DrawGameWireframe.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールドのワイヤーフレームのインスタンス instanceCount 個を、指定した
	* ターゲットへ描く記録をする。使う基本形状ワイヤーフレームの枠は、
	* 渡したルートアドレスのビューのもの。DrawEditor3D と DrawGameWireframe で
	* 共有する。
	*/
	void ShapeRenderer::DrawWireframe(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint instanceCount)
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
			cmd->DispatchMesh(instanceCount * PrimitiveWireframeShader::groupsPerInstance_, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(PrimitiveWireframeShader::groupsPerInstance_ * PrimitiveWireframeShader::linesPerGroup_ * PrimitiveWireframeShader::verticesPerLine_, instanceCount, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	/**
	* [EN]
	* Records the draw of the solid batch into the given target. On D12_2
	* one mesh dispatch covers every instance; below it the meshlets are
	* culled into the single-sided and double-sided lists for this view
	* and each list is drawn indirectly. Does nothing if the batch is
	* empty.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 面のバッチを、指定したターゲットへ描く記録をする。D12_2 では1回の
	* メッシュのディスパッチで全インスタンスを描く。それ未満では、この
	* ビュー用にメッシュレットを片面用と両面用の一覧へカリングし、それぞれの
	* 一覧を間接描画する。バッチが空なら何もしない。
	*/
	void ShapeRenderer::DrawSolid(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (solidInstances_.empty())
		{
			return;
		}

		Uint instanceCount = static_cast<Uint>(solidInstances_.size());

		auto* cmd = cmdList->Get();

		cmd->OMSetRenderTargets(1, &renderTargetView, FALSE, &depthStencilView);

		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(solidShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		/// [EN] One amplification group per instance culls and hands its visible meshlets to the mesh shader.
		/// [JP] インスタンスごとに1つの Amplification Shader のグループが、見えるメッシュレットを選んでメッシュシェーダーへ渡す。
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(solidShader_.GetPipelineState());
			cmd->DispatchMesh(instanceCount, 1, 1);
			ProfilerStats::AddDrawCall();
			return;
		}

		/// [EN] The culling compute shader writes this view's visible meshlets into the single-sided and double-sided lists and counts them into the indirect arguments, as the model culling does.
		/// [JP] カリングの Compute Shader が、このビューで見えるメッシュレットを片面用と両面用の一覧へ書き、その数を間接描画の引数に数える。モデルのカリングと同じ形。
		solidCullingBuffer_.Begin(cmd);

		cmd->SetComputeRootSignature(solidShader_.GetRootSignature());
		RootSignature::BindCompute(cmd, addresses);
		Uint cullingIndex = solidCullingBuffer_.GetConstantBufferIndex();
		cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
		cmd->SetPipelineState(solidShader_.GetPipelineStateCulling());
		cmd->Dispatch(instanceCount, 1, 1);

		solidCullingBuffer_.Barrier(cmd);

		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		Uint singleSidedIndex = solidCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
		cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
		cmd->SetPipelineState(solidShader_.GetPipelineStateSingleSided());
		cmd->ExecuteIndirect(solidCullingBuffer_.GetCommandSignature(), 1, solidCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
		ProfilerStats::AddDrawCall();

		Uint doubleSidedIndex = solidCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();
		cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
		cmd->SetPipelineState(solidShader_.GetPipelineStateDoubleSided());
		cmd->ExecuteIndirect(solidCullingBuffer_.GetCommandSignature(), 1, solidCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
		ProfilerStats::AddDrawCall();

		solidCullingBuffer_.End(cmd);
	}
}

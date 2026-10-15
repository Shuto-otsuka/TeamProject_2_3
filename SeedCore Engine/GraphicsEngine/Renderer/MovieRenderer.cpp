#include <GraphicsEngine/Renderer/MovieRenderer.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Movie/Video.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/System/IndicesSystem.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	MovieRenderer::MovieRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : movieShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	void MovieRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderResourceIndicesSystem& shaderResourceIndicesSystem)
	{
		bindlessHeap_ = bindlessHeap;
		maxCount_ = 1024;

		movieShader_.Create(shaderCache, device);

		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;

		spriteBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MovieSpriteStructuredBuffer>>(device, bindlessHeap, maxCount_);
		billboardBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MovieBillboardStructuredBuffer>>(device, bindlessHeap, maxCount_);
		fullscreenBuffer_ = MakePtr<ReadOnlyStructuredBuffer<MovieFullscreenStructuredBuffer>>(device, bindlessHeap, 32);

		shaderResourceIndicesSystem.SetMovieSpriteIndex(spriteBuffer_->Index());
		shaderResourceIndicesSystem.SetMovieBillboardIndex(billboardBuffer_->Index());
		shaderResourceIndicesSystem.SetMovieFullscreenIndex(fullscreenBuffer_->Index());
	}

	void MovieRenderer::Gather(MovieResource& movieResource, World& world, Vector2 nativeScreenSize, std::span<const Entity> selectedEntities)
	{
		spriteInstances_.clear();
		billboardInstances_.clear();
		canvasInstances_.clear();
		fullscreenInstances_.clear();
		spriteBatches_.clear();
		orderedEntities_.clear();
		hasSelectedSpriteInstance_ = false;
		hasSelectedBillboardInstance_ = false;

		Float spriteReferenceScale = (ScResolution::SC_CANVAS.Height > 0.0f) ? (nativeScreenSize.y / ScResolution::SC_CANVAS.Height) : 1.0f;

		/// [EN] Cached once so each Sprite-mode Movie actor's Bounds can be synced to its canvas rect below (mirrors TextureRenderer's Image -> Bounds sync), giving canvas movies a pickable box in CanvasViewPanel.
		/// [JP] 各スプライトモード Movie アクターの Bounds を下でキャンバス矩形へ同期できるよう一度だけキャッシュする(TextureRenderer の Image -> Bounds 同期と同じ)。これでキャンバス動画が CanvasViewPanel でピック可能なボックスを持つ。
		ComponentID boundsComponentID = ComponentRegistry::GetComponentID<Bounds>();

		Query<Read<Active>, Read<Movie>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Movie& movie)
			{
				if (!active.active_)
				{
					return;
				}

				orderedEntities_.push_back({ movie.order_, entityID });
			});

		std::ranges::stable_sort(orderedEntities_, [](const OrderedEntity& a, const OrderedEntity& b) { return a.order_ < b.order_; });

		++batchSerial_;
		Uint worldBatch = batchSerial_;

		for (const OrderedEntity& orderedEntity : orderedEntities_)
		{
			EntityID entityID = orderedEntity.entityID_;

			Actor actor = world.GetActor(entityID);
			if (!actor)
			{
				continue;
			}

			const Movie* movieComponent = actor.GetComponent<Movie>();
			if (!movieComponent)
			{
				continue;
			}
			const Movie& movie = *movieComponent;

			Video* video = movieResource.Resolve(movieResource.GetHandle(movie.movieID_));
			if (movie.movieID_ == 0 || !video || !video->HasTexture())
			{
				continue;
			}

			Uint textureIndex = video->GetTextureIndex();
			Int nativeWidth = video->GetWidth();
			Int nativeHeight = video->GetHeight();

			if (movie.displayMode_ == Movie::DisplayMode::Fullscreen)
			{
				if (nativeWidth <= 0 || nativeHeight <= 0)
				{
					continue;
				}

				MovieFullscreenStructuredBuffer instance{};
				instance.color_ = movie.color_;
				instance.textureIndex_ = textureIndex;
				instance.textureAspect_ = static_cast<Float>(nativeWidth) / static_cast<Float>(nativeHeight);
				fullscreenInstances_.push_back(instance);
				continue;
			}

			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 worldScale;
			Quaternion worldRotation;
			Vector3 worldTranslation;
			worldMatrix.Decompose(worldScale, worldRotation, worldTranslation);

			Vector2 size = movie.size_;
			if (size.x <= 0.0f && size.y <= 0.0f)
			{
				size = Vector2(static_cast<Float>(nativeWidth), static_cast<Float>(nativeHeight));
			}

			Uint selected = std::ranges::find(selectedEntities, actor.GetEntity()) != selectedEntities.end() ? 1 : 0;

			if (movie.displayMode_ == Movie::DisplayMode::Sprite)
			{
				hasSelectedSpriteInstance_ = hasSelectedSpriteInstance_ || selected != 0;

				Bool newBatch = spriteBatches_.empty() || spriteBatches_.back().order_ != movie.order_;
				Uint batch = newBatch ? batchSerial_ + 1 : batchSerial_;

				MovieSpriteStructuredBuffer instance{};
				instance.position_ = Vector2(worldTranslation.x, worldTranslation.y) * spriteReferenceScale;
				instance.rotation_ = worldRotation.ToEuler().z;
				instance.scale_ = Vector2(worldScale.x, worldScale.y) * spriteReferenceScale;
				instance.batch_ = batch;
				instance.size_ = size;
				instance.pivot_ = movie.pivot_;
				instance.color_ = movie.color_;
				instance.textureIndex_ = textureIndex;
				instance.selected_ = selected;
				spriteInstances_.push_back(instance);

				hasSelectedBillboardInstance_ = hasSelectedBillboardInstance_ || selected != 0;

				MovieBillboardStructuredBuffer canvasInstance{};
				canvasInstance.position_ = Vector3(100000.0f + worldTranslation.x, 100000.0f + (ScResolution::SC_CANVAS.Height - worldTranslation.y), 100000.0f);
				canvasInstance.rotation_ = Vector3::Zero;
				canvasInstance.scale_ = Vector2(worldScale.x * size.x, worldScale.y * size.y);
				canvasInstance.size_ = size;
				canvasInstance.pivot_ = movie.pivot_;
				canvasInstance.color_ = movie.color_;
				canvasInstance.textureIndex_ = textureIndex;
				canvasInstance.faceCamera_ = 1;
				canvasInstance.selected_ = selected;
				canvasInstance.batch_ = batch;
				canvasInstances_.push_back(canvasInstance);

				if (newBatch)
				{
					++batchSerial_;
					spriteBatches_.push_back({ movie.order_, static_cast<Uint>(spriteInstances_.size()) - 1, 1, static_cast<Uint>(canvasInstances_.size()) - 1, 1 });
				}
				else
				{
					++spriteBatches_.back().spriteCount_;
					++spriteBatches_.back().canvasCount_;
				}

				/// [EN] Sync this actor's Bounds (every actor has one) to the canvas movie's local-space rect: half-extents = size/2, centre shifted by the pivot the same way MovieBillboardMS.hlsl shifts the quad (0.5 - pivotNorm on X, pivotNorm - 0.5 on Y). Unscaled - the picker applies the actor's world scale.
				/// [JP] このアクターの Bounds(全アクターが持つ)をキャンバス動画のローカル空間矩形へ同期する: 半径 = size/2、中心は MovieBillboardMS.hlsl がクアッドをずらすのと同じ形(X は 0.5 - pivotNorm、Y は pivotNorm - 0.5)で pivot ぶんずらす。未スケール - ピッカーがアクターのワールドスケールを掛ける。
				if (boundsComponentID)
				{
					void* boundsRaw = world.GetComponent(entityID, boundsComponentID);
					if (boundsRaw)
					{
						Vector2 pivotNorm = (size.x > 0.0f && size.y > 0.0f) ? Vector2(movie.pivot_.x / size.x, movie.pivot_.y / size.y) : Vector2(0.5f, 0.5f);
						Bounds* bounds = static_cast<Bounds*>(boundsRaw);
						bounds->center_ = Vector3((0.5f - pivotNorm.x) * size.x, (pivotNorm.y - 0.5f) * size.y, 0.0f);
						bounds->extent_ = Vector3(size.x * 0.5f, size.y * 0.5f, 1.0f);
					}
				}
			}
			else
			{
				hasSelectedBillboardInstance_ = hasSelectedBillboardInstance_ || selected != 0;

				MovieBillboardStructuredBuffer instance{};
				instance.position_ = worldTranslation;
				instance.rotation_ = worldRotation.ToEuler();
				instance.scale_ = Vector2(worldScale.x, worldScale.y);
				instance.size_ = size;
				instance.pivot_ = movie.pivot_;
				instance.color_ = movie.color_;
				instance.textureIndex_ = textureIndex;
				instance.faceCamera_ = movie.faceCamera_ ? 1 : 0;
				instance.selected_ = selected;
				instance.batch_ = worldBatch;
				billboardInstances_.push_back(instance);
			}
		}

		worldBillboardCount_ = static_cast<Uint>(billboardInstances_.size());
		billboardInstances_.insert(billboardInstances_.end(), canvasInstances_.begin(), canvasInstances_.end());
	}

	void MovieRenderer::Upload()
	{
		shaderResourceIndicesSystem_->SetMovieSpriteIndex(spriteBuffer_->Index());
		shaderResourceIndicesSystem_->SetMovieBillboardIndex(billboardBuffer_->Index());
		shaderResourceIndicesSystem_->SetMovieFullscreenIndex(fullscreenBuffer_->Index());

		if (!spriteInstances_.empty())
		{
			spriteBuffer_->Update(spriteInstances_.data(), static_cast<Uint>(spriteInstances_.size()));
		}

		if (!billboardInstances_.empty())
		{
			billboardBuffer_->Update(billboardInstances_.data(), static_cast<Uint>(billboardInstances_.size()));
		}

		if (!fullscreenInstances_.empty())
		{
			fullscreenBuffer_->Update(fullscreenInstances_.data(), static_cast<Uint>(fullscreenInstances_.size()));
		}
	}

	Size MovieRenderer::GetSpriteBatchCount()const
	{
		return spriteBatches_.size();
	}

	Int MovieRenderer::GetSpriteBatchOrder(Size index)const
	{
		return spriteBatches_[index].order_;
	}

	void MovieRenderer::DrawFullscreen(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (fullscreenInstances_.empty())
		{
			return;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdList->SetGraphicsRootSignature(movieShader_.GetRootSignature());

		RootSignature::BindGraphics(cmdList, addresses);

		cmdList->SetPipelineState(movieShader_.GetPipelineStateFullscreen());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			Uint groupCount = (static_cast<Uint>(fullscreenInstances_.size()) + 31) / 32;
			cmdList->DispatchMesh(groupCount, 1, 1);
		}
		else
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, static_cast<Uint>(fullscreenInstances_.size()), 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void MovieRenderer::DrawSpriteBatch(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Size index)
	{
		DrawSpriteRange(cmdList, heap, addresses, spriteBatches_[index].spriteStart_, spriteBatches_[index].spriteCount_);
	}

	void MovieRenderer::DrawCanvasBatch(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Size index)
	{
		DrawBillboardRange(cmdList, heap, addresses, worldBillboardCount_ + spriteBatches_[index].canvasStart_, spriteBatches_[index].canvasCount_);
	}

	void MovieRenderer::DrawWorldBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		DrawBillboardRange(cmdList, heap, addresses, 0, worldBillboardCount_);
	}

	void MovieRenderer::DrawBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		DrawBillboardRange(cmdList, heap, addresses, 0xFFFFFFFFu, static_cast<Uint>(billboardInstances_.size()));
	}

	void MovieRenderer::DrawSilhouetteSprite(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (!hasSelectedSpriteInstance_)
		{
			return;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdList->SetGraphicsRootSignature(movieShader_.GetRootSignature());
		RootSignature::BindGraphics(cmdList, addresses);

		Uint fullRange = 0xFFFFFFFFu;
		cmdList->SetGraphicsRoot32BitConstants(3, 1, &fullRange, 0);

		cmdList->SetPipelineState(movieShader_.GetPipelineStateSilhouetteSprite());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			Uint groupCount = (static_cast<Uint>(spriteInstances_.size()) + 31) / 32;
			cmdList->DispatchMesh(groupCount, 1, 1);
		}
		else
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, static_cast<Uint>(spriteInstances_.size()), 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void MovieRenderer::DrawSilhouetteBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (!hasSelectedBillboardInstance_)
		{
			return;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdList->SetGraphicsRootSignature(movieShader_.GetRootSignature());
		RootSignature::BindGraphics(cmdList, addresses);

		Uint fullRange = 0xFFFFFFFFu;
		cmdList->SetGraphicsRoot32BitConstants(3, 1, &fullRange, 0);

		cmdList->SetPipelineState(movieShader_.GetPipelineStateSilhouetteBillboard());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			Uint groupCount = (static_cast<Uint>(billboardInstances_.size()) + 31) / 32;
			cmdList->DispatchMesh(groupCount, 1, 1);
		}
		else
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, static_cast<Uint>(billboardInstances_.size()), 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void MovieRenderer::DrawSpriteRange(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint start, Uint count)
	{
		if (count == 0)
		{
			return;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdList->SetGraphicsRootSignature(movieShader_.GetRootSignature());

		RootSignature::BindGraphics(cmdList, addresses);
		cmdList->SetGraphicsRoot32BitConstants(3, 1, &start, 0);

		cmdList->SetPipelineState(movieShader_.GetPipelineStateSprite());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			Uint groupCount = (count + 31) / 32;
			cmdList->DispatchMesh(groupCount, 1, 1);
		}
		else
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, count, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void MovieRenderer::DrawBillboardRange(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint start, Uint count)
	{
		if (count == 0)
		{
			return;
		}

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
		cmdList->SetGraphicsRootSignature(movieShader_.GetRootSignature());

		RootSignature::BindGraphics(cmdList, addresses);
		cmdList->SetGraphicsRoot32BitConstants(3, 1, &start, 0);

		cmdList->SetPipelineState(movieShader_.GetPipelineStateBillboard());

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			Uint groupCount = (count + 31) / 32;
			cmdList->DispatchMesh(groupCount, 1, 1);
		}
		else
		{
			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->DrawInstanced(6, count, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}
}

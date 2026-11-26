#include <External/NsightAftermath/Include/GFSDK_Aftermath.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerFlattener.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	CristerResidency::~CristerResidency()
	{
		auto found = std::ranges::find(streamingRegistry_, this);
		if (found != streamingRegistry_.end())
		{
			streamingRegistry_.erase(found);
		}
	}

	void CristerResidency::Release(BindlessHeap* heap)
	{
		if (!uploaded_)
		{
			return;
		}

		auto found = std::ranges::find(streamingRegistry_, this);
		if (found != streamingRegistry_.end())
		{
			streamingRegistry_.erase(found);
		}

		for (StreamingGeometry& page : streamingGeometry_)
		{
			if (!page.resident_)
			{
				continue;
			}
			heap->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
			heap->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
			heap->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
			heap->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);

			if (page.ownsVertices_)
			{
				heap->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
			}

			totalResidentGeometryBytes_ -= page.sizeBytes_;
			page.resident_ = false;
			++version_;
		}

		if (poolResident_)
		{
			heap->Release(std::move(poolResource_), poolBufferIndex_);
			totalResidentGeometryBytes_ -= poolSizeBytes_;
			poolResident_ = false;
			++version_;
		}

		for (StreamingTexture& streamingTexture : streamingTextures_)
		{
			if (streamingTexture.pinnedMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.pinnedMip_.sizeBytes_;
				heap->Release(std::move(streamingTexture.pinnedMip_.resource_), streamingTexture.pinnedMip_.bindlessIndex_);
			}
			if (streamingTexture.currentMip_.resource_)
			{
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
				heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
			}
		}

		heap->Release(std::move(vertexResource_), vertexBufferIndex_);
		heap->Release(std::move(skinVertexResource_), skinVertexBufferIndex_);
		heap->Release(std::move(indexResource_), indexBufferIndex_);
		heap->Release(std::move(positionResource_), positionBufferIndex_);
		heap->Release(std::move(raytracingSkinVertexResource_), raytracingSkinVertexBufferIndex_);
		heap->Release(std::move(raytracingMorphDeltaResource_), raytracingMorphDeltaBufferIndex_);
		heap->Release(std::move(morphDeltaResource_), morphDeltaBufferIndex_);
		heap->Release(std::move(vertexMorphSourceResource_), vertexMorphSourceBufferIndex_);

		uploaded_ = false;
	}

	void CristerResidency::Upload(Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap)
	{
		HRESULT hr{ S_OK };

		uploaded_ = true;

		subMeshPlacement_.assign(crister.geometry_.subMeshes_.size(), DynamicArray<Matrix>{});
		for (Size subMeshIndex = 0; subMeshIndex < crister.geometry_.subMeshes_.size(); subMeshIndex++)
		{
			const SubMesh& subMesh = crister.geometry_.subMeshes_[subMeshIndex];
			DynamicArray<Matrix>& placements = subMeshPlacement_[subMeshIndex];

			if (subMesh.skinIndex_ < 0 && subMesh.meshIndex_ >= 0)
			{
				for (const Node& node : crister.scene_.nodes_)
				{
					if (node.mesh_ == subMesh.meshIndex_)
					{
						placements.push_back(node.globalTransform_);
					}
				}
			}

			if (placements.empty())
			{
				placements.push_back(Matrix::Identity);
			}
		}

		streamingGeometry_.assign(crister.geometry_.clusters_.size(), StreamingGeometry{});
		poolVertexEnd_ = 0;
		for (Size clusterIndex = 0; clusterIndex < crister.geometry_.clusters_.size(); clusterIndex++)
		{
			const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];
			StreamingGeometry& page = streamingGeometry_[clusterIndex];
			if (cluster.meshletCount_ == 0)
			{
				continue;
			}

			const MeshletDesc& first = crister.geometry_.meshlets_[cluster.meshletOffset_];
			const MeshletDesc& last = crister.geometry_.meshlets_[cluster.meshletOffset_ + cluster.meshletCount_ - 1];
			page.vertexIndexBegin_ = first.vertexOffset_;
			page.vertexIndexEnd_ = last.vertexOffset_ + last.vertexCount_;
			page.primitiveBegin_ = first.triangleOffset_;
			page.primitiveEnd_ = last.triangleOffset_ + last.triangleCount_ * 3;
			page.ownsVertices_ = cluster.lodLevel_ >= 1;

			Uint32 minVertex = 0xFFFFFFFF;
			Uint32 maxVertex = 0;
			for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
			{
				minVertex = Min(minVertex, crister.geometry_.vertexIndices_[index]);
				maxVertex = Max(maxVertex, crister.geometry_.vertexIndices_[index]);
			}

			if (page.ownsVertices_)
			{
				page.vertexBegin_ = minVertex;
				page.vertexEnd_ = maxVertex + 1;
			}
			else
			{
				page.vertexBegin_ = 0;
				page.vertexEnd_ = maxVertex + 1;
				poolVertexEnd_ = Max(poolVertexEnd_, maxVertex + 1);
			}
		}

		for (const SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}
			streamingGeometry_[subMesh.clusterOffset_ + subMesh.clusterCount_ - 1].pinned_ = true;
			if (subMesh.skinIndex_ >= 0)
			{
				streamingGeometry_[subMesh.clusterOffset_].pinned_ = true;
			}
		}

		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_ && !streamingGeometry_[clusterIndex].ownsVertices_)
			{
				poolPinned_ = true;
			}
		}

		if (!crister.scene_.skins_.empty() && poolVertexEnd_ > 0)
		{
			hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.compressedSkinVertices_.data(), poolVertexEnd_, sizeof(CompressedSkinVertex), skinVertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "スキンバーテックスバッファの生成に失敗しました");
			skinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, skinVertexResource_.Get(), poolVertexEnd_, sizeof(CompressedSkinVertex));
		}

		CristerFlattener flattener;
		flattener.Flatten(crister, subMeshPlacement_);
		placedPositionMin_ = flattener.PlacedPositionMin();
		placedPositionExtent_ = flattener.PlacedPositionExtent();

		if (!flattener.TriangleIndices().empty())
		{
			hr = CreateStaticBufferUnbounded(device, lane, flattener.Vertices().data(), flattener.Vertices().size(), sizeof(CompressedVertex), vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用バーテックスバッファの生成に失敗しました");
			vertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexResource_.Get(), static_cast<Uint>(flattener.Vertices().size()), sizeof(CompressedVertex));
			proxyVertexCount_ = static_cast<Uint32>(flattener.Vertices().size());

			hr = CreateStaticBufferUnbounded(device, lane, flattener.Positions().data(), flattener.Positions().size(), sizeof(Vector3), positionResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用ポジションバッファの生成に失敗しました");
			positionBufferIndex_ = CreateStructuredShaderResourceView(device, heap, positionResource_.Get(), static_cast<Uint>(flattener.Positions().size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, lane, flattener.TriangleIndices().data(), flattener.TriangleIndices().size(), sizeof(Uint32), indexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "レイトレーシング用インデックスバッファの生成に失敗しました");
			triangleIndexCount_ = static_cast<Uint32>(flattener.TriangleIndices().size());
			indexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, indexResource_.Get(), triangleIndexCount_, sizeof(Uint32));

			if (flattener.SkinVerticesBuilt() && flattener.SkinVertices().size() == flattener.Vertices().size())
			{
				hr = CreateStaticBufferUnbounded(device, lane, flattener.SkinVertices().data(), flattener.SkinVertices().size(), sizeof(CompressedSkinVertex), raytracingSkinVertexResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用スキンバーテックスバッファの生成に失敗しました");
				raytracingSkinVertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingSkinVertexResource_.Get(), static_cast<Uint>(flattener.SkinVertices().size()), sizeof(CompressedSkinVertex));
			}

			if (!flattener.MorphDeltas().empty())
			{
				hr = CreateStaticBufferUnbounded(device, lane, flattener.MorphDeltas().data(), flattener.MorphDeltas().size(), sizeof(Vector3), raytracingMorphDeltaResource_.ReleaseAndGetAddressOf());
				SC_HR_CHECK(hr, "レイトレーシング用モーフデルタバッファの生成に失敗しました");
				raytracingMorphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, raytracingMorphDeltaResource_.Get(), static_cast<Uint>(flattener.MorphDeltas().size()), sizeof(Vector3));
			}
		}

		DynamicArray<Vector3> morphDeltas;
		for (SubMesh& subMesh : crister.geometry_.subMeshes_)
		{
			if (subMesh.morphs_.empty())
			{
				continue;
			}

			subMesh.morphDeltaOffset_ = static_cast<Uint32>(morphDeltas.size());
			for (const Morph& morph : subMesh.morphs_)
			{
				morphDeltas.insert(morphDeltas.end(), morph.positionDeltas_.begin(), morph.positionDeltas_.end());
			}
		}

		if (!morphDeltas.empty())
		{
			hr = CreateStaticBufferUnbounded(device, lane, morphDeltas.data(), morphDeltas.size(), sizeof(Vector3), morphDeltaResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "ラスタ用モーフデルタバッファの生成に失敗しました");
			morphDeltaBufferIndex_ = CreateStructuredShaderResourceView(device, heap, morphDeltaResource_.Get(), static_cast<Uint>(morphDeltas.size()), sizeof(Vector3));

			hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.vertexMorphSource_.data(), crister.geometry_.vertexMorphSource_.size(), sizeof(Uint32), vertexMorphSourceResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点モーフソースバッファの生成に失敗しました");
			vertexMorphSourceBufferIndex_ = CreateStructuredShaderResourceView(device, heap, vertexMorphSourceResource_.Get(), static_cast<Uint>(crister.geometry_.vertexMorphSource_.size()), sizeof(Uint32));
		}

		streamingTextures_.assign(crister.material_.bitmaps_.size(), StreamingTexture{});
		for (Size textureIndex = 0; textureIndex < crister.material_.bitmaps_.size(); textureIndex++)
		{
			const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];
			if (bitmap.width_ <= 0 || bitmap.height_ <= 0 || bitmap.cacheData_.empty() || bitmap.mipCount_ <= 0)
			{
				continue;
			}

			Uint32 streamableMipCount = 0;
			Int mipWidth = bitmap.width_;
			Int mipHeight = bitmap.height_;
			while (streamableMipCount < static_cast<Uint32>(bitmap.mipCount_) && mipWidth >= 4 && mipHeight >= 4 && mipWidth % 4 == 0 && mipHeight % 4 == 0)
			{
				streamableMipCount++;
				mipWidth /= 2;
				mipHeight /= 2;
			}
			if (streamableMipCount == 0)
			{
				SC_LOG_WARNING("テクスチャ {} の寸法({}x{})が BC7 の単一ミップリソースとして不正なため、ストリーミング対象外にします。", textureIndex, bitmap.width_, bitmap.height_);
				continue;
			}

			StreamingTexture& streamingTexture = streamingTextures_[textureIndex];
			streamingTexture.mipCount_ = streamableMipCount;

			streamingTexture.topResidentMip_ = streamingTexture.mipCount_;
			streamingTexture.valid_ = true;
		}

		if (!std::ranges::contains(streamingRegistry_, this))
		{
			streamingRegistry_.push_back(this);
		}
		for (Size clusterIndex = 0; clusterIndex < streamingGeometry_.size(); clusterIndex++)
		{
			if (streamingGeometry_[clusterIndex].pinned_)
			{
				MakeClusterResident(crister, device, lane, heap, static_cast<Uint32>(clusterIndex));
			}
		}

		for (Size textureIndex = 0; textureIndex < streamingTextures_.size(); textureIndex++)
		{
			if (streamingTextures_[textureIndex].valid_)
			{
				MakeTextureMipResident(crister, device, lane, heap, static_cast<Uint32>(textureIndex), streamingTextures_[textureIndex].mipCount_ - 1);
			}
		}
	}

	Vector3 CristerResidency::PlacedPositionMin()const
	{
		return placedPositionMin_;
	}

	Vector3 CristerResidency::PlacedPositionExtent()const
	{
		return placedPositionExtent_;
	}

	D3D12_GPU_VIRTUAL_ADDRESS CristerResidency::PositionBufferAddress()const
	{
		return positionResource_ ? positionResource_->GetGPUVirtualAddress() : 0;
	}

	Uint CristerResidency::PositionBufferIndex()const
	{
		return positionBufferIndex_;
	}

	Uint32 CristerResidency::VertexCount()const
	{
		return proxyVertexCount_;
	}

	D3D12_GPU_VIRTUAL_ADDRESS CristerResidency::IndexBufferAddress()const
	{
		return indexResource_ ? indexResource_->GetGPUVirtualAddress() : 0;
	}

	Uint32 CristerResidency::IndexCount()const
	{
		return triangleIndexCount_;
	}

	Uint CristerResidency::TextureBindlessIndex(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return SC_INVALID;
		}
		const StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_)
		{
			return SC_INVALID;
		}

		Bool isPinnedMip = streamingTexture.topResidentMip_ == streamingTexture.mipCount_ - 1;
		return isPinnedMip ? streamingTexture.pinnedMip_.bindlessIndex_ : streamingTexture.currentMip_.bindlessIndex_;
	}

	Bool CristerResidency::MaterialsDirty()const
	{
		return materialsDirty_;
	}

	void CristerResidency::ClearMaterialsDirty()const
	{
		materialsDirty_ = false;
	}

	Uint CristerResidency::VertexBufferIndex()const
	{
		return vertexBufferIndex_;
	}

	Uint CristerResidency::IndexBufferIndex()const
	{
		return indexBufferIndex_;
	}

	Uint CristerResidency::SkinVertexBufferIndex()const
	{
		return skinVertexBufferIndex_;
	}

	Uint CristerResidency::MorphDeltaBufferIndex()const
	{
		return morphDeltaBufferIndex_;
	}

	Uint CristerResidency::VertexMorphSourceBufferIndex()const
	{
		return vertexMorphSourceBufferIndex_;
	}

	Bool CristerResidency::ProxySkinned()const
	{
		return raytracingSkinVertexResource_ != nullptr;
	}

	Uint CristerResidency::ProxySkinVertexBufferIndex()const
	{
		return raytracingSkinVertexBufferIndex_;
	}

	Uint CristerResidency::ProxyMorphDeltaBufferIndex()const
	{
		return raytracingMorphDeltaBufferIndex_;
	}

	std::span<const Matrix> CristerResidency::SubMeshPlacement(Size subMeshIndex)const
	{
		static const DynamicArray<Matrix> identityPlacement = { Matrix::Identity };
		return subMeshIndex < subMeshPlacement_.size() ? subMeshPlacement_[subMeshIndex] : identityPlacement;
	}

	void CristerResidency::CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const
	{
		if (!positionResource_ || !destination || proxyVertexCount_ == 0)
		{
			return;
		}

		cmdList->CopyBufferRegion(destination, 0, positionResource_.Get(), 0, static_cast<UINT64>(proxyVertexCount_) * sizeof(Vector3));
	}

	void CristerResidency::MakeClusterResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		const Cluster& cluster = crister.geometry_.clusters_[clusterIndex];

		if (page.resident_ || cluster.meshletCount_ == 0 || !uploaded_)
		{
			return;
		}

		if (!page.ownsVertices_)
		{
			MakePoolResident(crister, device, lane, heap);
			poolResidentReferences_++;
		}

		DynamicArray<MeshletDesc> localMeshlets(cluster.meshletCount_);
		DynamicArray<MeshletBound> localBounds(cluster.meshletCount_);
		for (Uint32 meshletIndex = 0; meshletIndex < cluster.meshletCount_; meshletIndex++)
		{
			MeshletDesc meshlet = crister.geometry_.meshlets_[cluster.meshletOffset_ + meshletIndex];
			meshlet.vertexOffset_ -= page.vertexIndexBegin_;
			meshlet.triangleOffset_ -= page.primitiveBegin_;
			localMeshlets[meshletIndex] = meshlet;
			localBounds[meshletIndex] = crister.geometry_.meshletBounds_[cluster.meshletOffset_ + meshletIndex];
		}

		DynamicArray<Uint32> localVertexIndices(page.vertexIndexEnd_ - page.vertexIndexBegin_);
		for (Uint32 index = page.vertexIndexBegin_; index < page.vertexIndexEnd_; index++)
		{
			Uint32 value = crister.geometry_.vertexIndices_[index];
			localVertexIndices[index - page.vertexIndexBegin_] = page.ownsVertices_ ? value - page.vertexBegin_ : value;
		}

		Uint32 primitiveSize = page.primitiveEnd_ - page.primitiveBegin_;
		Uint32 alignedSize = (primitiveSize + 3) & ~3u;
		DynamicArray<Uint8> localPrimitives(alignedSize, 0);
		memcpy(localPrimitives.data(), crister.geometry_.primitiveIndices_.data() + page.primitiveBegin_, primitiveSize);

		DynamicArray<CompressedVertex> localVertices;
		if (page.ownsVertices_)
		{
			localVertices.resize(page.vertexEnd_ - page.vertexBegin_);
			for (Uint32 vertexIndex = page.vertexBegin_; vertexIndex < page.vertexEnd_; vertexIndex++)
			{
				localVertices[vertexIndex - page.vertexBegin_] = crister.geometry_.compressedVertices_[vertexIndex];
			}
		}

		HRESULT hr = CreateStaticBufferUnbounded(device, lane, localMeshlets.data(), localMeshlets.size(), sizeof(MeshletDesc), page.meshletResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバッファの生成に失敗しました");
		page.meshletBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.meshletResource_.Get(), cluster.meshletCount_, sizeof(MeshletDesc));

		hr = CreateStaticBufferUnbounded(device, lane, localBounds.data(), localBounds.size(), sizeof(MeshletBound), page.meshletBoundResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "メッシュレットバウンドバッファの生成に失敗しました");
		page.meshletBoundBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.meshletBoundResource_.Get(), cluster.meshletCount_, sizeof(MeshletBound));

		hr = CreateStaticBufferUnbounded(device, lane, localVertexIndices.data(), localVertexIndices.size(), sizeof(Uint32), page.vertexIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点インデックスバッファの生成に失敗しました");
		page.vertexIndicesBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.vertexIndicesResource_.Get(), static_cast<Uint>(localVertexIndices.size()), sizeof(Uint32));

		hr = CreateStaticBufferUnbounded(device, lane, localPrimitives.data(), alignedSize, 1, page.primitiveIndicesResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "プリミティブインデックスバッファの生成に失敗しました");
		{
			Uint index = heap->AllocateIndex();

			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Buffer.FirstElement = 0;
			shaderResourceViewDesc.Buffer.NumElements = alignedSize / 4;
			shaderResourceViewDesc.Buffer.StructureByteStride = 0;
			shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			device->CreateShaderResourceView(page.primitiveIndicesResource_.Get(), &shaderResourceViewDesc, heap->CPUHandle(index));

			page.primitiveIndicesBufferIndex_ = index;
		}

		if (page.ownsVertices_)
		{
			hr = CreateStaticBufferUnbounded(device, lane, localVertices.data(), localVertices.size(), sizeof(CompressedVertex), page.vertexResource_.ReleaseAndGetAddressOf());
			SC_HR_CHECK(hr, "頂点バッファの生成に失敗しました");
			page.vertexBufferIndex_ = CreateStructuredShaderResourceView(device, heap, page.vertexResource_.Get(), static_cast<Uint>(localVertices.size()), sizeof(CompressedVertex));
		}

		page.sizeBytes_ = static_cast<Uint64>(cluster.meshletCount_) * (sizeof(MeshletDesc) + sizeof(MeshletBound)) + localVertexIndices.size() * sizeof(Uint32) + alignedSize + localVertices.size() * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += page.sizeBytes_;
		page.resident_ = true;
		++version_;
	}

	void CristerResidency::MakeTextureMipResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 textureIndex, Uint32 targetMip)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		if (!streamingTexture.valid_ || !uploaded_)
		{
			return;
		}

		Uint32 mipIndex = Min(targetMip, streamingTexture.mipCount_ - 1);
		if (mipIndex >= streamingTexture.topResidentMip_)
		{
			return;
		}

		const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];

		Uint32 mipWidth = 0;
		Uint32 mipHeight = 0;
		Uint64 rowPitch = 0;
		Uint64 slicePitch = 0;
		Uint64 byteOffset = 0;
		ComputeTextureMipLayout(bitmap, mipIndex, mipWidth, mipHeight, rowPitch, slicePitch, byteOffset);

		D3D12_RESOURCE_DESC textureDesc{};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = mipWidth;
		textureDesc.Height = mipHeight;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Resource> newResource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(newResource.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
		{
			SC_LOG_WARNING("テクスチャ {} のミップ {} のアップロードに失敗しました(HRESULT=0x{:08X}, {}x{})。このミップ抜きで続行します。", textureIndex, mipIndex, static_cast<Uint32>(hr), mipWidth, mipHeight);
			return;
		}
#if SC_DEBUG
		newResource->SetName(L"Crister_TextureMip");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(newResource.Get());
#endif

		std::span<const std::byte> mipBytes = std::as_bytes(std::span(bitmap.cacheData_)).subspan(byteOffset, slicePitch);
		lane.GetStream().Upload(device, lane.GetCommandList(), newResource.Get(), 0, mipBytes);

		Uint newIndex = heap->AllocateIndex();
		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_BC7_UNORM;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		device->CreateShaderResourceView(newResource.Get(), &shaderResourceViewDesc, heap->CPUHandle(newIndex));

		if (mipIndex == streamingTexture.mipCount_ - 1)
		{
			streamingTexture.pinnedMip_.resource_ = newResource;
			streamingTexture.pinnedMip_.bindlessIndex_ = newIndex;
			streamingTexture.pinnedMip_.sizeBytes_ = slicePitch;
		}
		else
		{
			if (streamingTexture.currentMip_.resource_)
			{
				heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
				totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
			}
			streamingTexture.currentMip_.resource_ = newResource;
			streamingTexture.currentMip_.bindlessIndex_ = newIndex;
			streamingTexture.currentMip_.sizeBytes_ = slicePitch;
		}

		totalResidentTextureBytes_ += slicePitch;
		streamingTexture.topResidentMip_ = mipIndex;
		materialsDirty_ = true;
	}

	void CristerResidency::MakePoolResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap)
	{
		if (poolResident_ || poolVertexEnd_ == 0 || !uploaded_)
		{
			return;
		}

		HRESULT hr = CreateStaticBufferUnbounded(device, lane, crister.geometry_.compressedVertices_.data(), poolVertexEnd_, sizeof(CompressedVertex), poolResource_.ReleaseAndGetAddressOf());
		SC_HR_CHECK(hr, "頂点プールバッファの生成に失敗しました");
		poolBufferIndex_ = CreateStructuredShaderResourceView(device, heap, poolResource_.Get(), poolVertexEnd_, sizeof(CompressedVertex));

		poolSizeBytes_ = static_cast<Uint64>(poolVertexEnd_) * sizeof(CompressedVertex);
		totalResidentGeometryBytes_ += poolSizeBytes_;
		poolResident_ = true;
		++version_;
	}

	void CristerResidency::EvictCluster(BindlessHeap* heap, Uint32 clusterIndex)
	{
		StreamingGeometry& page = streamingGeometry_[clusterIndex];
		if (!page.resident_ || page.pinned_)
		{
			return;
		}

		heap->Release(std::move(page.meshletResource_), page.meshletBufferIndex_);
		heap->Release(std::move(page.meshletBoundResource_), page.meshletBoundBufferIndex_);
		heap->Release(std::move(page.vertexIndicesResource_), page.vertexIndicesBufferIndex_);
		heap->Release(std::move(page.primitiveIndicesResource_), page.primitiveIndicesBufferIndex_);
		if (page.ownsVertices_)
		{
			heap->Release(std::move(page.vertexResource_), page.vertexBufferIndex_);
		}
		else
		{
			poolResidentReferences_--;
		}

		totalResidentGeometryBytes_ -= page.sizeBytes_;
		page.sizeBytes_ = 0;
		page.resident_ = false;
		++version_;
	}

	void CristerResidency::EvictTextureMip(BindlessHeap* heap, Uint32 textureIndex)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}

		StreamingTexture& streamingTexture = streamingTextures_[textureIndex];

		if (!streamingTexture.valid_ || streamingTexture.topResidentMip_ >= streamingTexture.mipCount_ - 1)
		{
			return;
		}

		heap->Release(std::move(streamingTexture.currentMip_.resource_), streamingTexture.currentMip_.bindlessIndex_);
		streamingTexture.currentMip_.bindlessIndex_ = SC_INVALID;
		totalResidentTextureBytes_ -= streamingTexture.currentMip_.sizeBytes_;
		streamingTexture.currentMip_.sizeBytes_ = 0;
		streamingTexture.topResidentMip_ = streamingTexture.mipCount_ - 1;

		materialsDirty_ = true;
	}

	void CristerResidency::EvictPool(BindlessHeap* heap)
	{
		if (!poolResident_ || poolPinned_ || poolResidentReferences_ > 0)
		{
			return;
		}
		heap->Release(std::move(poolResource_), poolBufferIndex_);
		totalResidentGeometryBytes_ -= poolSizeBytes_;
		poolSizeBytes_ = 0;
		poolResident_ = false;
		++version_;
	}

	void CristerResidency::TouchCluster(Uint32 clusterIndex, Uint64 frame)
	{
		streamingGeometry_[clusterIndex].lastUsedFrame_ = frame;

		if (!streamingGeometry_[clusterIndex].ownsVertices_)
		{
			poolLastUsedFrame_ = frame;
		}
	}

	void CristerResidency::TouchTexture(Uint32 textureIndex, Uint64 frame)
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return;
		}
		streamingTextures_[textureIndex].lastUsedFrame_ = frame;
	}

	Bool CristerResidency::ClusterResident(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].resident_;
	}

	Uint64 CristerResidency::Version()const
	{
		return version_;
	}

	Bool CristerResidency::TextureResident(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return false;
		}

		return streamingTextures_[textureIndex].topResidentMip_ == 0;
	}

	void CristerResidency::EvictClusterBudget(BindlessHeap* heap, Uint64 currentFrame)
	{
		if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
		{
			return;
		}

		struct Candidate
		{
			CristerResidency* crister_;

			Uint32 clusterIndex_;

			Uint64 lastUsedFrame_;
		};

		DynamicArray<Candidate> candidates;
		for (CristerResidency* crister : streamingRegistry_)
		{
			for (Size clusterIndex = 0; clusterIndex < crister->streamingGeometry_.size(); clusterIndex++)
			{
				const StreamingGeometry& page = crister->streamingGeometry_[clusterIndex];
				if (page.resident_ && !page.pinned_ && page.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(clusterIndex), page.lastUsedFrame_ });
				}
			}
		}

		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
			{
				return a.lastUsedFrame_ < b.lastUsedFrame_;
			});

		for (const Candidate& candidate : candidates)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictCluster(heap, candidate.clusterIndex_);
		}

		for (CristerResidency* crister : streamingRegistry_)
		{
			if (totalResidentGeometryBytes_ <= geometryBudgetBytes_)
			{
				break;
			}
			if (crister->poolLastUsedFrame_ + evictAgeFrames_ <= currentFrame)
			{
				crister->EvictPool(heap);
			}
		}
	}

	void CristerResidency::EvictTextureBudget(BindlessHeap* heap, Uint64 currentFrame)
	{
		if (totalResidentTextureBytes_ <= textureBudgetBytes_)
		{
			return;
		}

		struct Candidate
		{
			CristerResidency* crister_;

			Uint32 textureIndex_;

			Uint64 lastUsedFrame_;
		};

		DynamicArray<Candidate> candidates;
		for (CristerResidency* crister : streamingRegistry_)
		{
			for (Size textureIndex = 0; textureIndex < crister->streamingTextures_.size(); textureIndex++)
			{
				const StreamingTexture& streamingTexture = crister->streamingTextures_[textureIndex];
				if (streamingTexture.valid_ && streamingTexture.topResidentMip_ < streamingTexture.mipCount_ - 1 && streamingTexture.lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
				{
					candidates.push_back({ crister, static_cast<Uint32>(textureIndex), streamingTexture.lastUsedFrame_ });
				}
			}
		}

		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
			{
				return a.lastUsedFrame_ < b.lastUsedFrame_;
			});

		for (const Candidate& candidate : candidates)
		{
			if (totalResidentTextureBytes_ <= textureBudgetBytes_)
			{
				break;
			}
			candidate.crister_->EvictTextureMip(heap, candidate.textureIndex_);
		}
	}

	Uint CristerResidency::ClusterVertexBufferIndex(Uint32 clusterIndex)const
	{
		const StreamingGeometry& page = streamingGeometry_[clusterIndex];

		return page.ownsVertices_ ? page.vertexBufferIndex_ : poolBufferIndex_;
	}

	Uint CristerResidency::ClusterMeshletBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBufferIndex_;
	}

	Uint CristerResidency::ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].meshletBoundBufferIndex_;
	}

	Uint CristerResidency::ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].vertexIndicesBufferIndex_;
	}

	Uint CristerResidency::ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].primitiveIndicesBufferIndex_;
	}

	Bool CristerResidency::StandaloneVertices(Uint32 clusterIndex)const
	{
		return streamingGeometry_[clusterIndex].ownsVertices_;
	}

	Uint32 CristerResidency::TextureFinestMip(Uint32 textureIndex)const
	{
		if (textureIndex >= streamingTextures_.size())
		{
			return 0;
		}
		return streamingTextures_[textureIndex].topResidentMip_;
	}

	Uint32 CristerResidency::TextureDesiredMip(const Crister& crister, Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const
	{
		if (textureIndex >= streamingTextures_.size() || !streamingTextures_[textureIndex].valid_)
		{
			return 0;
		}

		Float screenPixelsPerWorldUnit = worldScale * pixelsPerUnit;
		if (!(screenPixelsPerWorldUnit > 0.0f))
		{
			return 0;
		}

		const Bitmap& bitmap = crister.material_.bitmaps_[textureIndex];
		Float textureResolution = static_cast<Float>(Max(bitmap.width_, bitmap.height_));

		Float texelsPerWorldUnit = 0.0f;

		if (texcoordDensity > 0.0f)
		{
			texelsPerWorldUnit = texcoordDensity * textureResolution;
		}
		else
		{
			Vector3 extent = crister.geometry_.positionExtent_;
			Float modelSize = Max(Max(extent.x, extent.y), extent.z);
			texelsPerWorldUnit = textureResolution / Max(modelSize, 1e-6f);
		}

		Uint32 desiredMip = 0;
		Float texelsPerScreenPixel = texelsPerWorldUnit / screenPixelsPerWorldUnit;
		if (texelsPerScreenPixel > 1.0f)
		{
			desiredMip = static_cast<Uint32>(std::log2(texelsPerScreenPixel));
		}

		Uint32 mipCount = streamingTextures_[textureIndex].mipCount_;
		return mipCount == 0 ? 0 : Min(desiredMip, mipCount - 1);
	}

	Uint CristerResidency::CreateStructuredShaderResourceView(ID3D12Device* device, BindlessHeap* heap, ID3D12Resource* resource, Uint elementCount, Uint stride)
	{
		Uint index = heap->AllocateIndex();

		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shaderResourceViewDesc.Buffer.FirstElement = 0;
		shaderResourceViewDesc.Buffer.NumElements = elementCount;
		shaderResourceViewDesc.Buffer.StructureByteStride = stride;
		shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(resource, &shaderResourceViewDesc, heap->CPUHandle(index));

		return index;
	}

	HRESULT CristerResidency::CreateStaticBufferUnbounded(ID3D12Device* device, D3D12TransferLane& lane, const void* data, Uint64 count, Uint64 stride, ID3D12Resource** outResource)
	{
		const Uint64 sizeInBytes = count * stride;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(resource.GetAddressOf()));
		if (FAILED(hr))
		{
			return hr;
		}
#if SC_DEBUG
		resource->SetName(L"Crister_StaticBufferUnbounded");
		GFSDK_Aftermath_DX12_UpdateResourceInfo(resource.Get());
#endif

		lane.GetStream().Upload(device, lane.GetCommandList(), resource.Get(), 0, std::span<const std::byte>(static_cast<const std::byte*>(data), sizeInBytes));

		*outResource = resource.Detach();
		return S_OK;
	}

	void CristerResidency::ComputeTextureMipLayout(const Bitmap& bitmap, Uint32 mipIndex, Uint32& outWidth, Uint32& outHeight, Uint64& outRowPitch, Uint64& outSlicePitch, Uint64& outByteOffset)
	{
		Uint64 offset = 0;
		Int mipWidth = bitmap.width_;
		Int mipHeight = bitmap.height_;

		for (Uint32 mip = 0; mip < mipIndex; mip++)
		{
			Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
			Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
			offset += blockWidth * 16 * blockHeight;
			mipWidth = Max(1, mipWidth / 2);
			mipHeight = Max(1, mipHeight / 2);
		}

		Uint64 blockWidth = Max<Uint64>(1, (static_cast<Uint64>(mipWidth) + 3) / 4);
		Uint64 blockHeight = Max<Uint64>(1, (static_cast<Uint64>(mipHeight) + 3) / 4);
		outWidth = static_cast<Uint32>(mipWidth);
		outHeight = static_cast<Uint32>(mipHeight);
		outRowPitch = blockWidth * 16;
		outSlicePitch = outRowPitch * blockHeight;
		outByteOffset = offset;
	}
}
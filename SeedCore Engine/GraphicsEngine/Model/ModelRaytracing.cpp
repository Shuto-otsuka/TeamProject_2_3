#include <GraphicsEngine/Model/ModelRaytracing.h>
#include <GraphicsEngine/Model/ModelUnit.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Raytracing/BottomLevelShader.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderFrame.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <FoundationEngine/Log/DxFail.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	ModelRaytracing::ModelRaytracing()
	{
		/// No Code
	}

	ModelRaytracing::~ModelRaytracing() = default;

	void ModelRaytracing::Gather(const RenderScene& scene, RenderDevice& device, RenderFrame& frame)
	{
		updates_.clear();
		deforms_.clear();
		deformVertexCount_ = 0;
		submissionCount_ = 0;

		for (Proxy& proxy : proxies_)
		{
			proxy.seen_ = false;
			proxy.requested_ = false;
		}
		for (Slot& slot : slots_)
		{
			slot.seen_ = false;
		}

		World& world = *scene.world_;
		ModelResource& modelResource = *scene.resource_->modelResource_;
		BindlessHeap& heap = device.ResourceHeap();
		ID3D12Device* d3dDevice = device.Context().GetDevice();

		Query<Read<Active>, Read<Mesh>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Mesh& mesh)
			{
				if (!active.active_ || entityID.index_ >= ModelUnit::capacity_)
				{
					return;
				}
				Handle<Crister> cristerHandle = modelResource.GetHandle(mesh.meshID_);
				if (cristerHandle.empty())
				{
					return;
				}
				const Crister* crister = modelResource.Resolve(cristerHandle);
				if (!crister)
				{
					return;
				}
				const CristerResidency& residency = crister->Residency();
				if (residency.VertexCount() == 0 || residency.IndexCount() == 0 || residency.IndexCount() % 3 != 0 || residency.PositionBufferAddress() == 0 || residency.IndexBufferAddress() == 0 || residency.VertexBufferIndex() == SC_INVALID || residency.IndexBufferIndex() == SC_INVALID)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				Size proxyIndex = static_cast<Size>(cristerHandle.index_);
				if (proxies_.size() <= proxyIndex)
				{
					proxies_.resize(proxyIndex + 1);
				}
				Proxy& proxy = proxies_[proxyIndex];
				if (proxy.used_ && proxy.generation_ != cristerHandle.generation_)
				{
					heap.Release(std::move(proxy.surfaces_), proxy.surfaceBufferIndex_);
					proxy = Proxy{};
				}
				proxy.used_ = true;
				proxy.generation_ = cristerHandle.generation_;
				proxy.seen_ = true;
				D3D12_RAYTRACING_GEOMETRY_DESC& geometry = proxy.geometry_;
				if (geometry.Triangles.VertexBuffer.StartAddress != residency.PositionBufferAddress() || geometry.Triangles.IndexBuffer != residency.IndexBufferAddress() || geometry.Triangles.VertexCount != residency.VertexCount() || geometry.Triangles.IndexCount != residency.IndexCount())
				{
					Bool opaque = true;
					for (const Surface& surface : crister->Surfaces())
					{
						if (surface.alphaMode_ != 0)
						{
							opaque = false;
						}
					}
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Flags = opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
					geometry.Triangles.VertexBuffer.StartAddress = residency.PositionBufferAddress();
					geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vector3);
					geometry.Triangles.VertexCount = residency.VertexCount();
					geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					geometry.Triangles.IndexBuffer = residency.IndexBufferAddress();
					geometry.Triangles.IndexCount = residency.IndexCount();
					geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
					geometry.Triangles.Transform3x4 = 0;
					if (proxy.surfaces_)
					{
						heap.Release(std::move(proxy.surfaces_), proxy.surfaceBufferIndex_);
						proxy.surfaceBufferIndex_ = SC_INVALID;
					}
				}

				if (!proxy.surfaces_)
				{
					Uint32 triangleCount = residency.IndexCount() / 3;
					Uint32 surfaceCount = static_cast<Uint32>(crister->Surfaces().size());
					DynamicArray<Uint32> triangleSurfaces(triangleCount, 0);
					for (const SubMesh& subMesh : crister->SubMeshes())
					{
						Uint32 surfaceIndex = subMesh.surfaceIndex_ < surfaceCount ? subMesh.surfaceIndex_ : 0;
						Uint32 triangleEnd = Min(subMesh.raytracingTriangleOffset_ + subMesh.raytracingTriangleCount_, triangleCount);
						for (Uint32 triangleIndex = subMesh.raytracingTriangleOffset_;triangleIndex < triangleEnd;++triangleIndex)
						{
							triangleSurfaces[triangleIndex] = surfaceIndex;
						}
					}

					D3D12_HEAP_PROPERTIES heapProperties{};
					heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

					D3D12_RESOURCE_DESC resourceDesc{};
					resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					resourceDesc.Width = static_cast<Uint64>(triangleCount) * sizeof(Uint32);
					resourceDesc.Height = 1;
					resourceDesc.DepthOrArraySize = 1;
					resourceDesc.MipLevels = 1;
					resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
					resourceDesc.SampleDesc.Count = 1;
					resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

					HRESULT hr = d3dDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&proxy.surfaces_));
					SC_HR_CHECK(hr, "ModelRaytracing: 三角形のサーフェス表の生成に失敗しました");

					void* mapped = nullptr;
					D3D12_RANGE readRange{};
					hr = proxy.surfaces_->Map(0, &readRange, &mapped);
					SC_HR_CHECK(hr, "ModelRaytracing: 三角形のサーフェス表の Map に失敗しました");
					std::memcpy(mapped, triangleSurfaces.data(), static_cast<Size>(triangleCount) * sizeof(Uint32));
					proxy.surfaces_->Unmap(0, nullptr);

					proxy.surfaceBufferIndex_ = heap.AllocateIndex();
					D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
					shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.Buffer.NumElements = triangleCount;
					shaderResourceViewDesc.Buffer.StructureByteStride = sizeof(Uint32);
					d3dDevice->CreateShaderResourceView(proxy.surfaces_.Get(), &shaderResourceViewDesc, heap.CPUHandle(proxy.surfaceBufferIndex_));
				}

				Uint32 index = entityID.index_;
				if (slots_.size() <= index)
				{
					slots_.resize(static_cast<Size>(index) + 1);
				}
				Slot& slot = slots_[index];
				slot.used_ = true;
				slot.seen_ = true;

				BottomLevelRequest request;
				request.geometry_ = geometry;
				Bool skinned = residency.ProxySkinned();
				Bool morphed = residency.ProxyMorphDeltaBufferIndex() != SC_INVALID;
				if (actor.GetComponent<Animator>() && (skinned || morphed))
				{
					request.key_ = (2ull << 56) | index;
					request.dynamic_ = true;
					frame.Append(request);

					std::span<const SubMesh> subMeshes = crister->SubMeshes();
					for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
					{
						const SubMesh& subMesh = subMeshes[subMeshIndex];
						if (subMesh.raytracingVertexCount_ == 0)
						{
							continue;
						}
						Bool subMeshMorphed = morphed && !subMesh.morphs_.empty() && subMesh.raytracingMorphDeltaOffset_ != SC_INVALID;

						ModelDeformStructuredBuffer deform;
						deform.unitIndex_ = index;
						deform.subMeshIndex_ = static_cast<Uint>(subMeshIndex);
						deform.vertexOffset_ = subMesh.raytracingVertexOffset_;
						deform.vertexCount_ = subMesh.raytracingVertexCount_;
						deform.outputOffset_ = subMesh.raytracingVertexOffset_;
						deform.morphDeltaOffset_ = subMeshMorphed ? subMesh.raytracingMorphDeltaOffset_ : 0;
						deform.morphTargetCount_ = subMeshMorphed ? static_cast<Uint>(subMesh.morphs_.size()) : 0;
						deform.positionBufferIndex_ = residency.PositionBufferIndex();
						deform.skinVertexBufferIndex_ = skinned ? residency.ProxySkinVertexBufferIndex() : SC_INVALID;
						deform.morphDeltaBufferIndex_ = residency.ProxyMorphDeltaBufferIndex();
						deform.bottomLevel_ = request.key_;
						deforms_.push_back(deform);
						deformVertexCount_ = Max(deformVertexCount_, subMesh.raytracingVertexCount_);
					}
				}
				else
				{
					request.key_ = (1ull << 56) | ((cristerHandle.index_ & 0xFFFFFFFFull) << 24) | (cristerHandle.generation_ & 0xFFFFFFull);
					if (!proxy.requested_)
					{
						proxy.requested_ = true;
						frame.Append(request);
					}
				}

				ModelRaytracingStructuredBuffer row;
				row.bottomLevel_ = request.key_;
				row.vertexBufferIndex_ = residency.VertexBufferIndex();
				row.indexBufferIndex_ = residency.IndexBufferIndex();
				row.surfaceBufferIndex_ = proxy.surfaceBufferIndex_;
				row.texcoordMin_ = crister->TexcoordMin();
				row.texcoordExtent_ = crister->TexcoordExtent();
				++submissionCount_;

				if (!slot.sent_ || std::memcmp(&row, &slot.row_, sizeof(ModelRaytracingStructuredBuffer)) != 0)
				{
					ModelRaytracingUpdate update;
					update.index_ = index;
					update.data_ = row;
					updates_.push_back(update);
					slot.row_ = row;
					slot.sent_ = true;
				}
			});

		for (Proxy& proxy : proxies_)
		{
			if (!proxy.used_ || proxy.seen_)
			{
				continue;
			}
			heap.Release(std::move(proxy.surfaces_), proxy.surfaceBufferIndex_);
			proxy = Proxy{};
		}

		count_ = 0;
		for (Uint32 index = 0;index < static_cast<Uint32>(slots_.size());++index)
		{
			Slot& slot = slots_[index];
			if (slot.used_ && !slot.seen_)
			{
				slot.used_ = false;
			}
			if (slot.used_)
			{
				count_ = index + 1;
			}
		}

		for (Uint32 index = 0;index < count_;++index)
		{
			Slot& slot = slots_[index];
			if (slot.seen_)
			{
				continue;
			}
			ModelRaytracingStructuredBuffer row;
			if (!slot.sent_ || std::memcmp(&row, &slot.row_, sizeof(ModelRaytracingStructuredBuffer)) != 0)
			{
				ModelRaytracingUpdate update;
				update.index_ = index;
				update.data_ = row;
				updates_.push_back(update);
				slot.row_ = row;
				slot.sent_ = true;
			}
		}
	}

	std::span<const ModelRaytracingUpdate> ModelRaytracing::Updates()const
	{
		return updates_;
	}

	std::span<const ModelDeformStructuredBuffer> ModelRaytracing::Deforms()const
	{
		return deforms_;
	}

	Uint32 ModelRaytracing::DeformVertexCount()const
	{
		return deformVertexCount_;
	}

	Uint32 ModelRaytracing::Count()const
	{
		return count_;
	}

	Uint32 ModelRaytracing::SubmissionCount()const
	{
		return submissionCount_;
	}
}

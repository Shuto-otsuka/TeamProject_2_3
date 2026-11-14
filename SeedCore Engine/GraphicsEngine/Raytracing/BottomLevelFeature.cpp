#include <GraphicsEngine/Raytracing/BottomLevelFeature.h>
#include <GraphicsEngine/Raytracing/BottomLevelStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(BottomLevelFeature, UpdateStage, BuildStage);

	void BottomLevelFeature::Create(RenderDevice& device)
	{
		clearPipeline_.Create(device);
		updatePipeline_.Create(device);

		HRESULT hr = device.Context().GetDevice()->QueryInterface(IID_PPV_ARGS(&device_));
		SC_HR_CHECK(hr, "BottomLevel: ID3D12Device5 を取得できません");
		heap_ = &device.ResourceHeap();
		slots_.assign(tableCapacity_, SC_INVALID);
		marked_.assign(tableCapacity_, 0);
	}

	void BottomLevelFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		updates_.clear();
		staticBuilds_.clear();
		dynamicBuilds_.clear();
		for (Entry& entry : entries_)
		{
			entry.requested_ = false;
		}

		Uint32 vertexCount = 0;
		for (const BottomLevelRequest& request : frame.List<BottomLevelRequest>())
		{
			if (request.key_ == 0)
			{
				continue;
			}
			Uint32 handle = Acquire(request.key_, request.dynamic_);
			if (handle == SC_INVALID)
			{
				continue;
			}
			Entry& entry = entries_[handle];
			if (entry.requested_)
			{
				continue;
			}
			entry.requested_ = true;

			if (!entry.dynamic_)
			{
				if (std::memcmp(&entry.geometry_, &request.geometry_, sizeof(D3D12_RAYTRACING_GEOMETRY_DESC)) != 0)
				{
					entry.geometry_ = request.geometry_;
					entry.built_ = false;
				}
				continue;
			}

			Uint32 count = request.geometry_.Triangles.VertexCount;
			entry.vertexOffset_ = vertexCount + count <= vertexCapacity_ ? vertexCount : SC_INVALID;
			if (entry.vertexOffset_ != SC_INVALID)
			{
				vertexCount += count;
			}
			if (entry.geometry_.Triangles.VertexCount != count || entry.geometry_.Triangles.IndexCount != request.geometry_.Triangles.IndexCount || entry.geometry_.Triangles.IndexBuffer != request.geometry_.Triangles.IndexBuffer || entry.geometry_.Flags != request.geometry_.Flags)
			{
				entry.built_ = false;
			}
			entry.geometry_ = request.geometry_;
		}

		for (Uint32 handle = 0;handle < static_cast<Uint32>(entries_.size());++handle)
		{
			Entry& entry = entries_[handle];
			if (!entry.used_)
			{
				continue;
			}
			if (entry.requested_)
			{
				entry.unseenFrames_ = 0;
				continue;
			}
			++entry.unseenFrames_;
			if (entry.dynamic_ || entry.unseenFrames_ > 3)
			{
				Release(handle);
			}
		}

		Uint64 scratchSize = 0;
		for (Entry& entry : entries_)
		{
			if (!entry.used_ || !entry.requested_ || (!entry.dynamic_ && entry.built_) || (entry.dynamic_ && entry.vertexOffset_ == SC_INVALID))
			{
				continue;
			}

			Bool update = entry.dynamic_ && entry.built_;
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = entry.dynamic_ ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.Flags = flags;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &entry.geometry_;

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
			device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

			if (entry.capacity_ < prebuildInfo.ResultDataMaxSizeInBytes)
			{
				if (entry.resource_)
				{
					heap_->Release(std::move(entry.resource_), SC_INVALID);
				}

				D3D12_HEAP_PROPERTIES heapProperties{};
				heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

				D3D12_RESOURCE_DESC resourceDesc{};
				resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				resourceDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
				resourceDesc.Height = 1;
				resourceDesc.DepthOrArraySize = 1;
				resourceDesc.MipLevels = 1;
				resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
				resourceDesc.SampleDesc.Count = 1;
				resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

				HRESULT hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&entry.resource_));
				SC_HR_CHECK(hr, "BottomLevel: BLAS の生成に失敗しました");
				entry.capacity_ = prebuildInfo.ResultDataMaxSizeInBytes;
				update = false;
			}

			Build build;
			build.resource_ = entry.resource_.Get();
			build.geometry_ = entry.geometry_;
			build.flags_ = update ? flags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : flags;
			build.vertexOffset_ = entry.dynamic_ ? entry.vertexOffset_ : SC_INVALID;
			build.scratchOffset_ = scratchSize;
			Uint64 scratchBytes = update ? prebuildInfo.UpdateScratchDataSizeInBytes : prebuildInfo.ScratchDataSizeInBytes;
			scratchSize += (scratchBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) / D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT * D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
			if (entry.dynamic_)
			{
				dynamicBuilds_.push_back(build);
			}
			else
			{
				staticBuilds_.push_back(build);
			}
			entry.built_ = true;
		}

		if (scratchCapacity_ < scratchSize)
		{
			if (scratch_)
			{
				heap_->Release(std::move(scratch_), SC_INVALID);
			}

			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = scratchSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			HRESULT hr = device_->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&scratch_));
			SC_HR_CHECK(hr, "BottomLevel: BLAS のスクラッチの生成に失敗しました");
			scratch_->SetName(L"BottomLevel.Scratch");
			scratchCapacity_ = scratchSize;
		}

		for (Entry& entry : entries_)
		{
			if (!entry.used_)
			{
				continue;
			}
			Bool usable = entry.resource_ && (!entry.dynamic_ || entry.vertexOffset_ != SC_INVALID);
			Uint64 address = usable ? entry.resource_->GetGPUVirtualAddress() : 0;
			Uint32 vertexOffset = entry.dynamic_ && usable ? entry.vertexOffset_ : SC_INVALID;
			if (address != entry.sentAddress_ || vertexOffset != entry.sentVertexOffset_)
			{
				entry.sentAddress_ = address;
				entry.sentVertexOffset_ = vertexOffset;
				Mark(entry.slot_);
			}
		}

		for (Uint32 slot : markedSlots_)
		{
			BottomLevelTableUpdate update;
			update.slot_ = slot;
			if (slots_[slot] != SC_INVALID)
			{
				const Entry& entry = entries_[slots_[slot]];
				update.data_.key_ = entry.key_;
				update.data_.address_ = entry.sentAddress_;
				update.data_.vertexOffset_ = entry.sentVertexOffset_;
			}
			updates_.push_back(update);
			marked_[slot] = 0;
		}
		markedSlots_.clear();

		updating_ = clearing_ || !updates_.empty();
		buildingStatic_ = !staticBuilds_.empty();
		buildingDynamic_ = !dynamicBuilds_.empty();
		if (!updates_.empty())
		{
			upload.Write(TableUpdates, RenderViewKind::Game, std::span<const BottomLevelTableUpdate>(updates_));
		}
	}

	Uint32 BottomLevelFeature::Acquire(Uint64 key, Bool dynamic)
	{
		auto found = lookup_.find(key);
		if (found != lookup_.end())
		{
			if (entries_[found->second].dynamic_ == dynamic)
			{
				return found->second;
			}
			Release(found->second);
		}

		Uint32 handle = SC_INVALID;
		if (!freeHandles_.empty())
		{
			handle = freeHandles_.back();
			freeHandles_.pop_back();
		}
		else if (entries_.size() < capacity_)
		{
			handle = static_cast<Uint32>(entries_.size());
			entries_.emplace_back();
		}
		if (handle == SC_INVALID)
		{
			return SC_INVALID;
		}

		Entry& entry = entries_[handle];
		entry = Entry{};
		entry.key_ = key;
		entry.used_ = true;
		entry.dynamic_ = dynamic;
		lookup_[key] = handle;

		Uint32 hash = static_cast<Uint32>(key) * 0x9E3779B1u ^ static_cast<Uint32>(key >> 32) * 0x85EBCA77u;
		hash ^= hash >> 16;
		Uint32 slot = hash & (tableCapacity_ - 1);
		while (slots_[slot] != SC_INVALID)
		{
			slot = (slot + 1) & (tableCapacity_ - 1);
		}
		slots_[slot] = handle;
		entry.slot_ = slot;
		Mark(slot);
		return handle;
	}

	void BottomLevelFeature::Release(Uint32 handle)
	{
		Entry& entry = entries_[handle];
		if (entry.resource_)
		{
			heap_->Release(std::move(entry.resource_), SC_INVALID);
		}
		lookup_.erase(entry.key_);

		Uint32 hole = entry.slot_;
		slots_[hole] = SC_INVALID;
		Mark(hole);
		Uint32 next = (hole + 1) & (tableCapacity_ - 1);
		while (slots_[next] != SC_INVALID)
		{
			Uint32 moving = slots_[next];
			Uint64 key = entries_[moving].key_;
			Uint32 hash = static_cast<Uint32>(key) * 0x9E3779B1u ^ static_cast<Uint32>(key >> 32) * 0x85EBCA77u;
			hash ^= hash >> 16;
			Uint32 home = hash & (tableCapacity_ - 1);
			Bool move = hole <= next ? (home <= hole || home > next) : (home <= hole && home > next);
			if (move)
			{
				slots_[hole] = moving;
				entries_[moving].slot_ = hole;
				Mark(hole);
				slots_[next] = SC_INVALID;
				Mark(next);
				hole = next;
			}
			next = (next + 1) & (tableCapacity_ - 1);
		}

		entry = Entry{};
		freeHandles_.push_back(handle);
	}

	void BottomLevelFeature::Mark(Uint32 slot)
	{
		if (marked_[slot] != 0)
		{
			return;
		}
		marked_[slot] = 1;
		markedSlots_.push_back(slot);
	}
}

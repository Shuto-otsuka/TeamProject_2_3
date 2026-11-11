#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderGraphMemory.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	RenderGraph::RenderGraph(RenderDevice& device) :device_(device)
	{
		/// No Code
	}

	RenderGraph::~RenderGraph() = default;

	void RenderGraph::Initialize(Uint64 capacity)
	{
		upload_.Initialize(device_, capacity);

		ID3D12Device* d3dDevice = device_.Context().GetDevice();
		StaticArray<D3D12_SRV_DIMENSION, 8> dimensions =
		{
			D3D12_SRV_DIMENSION_TEXTURE2D,
			D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
			D3D12_SRV_DIMENSION_TEXTURECUBE,
			D3D12_SRV_DIMENSION_TEXTURECUBEARRAY,
			D3D12_SRV_DIMENSION_TEXTURE3D,
			D3D12_SRV_DIMENSION_BUFFER,
			D3D12_SRV_DIMENSION_UNKNOWN,
			D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
		};
		for (Size shapeIndex = 0;shapeIndex < nulls_.size();++shapeIndex)
		{
			nulls_[shapeIndex] = SC_INVALID;
			if (dimensions[shapeIndex] == D3D12_SRV_DIMENSION_UNKNOWN)
			{
				continue;
			}
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.ViewDimension = dimensions[shapeIndex];
			if (dimensions[shapeIndex] == D3D12_SRV_DIMENSION_BUFFER)
			{
				shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
				shaderResourceViewDesc.Buffer.StructureByteStride = sizeof(Uint32);
			}
			else if (dimensions[shapeIndex] != D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
			{
				shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
				shaderResourceViewDesc.Texture2DArray.ArraySize = 1;
			}
			nulls_[shapeIndex] = device_.ResourceHeap().AllocateIndex();
			d3dDevice->CreateShaderResourceView(nullptr, &shaderResourceViewDesc, device_.ResourceHeap().CPUHandle(nulls_[shapeIndex]));
		}

		RenderGraphResource fallback;
		fallback.resourceName_ = String("RenderGraph.Fallback");
		fallback.desc_ = RenderResourceDesc::FixedTexture(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1);
		fallback.shaderResource_ = true;
		fallback.renderTarget_ = true;
		RenderGraphInstance texel;
		texel.width_ = 1;
		texel.height_ = 1;
		for (RenderGraphAllocation& allocation : fallbacks_)
		{
			allocation = RenderGraphMemory::Create(device_, fallback, texel);
			allocation.state_ = D3D12_RESOURCE_STATE_COMMON;
		}

		for (ResourcePtr<StagingBuffer>& buffer : bindings_)
		{
			buffer = MakePtr<StagingBuffer>(d3dDevice, 1 << 20, D3D12_HEAP_TYPE_UPLOAD);
		}

		for (Uint32& index : scenes_)
		{
			index = device_.ResourceHeap().AllocateIndex();
		}

		profiler_.Create(device_);
	}

	void RenderGraph::Remove(const void* feature)
	{
		SeedCore::erase_if(resources_, [feature](const RenderGraphResource& resource) { return resource.feature_ == feature; });
		SeedCore::erase_if(stages_, [feature](const RenderGraphStage& stage) { return stage.feature_ == feature; });
		dirty_ = true;
	}

	void RenderGraph::Sort()
	{
		if (!dirty_)
		{
			return;
		}

		std::ranges::stable_sort(stages_, [](const RenderGraphStage& a, const RenderGraphStage& b) { return a.queue_ < b.queue_; });

		DynamicArray<RenderGraphStage> ordered;
		ordered.reserve(stages_.size());
		Size groupBegin = 0;
		while (groupBegin < stages_.size())
		{
			Size groupEnd = groupBegin;
			while (groupEnd < stages_.size() && stages_[groupEnd].queue_ == stages_[groupBegin].queue_)
			{
				++groupEnd;
			}
			Size count = groupEnd - groupBegin;

			DynamicArray<Bool> reach(count * count, false);
			for (Size fromIndex = 0;fromIndex < count;++fromIndex)
			{
				const RenderGraphStage& from = stages_[groupBegin + fromIndex];
				for (Size toIndex = 0;toIndex < count;++toIndex)
				{
					const RenderGraphStage& to = stages_[groupBegin + toIndex];
					if (fromIndex == toIndex)
					{
						continue;
					}
					if (from.featureName_ == to.featureName_)
					{
						reach[fromIndex * count + toIndex] = from.order_ < to.order_;
						continue;
					}
					for (const RenderGraphAccess& write : from.accesses_)
					{
						if (write.type_ != RenderAccessType::Write && write.type_ != RenderAccessType::Target && write.type_ != RenderAccessType::DepthWrite)
						{
							continue;
						}
						for (const RenderGraphAccess& read : to.accesses_)
						{
							if (read.resourceName_ == write.resourceName_ && (read.type_ == RenderAccessType::Read || read.type_ == RenderAccessType::Constant || read.type_ == RenderAccessType::DepthTest))
							{
								reach[fromIndex * count + toIndex] = true;
							}
						}
					}
				}
			}
			DynamicArray<Bool> edges = reach;

			for (Size middleIndex = 0;middleIndex < count;++middleIndex)
			{
				for (Size fromIndex = 0;fromIndex < count;++fromIndex)
				{
					for (Size toIndex = 0;toIndex < count;++toIndex)
					{
						if (reach[fromIndex * count + middleIndex] && reach[middleIndex * count + toIndex])
						{
							reach[fromIndex * count + toIndex] = true;
						}
					}
				}
			}

			for (Size index = 0;index < count;++index)
			{
				for (Size otherIndex = index + 1;otherIndex < count;++otherIndex)
				{
					const RenderGraphStage& stage = stages_[groupBegin + index];
					const RenderGraphStage& other = stages_[groupBegin + otherIndex];
					if (stage.featureName_ == other.featureName_ || reach[index * count + otherIndex] || reach[otherIndex * count + index])
					{
						continue;
					}
					for (const RenderGraphAccess& access : stage.accesses_)
					{
						for (const RenderGraphAccess& otherAccess : other.accesses_)
						{
							Bool write = access.type_ == RenderAccessType::Write || access.type_ == RenderAccessType::Target || access.type_ == RenderAccessType::DepthWrite;
							Bool otherWrite = otherAccess.type_ == RenderAccessType::Write || otherAccess.type_ == RenderAccessType::Target || otherAccess.type_ == RenderAccessType::DepthWrite;
							if (write && otherWrite && access.resourceName_ == otherAccess.resourceName_)
							{
								SC_LOG_ERROR("RenderGraph: {} と {} が同じ区切りで {} に書いていて、どちらが先か決まりません", stage.stageName_.c_str(), other.stageName_.c_str(), access.resourceName_.c_str());
							}
						}
					}
				}
			}

			DynamicArray<Uint32> incoming(count, 0);
			for (Size fromIndex = 0;fromIndex < count;++fromIndex)
			{
				for (Size toIndex = 0;toIndex < count;++toIndex)
				{
					if (edges[fromIndex * count + toIndex])
					{
						++incoming[toIndex];
					}
				}
			}
			DynamicArray<Bool> placed(count, false);
			for (Size step = 0;step < count;++step)
			{
				Size next = count;
				for (Size index = 0;index < count;++index)
				{
					if (!placed[index] && incoming[index] == 0)
					{
						next = index;
						break;
					}
				}
				if (next == count)
				{
					std::string cycle;
					for (Size index = 0;index < count;++index)
					{
						if (!placed[index])
						{
							cycle += cycle.empty() ? "" : ", ";
							cycle += stages_[groupBegin + index].stageName_.c_str();
						}
					}
					SC_LOG_ERROR("RenderGraph: 区切り {} の中で順番が循環しています（{}）", static_cast<Uint32>(stages_[groupBegin].queue_), cycle);
					for (Size index = 0;index < count;++index)
					{
						if (!placed[index])
						{
							placed[index] = true;
							ordered.push_back(std::move(stages_[groupBegin + index]));
						}
					}
					break;
				}
				placed[next] = true;
				ordered.push_back(std::move(stages_[groupBegin + next]));
				for (Size toIndex = 0;toIndex < count;++toIndex)
				{
					if (edges[next * count + toIndex])
					{
						--incoming[toIndex];
					}
				}
			}
			groupBegin = groupEnd;
		}
		stages_ = std::move(ordered);

		for (RenderGraphResource& resource : resources_)
		{
			resource.shaderResource_ = false;
			resource.unorderedAccess_ = false;
			resource.renderTarget_ = false;
			resource.depthStencil_ = false;
			resource.history_ = false;
			resource.passThroughIndex_ = SC_INVALID;
			if ((resource.fallback_ == RenderFallback::Zero && resource.desc_.shape_ == RenderResourceShape::Constant) || ((resource.fallback_ == RenderFallback::One || resource.fallback_ == RenderFallback::Alpha) && resource.desc_.shape_ != RenderResourceShape::Texture2D))
			{
				SC_LOG_ERROR("RenderGraph: {} の形では、このフォールバックは使えません", resource.resourceName_.c_str());
			}
			if (resource.passThroughName_.view().empty())
			{
				continue;
			}
			for (Size resourceIndex = 0;resourceIndex < resources_.size();++resourceIndex)
			{
				if (resources_[resourceIndex].resourceName_ == resource.passThroughName_)
				{
					resource.passThroughIndex_ = static_cast<Uint32>(resourceIndex);
				}
			}
			if (resource.passThroughIndex_ == SC_INVALID)
			{
				SC_LOG_ERROR("RenderGraph: {} の素通し元 {} が宣言されていません", resource.resourceName_.c_str(), resource.passThroughName_.c_str());
			}
		}

		for (RenderGraphStage& stage : stages_)
		{
			for (RenderGraphAccess& access : stage.accesses_)
			{
				access.index_ = SC_INVALID;
				for (Size resourceIndex = 0;resourceIndex < resources_.size();++resourceIndex)
				{
					if (resources_[resourceIndex].resourceName_ == access.resourceName_)
					{
						access.index_ = static_cast<Uint32>(resourceIndex);
					}
				}
				if (access.index_ == SC_INVALID)
				{
					SC_LOG_ERROR("RenderGraph: {} が宣言されていないリソース {} を使っています", stage.stageName_.c_str(), access.resourceName_.c_str());
					continue;
				}
				if (stage.viewMask_ == RenderViewMask::Frame && resources_[access.index_].scope_ == RenderResourceScope::View)
				{
					SC_LOG_ERROR("RenderGraph: Frame の {} が View 範囲の {} を使っています", stage.stageName_.c_str(), access.resourceName_.c_str());
				}
				RenderGraphResource& resource = resources_[access.index_];
				resource.shaderResource_ = resource.shaderResource_ || access.type_ == RenderAccessType::Read || access.type_ == RenderAccessType::History;
				resource.unorderedAccess_ = resource.unorderedAccess_ || access.type_ == RenderAccessType::Write;
				resource.renderTarget_ = resource.renderTarget_ || access.type_ == RenderAccessType::Target;
				resource.depthStencil_ = resource.depthStencil_ || access.type_ == RenderAccessType::DepthWrite || access.type_ == RenderAccessType::DepthTest;
				resource.history_ = resource.history_ || access.type_ == RenderAccessType::History;
			}
		}

		for (Size stageIndex = 0;stageIndex < stages_.size();++stageIndex)
		{
			for (const RenderGraphAccess& access : stages_[stageIndex].accesses_)
			{
				if (access.index_ == SC_INVALID || (access.type_ != RenderAccessType::Read && access.type_ != RenderAccessType::Constant && access.type_ != RenderAccessType::DepthTest))
				{
					continue;
				}
				Bool writtenBefore = false;
				Bool writtenAfter = false;
				for (Size otherIndex = 0;otherIndex < stages_.size();++otherIndex)
				{
					for (const RenderGraphAccess& other : stages_[otherIndex].accesses_)
					{
						if (other.index_ != access.index_ || (other.type_ != RenderAccessType::Write && other.type_ != RenderAccessType::Target && other.type_ != RenderAccessType::DepthWrite))
						{
							continue;
						}
						if (otherIndex < stageIndex)
						{
							writtenBefore = true;
						}
						if (otherIndex > stageIndex)
						{
							writtenAfter = true;
						}
					}
				}
				if (!writtenBefore && writtenAfter)
				{
					SC_LOG_WARNING("RenderGraph: {} が {} を、書く Stage より前で読んでいます", stages_[stageIndex].stageName_.c_str(), access.resourceName_.c_str());
				}
			}
		}

		dirty_ = false;
	}

	void RenderGraph::Compile(const RenderScene& scene)
	{
		++frame_;
		for (RenderGraphAllocation& allocation : uploads_)
		{
			RenderGraphMemory::Release(device_, allocation);
		}
		uploads_.clear();
		imports_.clear();
		passes_.clear();
		for (Size stageIndex = 0;stageIndex < stages_.size();++stageIndex)
		{
			const RenderGraphStage& stage = stages_[stageIndex];
			if (stage.viewMask_ == RenderViewMask::Frame)
			{
				if (stage.enabled_(stage.feature_, RenderViewKind::Game))
				{
					passes_.push_back(RenderGraphPass{ static_cast<Uint32>(stageIndex), SC_INVALID });
				}
				continue;
			}
			for (Size viewIndex = 0;viewIndex < scene.views_.size();++viewIndex)
			{
				Uint32 viewBit = 1u << static_cast<Uint32>(scene.views_[viewIndex].kind_);
				if ((static_cast<Uint32>(stage.viewMask_) & viewBit) != 0 && stage.enabled_(stage.feature_, scene.views_[viewIndex].kind_))
				{
					passes_.push_back(RenderGraphPass{ static_cast<Uint32>(stageIndex), static_cast<Uint32>(viewIndex) });
				}
			}
		}

		instances_.clear();
		instances_.resize(resources_.size() * RenderViewCount);
		for (Size resourceIndex = 0;resourceIndex < resources_.size();++resourceIndex)
		{
			const RenderGraphResource& resource = resources_[resourceIndex];
			const RenderResourceDesc& desc = resource.desc_;
			Size instanceCount = resource.scope_ == RenderResourceScope::View ? scene.views_.size() : 1;
			for (Size viewIndex = 0;viewIndex < instanceCount;++viewIndex)
			{
				RenderGraphInstance& instance = instances_[resourceIndex * RenderViewCount + viewIndex];
				instance.resource_ = static_cast<Uint32>(resourceIndex);
				instance.view_ = resource.scope_ == RenderResourceScope::View ? static_cast<Uint32>(viewIndex) : SC_INVALID;
				instance.width_ = desc.width_;
				instance.height_ = desc.height_;
				instance.depth_ = desc.depth_;
				if (desc.scale_ != RenderResourceScale::Fixed && resource.scope_ == RenderResourceScope::View)
				{
					const RenderView& view = scene.views_[viewIndex];
					Uint32 baseWidth = desc.scale_ == RenderResourceScale::Render ? view.renderWidth_ : view.displayWidth_;
					Uint32 baseHeight = desc.scale_ == RenderResourceScale::Render ? view.renderHeight_ : view.displayHeight_;
					instance.width_ = (baseWidth + desc.divisor_ - 1) / desc.divisor_;
					Uint32 heightDivisor = desc.heightDivisor_ != 0 ? desc.heightDivisor_ : desc.divisor_;
					instance.height_ = (baseHeight + heightDivisor - 1) / heightDivisor;
				}
				instance.count_ = desc.shape_ == RenderResourceShape::Buffer && desc.scale_ != RenderResourceScale::Fixed ? instance.width_ * instance.height_ * desc.depth_ * desc.count_ : desc.count_;
			}
		}

		for (const RenderGraphPass& pass : passes_)
		{
			for (const RenderGraphAccess& access : stages_[pass.stage_].accesses_)
			{
				if (access.index_ == SC_INVALID)
				{
					continue;
				}
				Uint32 view = resources_[access.index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
				RenderGraphInstance& instance = instances_[access.index_ * RenderViewCount + view];
				instance.accessed_ = true;
				instance.written_ = instance.written_ || access.type_ == RenderAccessType::Write || access.type_ == RenderAccessType::Target || access.type_ == RenderAccessType::DepthWrite;
			}
		}

		for (RenderGraphInstance& instance : instances_)
		{
			if (instance.resource_ == SC_INVALID)
			{
				continue;
			}
			const RenderGraphResource& resource = resources_[instance.resource_];
			if (instance.written_ || resource.lifetime_ == RenderResourceLifetime::Imported || resource.lifetime_ == RenderResourceLifetime::Upload || resource.lifetime_ == RenderResourceLifetime::Persistent)
			{
				continue;
			}
			if (resource.passThroughIndex_ != SC_INVALID)
			{
				Uint32 view = resources_[resource.passThroughIndex_].scope_ == RenderResourceScope::View ? instance.view_ : 0;
				instance.target_ = resource.passThroughIndex_ * RenderViewCount + view;
				continue;
			}
			instance.fallback_ = resource.fallback_;
		}

		for (const RenderGraphInstance& instance : instances_)
		{
			Uint32 target = instance.target_;
			while (instance.accessed_ && target != SC_INVALID)
			{
				instances_[target].accessed_ = true;
				target = instances_[target].target_;
			}
		}

		for (Size allocationIndex = pool_.size();allocationIndex > 0;--allocationIndex)
		{
			RenderGraphAllocation& allocation = pool_[allocationIndex - 1];
			allocation.idleFrames_ = allocation.used_ ? 0 : allocation.idleFrames_ + 1;
			allocation.used_ = false;
			if (allocation.idleFrames_ > 2)
			{
				RenderGraphMemory::Release(device_, allocation);
				pool_.erase(pool_.begin() + (allocationIndex - 1));
			}
		}

		for (Size slot = resources_.size() * RenderViewCount * 2;slot < persistent_.size();++slot)
		{
			RenderGraphMemory::Release(device_, persistent_[slot]);
		}
		persistent_.resize(resources_.size() * RenderViewCount * 2);

		for (Size instanceIndex = 0;instanceIndex < instances_.size();++instanceIndex)
		{
			RenderGraphInstance& instance = instances_[instanceIndex];
			if (instance.resource_ == SC_INVALID || !instance.accessed_ || instance.target_ != SC_INVALID || instance.fallback_ != RenderFallback::None)
			{
				continue;
			}
			const RenderGraphResource& resource = resources_[instance.resource_];
			if (resource.lifetime_ == RenderResourceLifetime::Transient)
			{
				Uint32 found = SC_INVALID;
				for (Size allocationIndex = 0;allocationIndex < pool_.size();++allocationIndex)
				{
					if (!pool_[allocationIndex].used_ && RenderGraphMemory::Matches(pool_[allocationIndex], resource, instance))
					{
						found = static_cast<Uint32>(allocationIndex);
						break;
					}
				}
				if (found == SC_INVALID)
				{
					found = static_cast<Uint32>(pool_.size());
					pool_.push_back(RenderGraphMemory::Create(device_, resource, instance));
				}
				pool_[found].used_ = true;
				instance.allocation_ = found;
				continue;
			}
			if (resource.lifetime_ == RenderResourceLifetime::Persistent)
			{
				Uint32 current = static_cast<Uint32>(instanceIndex * 2 + (resource.history_ ? (frame_ & 1) : 0));
				Uint32 previous = static_cast<Uint32>(instanceIndex * 2 + ((frame_ + 1) & 1));
				for (Uint32 slot : { current, previous })
				{
					if (slot == previous && !resource.history_)
					{
						continue;
					}
					if (!persistent_[slot].resource_ || !RenderGraphMemory::Matches(persistent_[slot], resource, instance))
					{
						RenderGraphMemory::Release(device_, persistent_[slot]);
						persistent_[slot] = RenderGraphMemory::Create(device_, resource, instance);
					}
				}
				instance.allocation_ = current;
				instance.history_ = resource.history_ ? previous : SC_INVALID;
				continue;
			}
			RenderViewKind kind = instance.view_ != SC_INVALID ? scene.views_[instance.view_].kind_ : RenderViewKind::Game;
			if (resource.lifetime_ == RenderResourceLifetime::Upload)
			{
				const RenderUploadEntry* entry = upload_.Find(resource.resourceName_, kind);
				if (!entry)
				{
					instance.fallback_ = resource.fallback_;
					continue;
				}
				instance.allocation_ = static_cast<Uint32>(uploads_.size());
				uploads_.push_back(RenderGraphMemory::Create(device_, resource, upload_.Buffer(), *entry));
				continue;
			}
			if (resource.lifetime_ == RenderResourceLifetime::Imported)
			{
				for (const RenderGraphImport& imported : scene.imports_)
				{
					if (imported.resourceName_ == resource.resourceName_ && imported.view_ == kind)
					{
						instance.allocation_ = static_cast<Uint32>(imports_.size());
						imports_.push_back(imported.allocation_);
						break;
					}
				}
				if (instance.allocation_ == SC_INVALID)
				{
					instance.fallback_ = resource.fallback_;
				}
			}
		}

		for (RenderGraphInstance& instance : instances_)
		{
			if (instance.resource_ == SC_INVALID || instance.allocation_ == SC_INVALID)
			{
				continue;
			}
			RenderResourceLifetime lifetime = resources_[instance.resource_].lifetime_;
			if (lifetime == RenderResourceLifetime::Transient)
			{
				instance.memory_ = &pool_[instance.allocation_];
			}
			else if (lifetime == RenderResourceLifetime::Persistent)
			{
				instance.memory_ = &persistent_[instance.allocation_];
			}
			else if (lifetime == RenderResourceLifetime::Upload)
			{
				instance.memory_ = &uploads_[instance.allocation_];
			}
			else
			{
				instance.memory_ = &imports_[instance.allocation_];
			}
		}

		for (RenderGraphInstance& instance : instances_)
		{
			Uint32 target = instance.target_;
			while (target != SC_INVALID && instances_[target].target_ != SC_INVALID)
			{
				target = instances_[target].target_;
			}
			if (target != SC_INVALID)
			{
				instance.memory_ = instances_[target].memory_;
				instance.fallback_ = instances_[target].fallback_;
			}
		}

		for (RenderGraphAllocation& allocation : pool_)
		{
			allocation.writer_ = SC_INVALID;
			allocation.readers_ = { SC_INVALID, SC_INVALID };
		}

		for (RenderGraphAllocation& allocation : persistent_)
		{
			allocation.writer_ = SC_INVALID;
			allocation.readers_ = { SC_INVALID, SC_INVALID };
		}

		for (RenderGraphAllocation& allocation : imports_)
		{
			allocation.writer_ = SC_INVALID;
			allocation.readers_ = { SC_INVALID, SC_INVALID };
		}

		opening_.clear();
		StaticArray<Uint32, 2> waited = { SC_INVALID, SC_INVALID };
		for (Uint32 passIndex = 0;passIndex < passes_.size();++passIndex)
		{
			RenderGraphPass& pass = passes_[passIndex];
			const RenderGraphStage& stage = stages_[pass.stage_];
			Uint32 queue = stage.async_ ? 1 : 0;
			Bool pixel = stage.kind_ == RenderStageKind::Raster || stage.kind_ == RenderStageKind::Callback;
			Bool copy = stage.kind_ == RenderStageKind::Copy;
			for (const RenderGraphAccess& access : stage.accesses_)
			{
				if (access.index_ == SC_INVALID)
				{
					continue;
				}
				const RenderGraphResource& resource = resources_[access.index_];
				Uint32 view = resource.scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
				const RenderGraphInstance& instance = instances_[access.index_ * RenderViewCount + view];
				RenderGraphAllocation* memory = instance.memory_;
				if (access.type_ == RenderAccessType::History)
				{
					memory = instance.history_ != SC_INVALID ? &persistent_[instance.history_] : nullptr;
				}
				if (!memory || resource.lifetime_ == RenderResourceLifetime::Upload)
				{
					continue;
				}

				D3D12_RESOURCE_STATES required = D3D12_RESOURCE_STATE_DEPTH_READ;
				if (access.type_ == RenderAccessType::Read || access.type_ == RenderAccessType::History)
				{
					required = copy ? D3D12_RESOURCE_STATE_COPY_SOURCE : pixel ? D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
					if (resource.desc_.shape_ == RenderResourceShape::Buffer && !copy)
					{
						required |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
					}
				}
				else if (access.type_ == RenderAccessType::Constant)
				{
					required = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				}
				else if (access.type_ == RenderAccessType::Write)
				{
					required = copy ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				}
				else if (access.type_ == RenderAccessType::Target)
				{
					required = D3D12_RESOURCE_STATE_RENDER_TARGET;
				}
				else if (access.type_ == RenderAccessType::DepthWrite)
				{
					required = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				}
				if (resource.desc_.shape_ == RenderResourceShape::AccelerationStructure)
				{
					required = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
				}

				Bool write = access.type_ == RenderAccessType::Write || access.type_ == RenderAccessType::Target || access.type_ == RenderAccessType::DepthWrite;
				D3D12_RESOURCE_STATES readable = D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_DEPTH_READ;
				D3D12_RESOURCE_STATES computable = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
				Bool satisfied = memory->state_ == required;
				if (!satisfied && memory->state_ != D3D12_RESOURCE_STATE_COMMON && (memory->state_ & ~readable) == 0 && (required & ~readable) == 0)
				{
					satisfied = (memory->state_ & required) == required;
					required |= memory->state_;
				}
				if (satisfied)
				{
					if (memory->state_ == D3D12_RESOURCE_STATE_UNORDERED_ACCESS || memory->state_ == D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE)
					{
						D3D12_RESOURCE_BARRIER barrier{};
						barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
						barrier.UAV.pResource = memory->resource_.Get();
						pass.barriers_.push_back(barrier);
					}
				}
				else
				{
					D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { memory->resource_.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, memory->state_, required } };
					if (stage.async_ && ((memory->state_ | required) & ~computable) != 0)
					{
						Uint32 owner = memory->readers_[0];
						if (memory->writer_ != SC_INVALID && !stages_[passes_[memory->writer_].stage_].async_ && (owner == SC_INVALID || memory->writer_ > owner))
						{
							owner = memory->writer_;
						}
						if (memory->readers_[1] != SC_INVALID && (owner == SC_INVALID || memory->readers_[1] > owner))
						{
							pass.handoff_.push_back(barrier);
						}
						else if (owner != SC_INVALID)
						{
							passes_[owner].release_.push_back(barrier);
						}
						else
						{
							opening_.push_back(barrier);
						}
					}
					else
					{
						pass.barriers_.push_back(barrier);
					}
					memory->state_ = required;
					write = true;
				}

				Uint32 dependency = SC_INVALID;
				if (memory->writer_ != SC_INVALID && stages_[passes_[memory->writer_].stage_].async_ != stage.async_)
				{
					dependency = memory->writer_;
				}
				if (write && memory->readers_[1 - queue] != SC_INVALID && (dependency == SC_INVALID || memory->readers_[1 - queue] > dependency))
				{
					dependency = memory->readers_[1 - queue];
				}
				if (dependency != SC_INVALID && (pass.wait_ == SC_INVALID || dependency > pass.wait_))
				{
					pass.wait_ = dependency;
				}
				if (write)
				{
					memory->writer_ = passIndex;
					memory->readers_ = { SC_INVALID, SC_INVALID };
				}
				else
				{
					memory->readers_[queue] = passIndex;
				}
			}

			if (!pass.handoff_.empty())
			{
				pass.wait_ = SC_INVALID;
				waited[queue] = passIndex;
			}
			else if (pass.wait_ != SC_INVALID && waited[queue] != SC_INVALID && pass.wait_ <= waited[queue])
			{
				pass.wait_ = SC_INVALID;
			}
			else if (pass.wait_ != SC_INVALID)
			{
				waited[queue] = pass.wait_;
				passes_[pass.wait_].signal_ = true;
			}
		}

		closing_.clear();
		for (RenderGraphAllocation& allocation : imports_)
		{
			for (const RenderGraphImport& imported : scene.imports_)
			{
				if (imported.allocation_.resource_ == allocation.resource_ && imported.allocation_.state_ != allocation.state_)
				{
					closing_.push_back(D3D12_RESOURCE_BARRIER{ D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { allocation.resource_.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, allocation.state_, imported.allocation_.state_ } });
					allocation.state_ = imported.allocation_.state_;
					break;
				}
			}
		}

		StagingBuffer& bindings = *bindings_[frame_ & 1];
		bindings.Reset();

		StagingBuffer& uploadBuffer = upload_.Buffer();
		ID3D12Device* d3dDevice = device_.Context().GetDevice();
		Uint64 sceneSize = (sizeof(SceneConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		for (Size viewIndex = 0;viewIndex < scene.views_.size();++viewIndex)
		{
			const RenderView& view = scene.views_[viewIndex];
			std::optional<Uint64> sceneOffset = uploadBuffer.Allocate(sceneSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			std::optional<Uint64> viewOffset = uploadBuffer.Allocate(sizeof(ViewBindings), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			if (!sceneOffset || !viewOffset)
			{
				SC_LOG_ERROR("RenderGraph: ビューのバインディングの容量が足りません");
				break;
			}
			uploadBuffer.Write(*sceneOffset, std::as_bytes(std::span<const SceneConstantBuffer>(&view.scene_, 1)));

			ViewBindings viewBindings;
			viewBindings.scene_ = scenes_[(frame_ & 1) * RenderViewCount + viewIndex];
			viewBindings.frame_ = SC_INVALID;
			viewBindings.view_ = static_cast<Uint32>(view.kind_);
			viewBindings.sampler_ = 0;

			D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc{};
			constantBufferViewDesc.BufferLocation = uploadBuffer.Address() + *sceneOffset;
			constantBufferViewDesc.SizeInBytes = static_cast<Uint32>(sceneSize);
			d3dDevice->CreateConstantBufferView(&constantBufferViewDesc, device_.ResourceHeap().CPUHandle(viewBindings.scene_));

			bindings.Write(*viewOffset, std::as_bytes(std::span<const ViewBindings>(&viewBindings, 1)));
			views_[viewIndex] = bindings.Address() + *viewOffset;
		}

		for (RenderGraphPass& pass : passes_)
		{
			const RenderGraphStage& stage = stages_[pass.stage_];
			std::optional<Uint64> offset = bindings.Allocate(sizeof(Uint32) * stage.accesses_.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
			if (!offset)
			{
				SC_LOG_ERROR("RenderGraph: バインディングの容量（{} バイト）が足りません", bindings.Size());
				break;
			}
			pass.bindings_ = bindings.Address() + *offset;
			Uint64 cursor = *offset;
			for (const RenderGraphAccess& access : stage.accesses_)
			{
				if (access.type_ == RenderAccessType::Target || access.type_ == RenderAccessType::DepthWrite || access.type_ == RenderAccessType::DepthTest)
				{
					continue;
				}
				Uint32 index = SC_INVALID;
				if (access.index_ != SC_INVALID)
				{
					const RenderGraphResource& resource = resources_[access.index_];
					Uint32 view = resource.scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
					const RenderGraphInstance& instance = instances_[access.index_ * RenderViewCount + view];
					const RenderGraphAllocation* memory = access.type_ == RenderAccessType::History && instance.history_ != SC_INVALID ? &persistent_[instance.history_] : instance.memory_;
					if (instance.fallback_ == RenderFallback::Zero)
					{
						index = nulls_[static_cast<Size>(resource.desc_.shape_)];
					}
					else if (instance.fallback_ == RenderFallback::One || instance.fallback_ == RenderFallback::Alpha)
					{
						index = fallbacks_[static_cast<Size>(instance.fallback_) - 2].shaderResourceView_;
					}
					else if (memory)
					{
						index = access.type_ == RenderAccessType::Write ? memory->unorderedAccessView_ : memory->shaderResourceView_;
					}
				}
				bindings.Write(cursor, std::as_bytes(std::span<const Uint32>(&index, 1)));
				cursor += sizeof(Uint32);
			}
		}

		upload_.Reset(frame_);
	}

	void RenderGraph::Execute(const RenderScene& scene)
	{
		D3D12Context& context = device_.Context();
		D3D12CommandList& direct = *context.GetDirectList();
		StaticArray<D3D12CommandList*, 2> lists = { &direct, context.GetComputeList() };
		StaticArray<Bool, 2> fresh = { true, true };
		profiler_.Read();

		for (Size fallbackIndex = 0;fallbackIndex < fallbacks_.size();++fallbackIndex)
		{
			RenderGraphAllocation& fallback = fallbacks_[fallbackIndex];
			if (fallback.state_ != D3D12_RESOURCE_STATE_COMMON)
			{
				continue;
			}
			Float value = fallbackIndex == 0 ? 1.0f : 0.0f;
			StaticArray<Float, 4> color = { value,value,value,1.0f };
			direct.Barrier(fallback.resource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			direct.Get()->ClearRenderTargetView(device_.RenderTargetHeap().CPUHandle(fallback.renderTargetView_), color.data(), 0, nullptr);
			direct.Barrier(fallback.resource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
			fallback.state_ = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		}

		if (!opening_.empty())
		{
			direct.Get()->ResourceBarrier(static_cast<Uint32>(opening_.size()), opening_.data());
		}

		Uint64 transferFenceValue = context.GetFrameTransferLane()->Submit(*context.GetCopyQueue());
		context.GetDirectQueue()->Wait(*context.GetCopyQueue(), transferFenceValue);
		Uint64 startFenceValue = context.SubmitDirect();
		context.GetComputeQueue()->Wait(*context.GetDirectQueue(), startFenceValue);
		context.GetFrameTransferLane()->Begin(context.GetDevice(), *context.GetCopyQueue());

		StaticArray<D3D12CommandQueue*, 2> queues = { context.GetDirectQueue(), context.GetComputeQueue() };
		StaticArray<ID3D12DescriptorHeap*, 2> heaps = { device_.ResourceHeap().Heap(), device_.SamplerHeap().Heap() };
		Uint32 frameView = 0;
		for (Size viewIndex = 0;viewIndex < scene.views_.size();++viewIndex)
		{
			if (scene.views_[viewIndex].kind_ == RenderViewKind::Game)
			{
				frameView = static_cast<Uint32>(viewIndex);
			}
		}

		Bool async = false;
		for (Uint32 passIndex = 0;passIndex < passes_.size();++passIndex)
		{
			RenderGraphPass& pass = passes_[passIndex];
			const RenderGraphStage& stage = stages_[pass.stage_];
			Uint32 queue = stage.async_ ? 1 : 0;
			D3D12CommandList& list = *lists[queue];
			async = async || stage.async_;

			if (!pass.handoff_.empty())
			{
				direct.Get()->ResourceBarrier(static_cast<Uint32>(pass.handoff_.size()), pass.handoff_.data());
				context.SubmitCompute();
				Uint64 handoffFenceValue = context.SubmitDirect();
				queues[1]->Wait(*queues[0], handoffFenceValue);
				fresh = { true, true };
			}
			if (pass.wait_ != SC_INVALID)
			{
				if (queue == 0)
				{
					context.SubmitDirect();
				}
				else
				{
					context.SubmitCompute();
				}
				queues[queue]->Wait(*queues[1 - queue], passes_[pass.wait_].fence_);
				fresh[queue] = true;
			}

			if (fresh[queue])
			{
				list.Get()->SetDescriptorHeaps(static_cast<Uint32>(heaps.size()), heaps.data());
				list.Get()->SetComputeRootSignature(device_.RootSignature().Signature());
				if (queue == 0)
				{
					list.Get()->SetGraphicsRootSignature(device_.RootSignature().Signature());
				}
				fresh[queue] = false;
			}

			if (!pass.barriers_.empty())
			{
				list.Get()->ResourceBarrier(static_cast<Uint32>(pass.barriers_.size()), pass.barriers_.data());
			}

			if (stage.kind_ == RenderStageKind::Raster)
			{
				StaticArray<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT> targets{};
				Uint32 targetCount = 0;
				D3D12_CPU_DESCRIPTOR_HANDLE depth{};
				Bool hasDepth = false;
				Uint32 width = 0;
				Uint32 height = 0;
				for (const RenderGraphAccess& access : stage.accesses_)
				{
					if (access.index_ == SC_INVALID || (access.type_ != RenderAccessType::Target && access.type_ != RenderAccessType::DepthWrite && access.type_ != RenderAccessType::DepthTest))
					{
						continue;
					}
					Uint32 view = resources_[access.index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
					const RenderGraphAllocation* memory = instances_[access.index_ * RenderViewCount + view].memory_;
					if (!memory)
					{
						continue;
					}
					if (access.type_ == RenderAccessType::Target)
					{
						targets[targetCount] = device_.RenderTargetHeap().CPUHandle(memory->renderTargetView_);
						++targetCount;
					}
					else
					{
						depth = device_.DepthStencilHeap().CPUHandle(access.type_ == RenderAccessType::DepthWrite ? memory->depthStencilView_ : memory->depthReadOnlyView_);
						hasDepth = true;
					}
					if (width == 0)
					{
						width = memory->width_;
						height = memory->height_;
					}
				}
				list.Get()->OMSetRenderTargets(targetCount, targets.data(), FALSE, hasDepth ? &depth : nullptr);
				if (width != 0)
				{
					D3D12_VIEWPORT viewport{};
					viewport.Width = static_cast<Float>(width);
					viewport.Height = static_cast<Float>(height);
					viewport.MaxDepth = 1.0f;
					D3D12_RECT scissor{};
					scissor.right = static_cast<LONG>(width);
					scissor.bottom = static_cast<LONG>(height);
					list.Get()->RSSetViewports(1, &viewport);
					list.Get()->RSSetScissorRects(1, &scissor);
				}
			}

			D3D12_GPU_VIRTUAL_ADDRESS viewAddress = views_[pass.view_ != SC_INVALID ? pass.view_ : frameView];
			if (stage.kind_ == RenderStageKind::Raster || stage.kind_ == RenderStageKind::Callback)
			{
				list.Get()->SetGraphicsRootConstantBufferView(static_cast<Uint32>(RootParameter::Stage), pass.bindings_);
				list.Get()->SetGraphicsRootConstantBufferView(static_cast<Uint32>(RootParameter::View), viewAddress);
			}
			else if (stage.kind_ != RenderStageKind::Copy)
			{
				list.Get()->SetComputeRootConstantBufferView(static_cast<Uint32>(RootParameter::Stage), pass.bindings_);
				list.Get()->SetComputeRootConstantBufferView(static_cast<Uint32>(RootParameter::View), viewAddress);
			}

			RenderViewMask profileView = pass.view_ != SC_INVALID ? static_cast<RenderViewMask>(1u << static_cast<Uint32>(scene.views_[pass.view_].kind_)) : RenderViewMask::Frame;
			RenderPass renderPass(*this, list, passIndex, pass.view_ != SC_INVALID ? &scene.views_[pass.view_] : nullptr);
			profiler_.Begin(list, passIndex, stage, profileView);
			stage.execute_(stage.feature_, renderPass);
			profiler_.End(list, passIndex);

			if (!pass.release_.empty())
			{
				list.Get()->ResourceBarrier(static_cast<Uint32>(pass.release_.size()), pass.release_.data());
			}
			if (stage.kind_ == RenderStageKind::Callback)
			{
				fresh[queue] = true;
			}
			if (pass.signal_)
			{
				pass.fence_ = queue == 0 ? context.SubmitDirect() : context.SubmitCompute();
				fresh[queue] = true;
			}
		}

		if (async)
		{
			Uint64 computeFenceValue = context.SubmitCompute();
			queues[0]->Wait(*queues[1], computeFenceValue);
		}
		if (!closing_.empty())
		{
			direct.Get()->ResourceBarrier(static_cast<Uint32>(closing_.size()), closing_.data());
		}
		imports_.clear();
		profiler_.Resolve(direct, static_cast<Uint32>(passes_.size()));
	}

	RenderUpload& RenderGraph::Upload()
	{
		return upload_;
	}

	const GpuProfiler& RenderGraph::Profiler()const
	{
		return profiler_;
	}
}
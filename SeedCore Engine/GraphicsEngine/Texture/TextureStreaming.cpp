#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	TextureStreaming::TextureStreaming(TextureResource& textureResource, D3D12Context& context, BindlessHeap& heap) :textureResource_(textureResource), context_(context), heap_(heap)
	{
		/// No Code
	}

	Texture* TextureStreaming::Resolve(const Handle<Texture>& handle, Uint64 frame)
	{
		Texture* texture = textureResource_.Resolve(handle);
		if (!texture)
		{
			return nullptr;
		}

		texture->lastUsedFrame_ = frame;

		if (!texture->resource_ && !texture->filePath_.view().empty())
		{
			texture->textureIndex_ = heap_.AllocateIndex();
			TextureCreator::CreateTexturePath(context_.GetDevice(), context_.GetCopyQueue(), context_.GetLoadTransferLane(LoadThread::Main), heap_.Heap(), texture->filePath_, texture->resource_, texture->textureIndex_);
			if (texture->resource_)
			{
				D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
				texture->sizeBytes_ = context_.GetDevice()->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
			}
		}

		return texture;
	}

	Uint32 TextureStreaming::GetID(Uint32 assetID, Uint64 frame)
	{
		Handle<Texture> handle = textureResource_.GetHandle(assetID);
		if (handle.empty())
		{
			return SC_INVALID;
		}

		Texture* texture = Resolve(handle, frame);
		if (!texture || !texture->Resource())
		{
			return SC_INVALID;
		}

		return texture->ShaderResourceViewIndex();
	}

	void TextureStreaming::EvictBudget(Uint64 currentFrame)
	{
		struct Candidate
		{
			Texture* texture_;

			Uint64 lastUsedFrame_;
		};

		Uint64 residentBytes = 0;
		DynamicArray<Candidate> candidates;
		for (const Handle<Texture>& handle : textureResource_.assetHandleMap_ | std::ranges::views::values)
		{
			Texture* texture = textureResource_.Resolve(handle);
			if (!texture || !texture->resource_)
			{
				continue;
			}

			residentBytes += texture->sizeBytes_;
			if (!texture->pinned_ && texture->lastUsedFrame_ + evictAgeFrames_ <= currentFrame)
			{
				candidates.push_back({ texture, texture->lastUsedFrame_ });
			}
		}

		if (residentBytes <= budgetBytes_)
		{
			return;
		}

		std::ranges::sort(candidates, [](const Candidate& a, const Candidate& b)
		{
			return a.lastUsedFrame_ < b.lastUsedFrame_;
		});

		for (const Candidate& candidate : candidates)
		{
			if (residentBytes <= budgetBytes_)
			{
				break;
			}

			Texture* texture = candidate.texture_;
			heap_.Release(std::move(texture->resource_), texture->textureIndex_);
			texture->textureIndex_ = SC_INVALID;
			residentBytes -= texture->sizeBytes_;
			texture->sizeBytes_ = 0;
		}
	}
}

#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <GraphicsEngine/Texture/TextureResource.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	/**
	* [EN]
	* Takes the texture manager whose textures are streamed, and the
	* device context and heap an evicted texture is recreated with.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* テクスチャをストリーミングする対象のマネージャと、追い出された
	* テクスチャを作り直すデバイスのコンテキストとヒープを受け取る。
	*/
	TextureStreaming::TextureStreaming(TextureResource& textureResource, D3D12Context& context, BindlessHeap& heap) :textureResource_(textureResource), context_(context), heap_(heap)
	{
		/// No Code
	}

	/**
	* [EN]
	* Resolves a handle for drawing on frame: marks the texture used on
	* that frame and recreates it from its file when it had been
	* evicted. Returns nullptr when the handle is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* frame で描画に使うハンドルを解決する。そのフレームで使ったと記録し、
	* 追い出されていればファイルから作り直す。ハンドルが古ければ nullptr
	* を返す。
	*/
	Texture* TextureStreaming::Resolve(const Handle<Texture>& handle, Uint64 frame)
	{
		Texture* texture = textureResource_.Resolve(handle);
		if (!texture)
		{
			return nullptr;
		}

		texture->lastUsedFrame_ = frame;

		/// [EN] An evicted texture has no resource but still knows its file, so it is brought back the moment it is drawn again.
		/// [JP] 追い出されたテクスチャはリソースを持たないがファイルの位置は覚えているため、再び描かれた時点で戻す。
		if (!texture->resource_ && !texture->filePath_.view().empty())
		{
			texture->textureIndex_ = heap_.AllocateIndex();
			TextureCreator::CreateTexturePath(context_.GetDevice(), context_.GetDirectQueue(), heap_.Heap(), texture->filePath_, texture->resource_, texture->textureIndex_);
			if (texture->resource_)
			{
				D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
				texture->sizeBytes_ = context_.GetDevice()->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
			}
		}

		return texture;
	}

	/**
	* [EN]
	* The bindless index of the texture registered under assetID, for
	* drawing on frame, or SC_INVALID when there is none or it could not
	* be made resident.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* assetID で登録されたテクスチャの、frame で描画に使うバインドレスの
	* 番号。無い、または常駐させられなかった場合は SC_INVALID。
	*/
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

	/**
	* [EN]
	* Releases resident textures, longest-unused first, until the total
	* fits the budget. A texture used within the last few frames is
	* kept even over budget, since it is likely to be drawn again at
	* once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 合計が予算に収まるまで、常駐しているテクスチャを最も長く使われて
	* いないものから解放する。直近数フレームで使ったテクスチャは、すぐまた
	* 描かれる可能性が高いため、予算を超えていても残す。
	*/
	void TextureStreaming::EvictBudget(Uint64 currentFrame)
	{
		/// [EN] One texture that may be evicted, with the frame it was last drawn on.
		/// [JP] 追い出してよいテクスチャ1つと、それが最後に描かれたフレーム。
		struct Candidate
		{
			/// [EN] Texture that may be evicted.
			/// [JP] 追い出してよいテクスチャ。
			Texture* texture_;

			/// [EN] Frame the texture was last drawn on.
			/// [JP] テクスチャが最後に描かれたフレーム。
			Uint64 lastUsedFrame_;
		};

		/// [EN] The resident total is summed afresh each frame from the textures themselves, so it can never drift from what is actually allocated.
		/// [JP] 常駐量の合計はフレームごとにテクスチャ自身から数え直す。実際に確保されている量からずれることが無いようにするため。
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

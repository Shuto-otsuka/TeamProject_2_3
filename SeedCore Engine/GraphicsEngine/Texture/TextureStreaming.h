#pragma once
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	class TextureResource;
	class BindlessHeap;
	class D3D12Context;

	class SEEDCORE_API TextureStreaming :public NonCopyable
	{
	public:
		TextureStreaming(TextureResource& textureResource, D3D12Context& context, BindlessHeap& heap);
		~TextureStreaming() = default;

		Texture* Resolve(const Handle<Texture>& handle, Uint64 frame);

		Uint32 GetID(Uint32 assetID, Uint64 frame);

		void EvictBudget(Uint64 currentFrame);

	private:
		TextureResource& textureResource_;

		D3D12Context& context_;

		BindlessHeap& heap_;

		Uint64 budgetBytes_ = 128ull * 1024 * 1024;

		SC_CONSTEX Uint64 evictAgeFrames_ = 8;
	};
}

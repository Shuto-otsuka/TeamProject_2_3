#pragma once
#include <GraphicsEngine/Texture/Texture.h>
#include <FoundationEngine/Resource/Asset/AssetLoader.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12Context;

	class SEEDCORE_API TextureLoader :public AssetLoader<Texture>
	{
	private:
		friend class TextureResource;

	public:
		TextureLoader(D3D12Context& context, BindlessHeap& heap);
		~TextureLoader() = default;

		Handle<Texture> Load(String filePath, LoadThread thread);

		Handle<Texture> Adopt(Texture&& texture);

		void Clear(Handle<Texture>& handle)noexcept;

	private:
		Bool Bake(String sourcePath, String cachePath);

	private:
		D3D12Context& context_;

		BindlessHeap& heap_;
	};
}

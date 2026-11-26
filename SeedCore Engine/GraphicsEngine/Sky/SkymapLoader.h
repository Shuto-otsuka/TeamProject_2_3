#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Sky/Skymap.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12Context;

	class SkymapLoader :public AssetLoader<Skymap>
	{
	private:
		friend class SkymapResource;

	public:
		SkymapLoader(D3D12Context& context, BindlessHeap& heap);
		~SkymapLoader() = default;

		Handle<Skymap> Load(String filePath, LoadThread thread);

		void Clear(Handle<Skymap>& handle)noexcept;

	private:
		Bool Bake(String sourcePath, String cachePath);

	private:
		D3D12Context& context_;

		BindlessHeap& heap_;
	};
}

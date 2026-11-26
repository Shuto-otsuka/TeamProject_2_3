#include <GraphicsEngine/System/FontSystem.h>
#include <GraphicsEngine/Font/Font.h>
#include <GraphicsEngine/Font/FontResource.h>

namespace SeedCore
{
	void FontSystem::Update(FontResource& fontResource, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* bindlessHeap)
	{
		for (const Handle<Font>& handle : fontResource.assetHandleMap_ | std::ranges::views::values)
		{
			Font* font = fontResource.Resolve(handle);
			if (!font)
			{
				continue;
			}

			font->Update();
			font->UploadAtlas(device, lane, bindlessHeap);
		}
	}
}

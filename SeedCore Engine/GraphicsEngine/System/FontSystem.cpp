#include <GraphicsEngine/System/FontSystem.h>
#include <GraphicsEngine/Font/Font.h>
#include <GraphicsEngine/Font/FontResource.h>

namespace SeedCore
{
	/**
	* [EN]
	* Fills and uploads the atlas of every font fontResource holds,
	* through cmdQueue into bindlessHeap.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* fontResource が持つ全フォントのアトラスを埋め、cmdQueue を通して
	* bindlessHeap へアップロードする。
	*/
	void FontSystem::Update(FontResource& fontResource, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap)
	{
		/// [EN] Every loaded font is advanced, not only those a Text component names, since glyphs can be requested from outside the world as well.
		/// [JP] Text コンポーネントが指すものに限らず、読み込み済みの全フォントを進める。グリフは world の外からも求められることがあるため。
		for (const Handle<Font>& handle : fontResource.assetHandleMap_ | std::ranges::views::values)
		{
			Font* font = fontResource.Resolve(handle);
			if (!font)
			{
				continue;
			}

			font->Update();
			font->UploadAtlas(device, cmdQueue, bindlessHeap);
		}
	}
}

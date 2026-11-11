#include <GraphicsEngine/Graphics/GraphicsDisplay.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the shader-visible handle of view's output texture.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* view の出力テクスチャの、シェーダー可視ハンドルを返す。
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE GraphicsDisplay::View(RenderViewKind view)const
	{
		return views_[static_cast<Size>(view)];
	}
}

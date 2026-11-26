#include <GraphicsEngine/Graphics/GraphicsDisplay.h>

namespace SeedCore
{
	D3D12_GPU_DESCRIPTOR_HANDLE GraphicsDisplay::View(RenderViewKind view)const
	{
		return views_[static_cast<Size>(view)];
	}
}

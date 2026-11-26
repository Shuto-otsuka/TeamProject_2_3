#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	class SEEDCORE_API GraphicsDisplay
	{
	public:
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE View(RenderViewKind view)const;

	private:
		friend class GraphicsScene;

		StaticArray<D3D12_GPU_DESCRIPTOR_HANDLE, RenderViewCount> views_ = {};
	};
}

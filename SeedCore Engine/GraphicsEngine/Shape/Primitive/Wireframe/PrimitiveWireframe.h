#pragma once
#include <GraphicsEngine/Shape/Primitive/PrimitiveShader.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <FoundationEngine/Interop/ShapeInstance.h>

namespace SeedCore
{
	class PrimitiveWireframe :public NonTransferable
	{
	public:
		PrimitiveWireframe();

		~PrimitiveWireframe();

		void Gather(const ShapeList& list);

	public:
		NODISCARD std::span<const PrimitiveWireframeStructuredBuffer> Instances(RenderViewKind view)const;

	private:
		StaticArray<DynamicArray<PrimitiveWireframeStructuredBuffer>, RenderViewCount> instances_;
	};
}

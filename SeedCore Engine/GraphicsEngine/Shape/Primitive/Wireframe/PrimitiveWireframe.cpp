#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.h>

namespace SeedCore
{
	PrimitiveWireframe::PrimitiveWireframe()
	{
		/// No Code
	}

	PrimitiveWireframe::~PrimitiveWireframe() = default;

	void PrimitiveWireframe::Gather(const ShapeList& list)
	{
		for (DynamicArray<PrimitiveWireframeStructuredBuffer>& instances : instances_)
		{
			instances.clear();
		}

		for (std::span<const ShapeDesc> shapes : { list.shapes_, list.colliders_ })
		{
			for (const ShapeDesc& shape : shapes)
			{
				if (shape.style_ != ShapeStyle::Wireframe)
				{
					continue;
				}

				PrimitiveWireframeStructuredBuffer wireframe;
				wireframe.position_ = shape.position_;
				wireframe.rotation_ = shape.rotation_;
				wireframe.dimensions_ = shape.dimensions_;
				wireframe.color_ = shape.color_;
				wireframe.shapeKind_ = static_cast<Uint32>(shape.kind_);
				wireframe.headLength_ = shape.headLength_;
				wireframe.lineWidth_ = shape.lineWidth_;
				if (shape.space_ == ShapeSpace::Canvas)
				{
					instances_[static_cast<Size>(RenderViewKind::Canvas)].push_back(wireframe);
					continue;
				}
				instances_[static_cast<Size>(RenderViewKind::Editor)].push_back(wireframe);
				if (shape.scope_ == ShapeScope::Game)
				{
					instances_[static_cast<Size>(RenderViewKind::Game)].push_back(wireframe);
				}
			}
		}
	}

	std::span<const PrimitiveWireframeStructuredBuffer> PrimitiveWireframe::Instances(RenderViewKind view)const
	{
		return instances_[static_cast<Size>(view)];
	}
}

#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	void RenderInstance::Add(const ShapeDesc& shape)
	{
		shapes_.push_back(shape);
	}

	std::span<const ShapeDesc> RenderInstance::Shapes()const
	{
		return shapes_;
	}

	void RenderInstance::Clear()
	{
		shapes_.clear();
	}
}

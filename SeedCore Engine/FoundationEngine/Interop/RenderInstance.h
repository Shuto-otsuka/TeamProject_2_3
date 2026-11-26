#pragma once
#include <FoundationEngine/Interop/ShapeInstance.h>

namespace SeedCore
{
	class SEEDCORE_API RenderInstance
	{
	public:
		void Add(const ShapeDesc& shape);

		NODISCARD std::span<const ShapeDesc> Shapes()const;

		void Clear();

	private:
		DynamicArray<ShapeDesc> shapes_;
	};
}

#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* Records one shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形を1つ記録する。
	*/
	void RenderInstance::Add(const ShapeDesc& shape)
	{
		shapes_.push_back(shape);
	}

	/**
	* [EN]
	* Returns the shapes recorded since the last Clear.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の Clear から記録した形を返す。
	*/
	std::span<const ShapeDesc> RenderInstance::Shapes()const
	{
		return shapes_;
	}

	/**
	* [EN]
	* Forgets every recorded shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 記録した形をすべて捨てる。
	*/
	void RenderInstance::Clear()
	{
		shapes_.clear();
	}
}

#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ShapeInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* The debug shapes requested since it was last cleared, owned by World.
	* DebugDraw adds each shape as it is requested, and whoever draws them
	* reads the list and clears it. Sits in FoundationEngine so World can own
	* it alongside QueryInstance.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回空にしてから頼まれたデバッグ用の形の記録。World が所有する。
	* DebugDraw が頼まれるたびに足し、描く側が読んで空にする。QueryInstance と
	* 並べて World が持てるよう、FoundationEngine に置く。
	*/
	class SEEDCORE_API RenderInstance
	{
	public:
		/**
		* [EN]
		* Records one shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 形を1つ記録する。
		*/
		void Add(const ShapeDesc& shape);

		/**
		* [EN]
		* Returns the shapes recorded since the last Clear.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の Clear から記録した形を返す。
		*/
		[[nodiscard]] std::span<const ShapeDesc> Shapes()const;

		/**
		* [EN]
		* Forgets every recorded shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 記録した形をすべて捨てる。
		*/
		void Clear();

	private:
		/// [EN] Recorded shapes, oldest first.
		/// [JP] 記録した形。古い順。
		DynamicArray<ShapeDesc> shapes_;
	};
}

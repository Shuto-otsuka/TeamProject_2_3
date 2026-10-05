#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ColliderInstance.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Gathers the World's colliders into ColliderDesc entries for the
	* collider wireframe, once per frame before rendering reads them. Knows
	* the collider components and how each one is scaled; knows nothing
	* about the GPU, which is ColliderRenderer's side.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World のコライダーを、コライダーのワイヤーフレーム用の ColliderDesc に
	* 集める。描画が読む前に、フレームに1回行う。コライダーのコンポーネントと
	* それぞれの拡縮のしかたは知っているが、GPU のことは知らない（そちらは
	* ColliderRenderer の担当）。
	*/
	class ColliderSystem
	{
	public:
		/**
		* [EN]
		* Rebuilds the collider list from every active Box, Sphere, Capsule,
		* Cylinder, Rect and Circle collider and every CharacterController's
		* capsule in the World.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* World の、アクティブな Box・Sphere・Capsule・Cylinder・Rect・Circle の
		* 各コライダーと、各 CharacterController のカプセルから、コライダーの
		* 一覧を作り直す。
		*/
		void Update(World& world);

	public:
		/**
		* [EN]
		* Returns the colliders gathered by the last Update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直前の Update で集めたコライダーを返す。
		*/
		[[nodiscard]] std::span<const ColliderDesc> Colliders()const;

	private:
		/// [EN] Colliders gathered this frame; kept between frames so the storage is reused.
		/// [JP] このフレームに集めたコライダー。領域を使い回すため、フレームをまたいで持つ。
		DynamicArray<ColliderDesc> colliders_;
	};
}

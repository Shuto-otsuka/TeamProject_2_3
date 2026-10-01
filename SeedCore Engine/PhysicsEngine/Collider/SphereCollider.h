#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component that creates a sphere-shaped collider body for its actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクター用の球型コライダーボディを生成するコンポーネント。
	*/
	class SEEDCORE_API SphereCollider :public SeedScript
	{
	public:
		/// [EN] Radius of the sphere.
		/// [JP] 球の半径。
		SC_REFLECTION_CLAMPED_EX("半径", 0.001f, 100.0f)
		Float radius_ = 0.5f;

		/// [EN] Whether this collider acts as a trigger.
		/// [JP] このコライダーをトリガーとして扱うか。
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [EN]
		* Creates the sphere shape and collider body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 球形状とコライダーボディを生成する。
		*/
		void OnAwake();

		/**
		* [EN]
		* Destroys the collider body and releases its shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コライダーボディを破棄し、形状を解放する。
		*/
		void OnDestroy();

	public:
		/**
		* [EN]
		* Creates and returns a sphere shape from the current settings, scaled
		* by the actor's own Scale; the parent's scale is not included. The
		* radius follows the largest axis, so the sphere encloses the scaled
		* shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の設定に Actor 自身の Scale を掛けた球形状を生成して返す。
		* 親のスケールは含めない。半径は最も大きい軸に合わせるので、球は
		* 拡縮後の形を包む。
		*/
		Handle<JPH::Shape> GetShapeHandle()const;

	private:
		/// [EN] Jolt body identifier of the generated collider body.
		/// [JP] 生成したコライダーボディの Jolt ボディ ID。
		JPH::BodyID bodyID_;

		/// [EN] Pool handle of the attached shape.
		/// [JP] 取り付けた形状のプールハンドル。
		Handle<JPH::Shape> shapeHandle_;
	};
	REGISTER_COMPONENT(SphereCollider, "Collider");
}

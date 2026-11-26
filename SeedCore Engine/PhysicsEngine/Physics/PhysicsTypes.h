#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/// [JP] 物理エンジンの中の体の番号
	/// [EN] A body number inside the physics engine
	using ScBodyID = Uint32;

	/// [JP] 「体がない」ことを表す番号
	/// [EN] The number that means "no body"
	inline constexpr ScBodyID ScInvalidBodyID = static_cast<ScBodyID>(-1);

	/// [JP] 何と何がぶつかるかを決める層の番号
	/// [EN] The layer number that decides what hits what
	using ScObjectLayer = Uint16;

	/// [JP] 大まかな当たり判定で使う層の番号
	/// [EN] The layer number used for the rough first collision check
	using ScBroadPhaseLayer = Uint8;

	/// [JP] キャラクターと、そのぶつかり通知を受ける係の目印
	/// [EN] Handles to a character and to what receives its contacts
	using ScCharacterHandle = void*;
	using ScCharacterListenerHandle = void*;

	enum class ScShapeTag;

	/// [JP] 当たり判定の形の目印
	/// [EN] A handle to a collision shape
	using ScShapeHandle = Handle<ScShapeTag>;

	enum class ScConstraintTag;

	/// [JP] つなぎ（ジョイント）の目印
	/// [EN] A handle to a joint
	using ScConstraintHandle = Handle<ScConstraintTag>;

	/**
	* [JP]
	* 体の動き方の種類。
	*
	* @brief 体の動き方
	*
	* [EN]
	* How a body moves.
	*
	* @brief How a body moves
	*/
	enum class ScMotionType : Uint8
	{
		/// [JP] 動かない
		/// [EN] Never moves
		Static,

		/// [JP] プログラムで動かす。ぶつかっても押されない
		/// [EN] Moved by code; it is not pushed by hits
		Kinematic,

		/// [JP] 重さや力で動く
		/// [EN] Moved by weight and forces
		Dynamic,
	};

	/// [JP] 動いたり回ったりしてよい向きの組み合わせ
	/// [EN] Which directions it may move and turn in
	using ScAllowedDOFs = Uint8;

	/// [JP] 全部の向き
	/// [EN] Every direction
	inline constexpr ScAllowedDOFs ScAllowedDOFsAll = 0b111111;

	/// [JP] 1つの向きだけ。X・Y・Z の移動と回転
	/// [EN] A single direction: moving or turning on X, Y or Z
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationX = 0b000001;
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationY = 0b000010;
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationZ = 0b000100;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationX = 0b001000;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationY = 0b010000;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationZ = 0b100000;

	/// [JP] 2D 用。X と Y の移動と、Z の回転だけ
	/// [EN] For 2D: only moving on X and Y and turning on Z
	inline constexpr ScAllowedDOFs ScAllowedDOFsPlane2D = ScAllowedDOFsTranslationX | ScAllowedDOFsTranslationY | ScAllowedDOFsRotationZ;
}

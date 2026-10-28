#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/// [EN] Engine-facing identifier for a physics body. Jolt conversion is confined to Physics implementation files.
	/// [JP] 物理ボディのエンジン側識別子。Jolt への変換は Physics の実装ファイル内に閉じる。
	using ScBodyID = Uint32;

	inline constexpr ScBodyID ScInvalidBodyID = static_cast<ScBodyID>(-1);

	/// [EN] Packed physics layer exposed by the engine-facing API.
	/// [JP] エンジン側 API が公開するパック済み物理レイヤー。
	using ScObjectLayer = Uint16;

	/// [EN] Engine-facing broad-phase layer index.
	/// [JP] エンジン側のブロードフェーズレイヤー番号。
	using ScBroadPhaseLayer = Uint8;

	/// [EN] Opaque handle to the physics backend's virtual character.
	/// [JP] 物理バックエンドの仮想キャラクターを指す不透明ハンドル。
	using ScCharacterHandle = void*;
	using ScCharacterListenerHandle = void*;

	/// [EN] Type tag for handles owned by the physics shape pool.
	/// [JP] 物理形状プールが所有するハンドルの型タグ。
	enum class ScShapeTag;

	using ScShapeHandle = Handle<ScShapeTag>;

	/// [EN] Type tag for handles owned by the physics constraint pool.
	/// [JP] 物理拘束プールが所有するハンドルの型タグ。
	enum class ScConstraintTag;

	using ScConstraintHandle = Handle<ScConstraintTag>;

	/// [EN] Engine-facing motion behavior for a rigid body.
	/// [JP] 剛体のエンジン側運動種別。
	enum class ScMotionType : Uint8
	{
		Static,
		Kinematic,
		Dynamic,
	};

	/// [EN] Bit mask describing the translation and rotation axes a body may use.
	/// [JP] ボディが使用できる移動・回転軸を表すビットマスク。
	using ScAllowedDOFs = Uint8;
	inline constexpr ScAllowedDOFs ScAllowedDOFsAll = 0b111111;
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationX = 0b000001;
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationY = 0b000010;
	inline constexpr ScAllowedDOFs ScAllowedDOFsTranslationZ = 0b000100;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationX = 0b001000;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationY = 0b010000;
	inline constexpr ScAllowedDOFs ScAllowedDOFsRotationZ = 0b100000;
	inline constexpr ScAllowedDOFs ScAllowedDOFsPlane2D = ScAllowedDOFsTranslationX | ScAllowedDOFsTranslationY | ScAllowedDOFsRotationZ;
}

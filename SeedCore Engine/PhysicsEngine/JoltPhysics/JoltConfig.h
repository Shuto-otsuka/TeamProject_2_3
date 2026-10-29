#pragma once

/// [EN] These options must match the configuration used to build Jolt.lib: several of them add members and virtual functions to Jolt's classes, so a mismatch makes this code and the library disagree on class layouts.
/// [JP] これらのオプションは Jolt.lib をビルドしたときの設定と一致させる。いくつかは Jolt のクラスにメンバや仮想関数を追加するので、食い違うとこのコードとライブラリでクラスのレイアウトが合わなくなる。

/// [EN] Builds in Jolt's DebugRenderer interface, through which bodies, shapes and contacts can be drawn for debugging.
/// [JP] Jolt の DebugRenderer インターフェースを組み込む。これを通して、ボディ・形状・接触点をデバッグ用に描画できる。
#ifndef JPH_DEBUG_RENDERER
#define JPH_DEBUG_RENDERER
#endif

/// [EN] Turns on Jolt's internal profiling scopes, which time each stage of the physics update.
/// [JP] Jolt 内部のプロファイル計測を有効にし、物理更新の各段階にかかる時間を測る。
#ifndef JPH_PROFILE_ENABLED
#define JPH_PROFILE_ENABLED
#endif

/// [EN] Lets floating-point exceptions (division by zero, invalid operations) be trapped, so a NaN or infinity is caught where it is produced; Jolt masks them itself around code that relies on them.
/// [JP] 浮動小数点例外(0 除算・無効な演算)を捕まえられるようにし、NaN や無限大を生まれたその場所で検出する。それらに頼る箇所では、Jolt 自身が例外をマスクする。
#ifndef JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#endif

/// [EN] Builds in ObjectStream, Jolt's serialization of settings and shapes to and from text or binary streams.
/// [JP] ObjectStream を組み込む。設定や形状をテキストまたはバイナリのストリームとの間で読み書きする、Jolt のシリアライズ機能。
#ifndef JPH_OBJECT_STREAM
#define JPH_OBJECT_STREAM
#endif

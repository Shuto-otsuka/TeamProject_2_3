#pragma once

struct CriAtomExPlayerTag;
struct CriAtomEx3dSourceObjTag;
struct CriAtomEx3dListenerObjTag;
struct CriAtomExAcbTag;

namespace SeedCore
{
	/// [EN] Opaque handle to an audio playback voice.
	/// [JP] オーディオ再生ボイスを指す不透明ハンドル。
	using ScAudioPlayerHandle = ::CriAtomExPlayerTag*;

	/// [EN] Opaque handle to a positional 3D audio source.
	/// [JP] 位置を持つ3D音源を指す不透明ハンドル。
	using ScAudioSourceHandle = ::CriAtomEx3dSourceObjTag*;

	/// [EN] Opaque handle to the shared 3D audio listener.
	/// [JP] 共有3Dオーディオリスナーを指す不透明ハンドル。
	using ScAudioListenerHandle = ::CriAtomEx3dListenerObjTag*;

	/// [EN] Opaque handle to a loaded audio cue bank.
	/// [JP] 読み込まれたオーディオキューバンクを指す不透明ハンドル。
	using ScAudioBankHandle = ::CriAtomExAcbTag*;
}

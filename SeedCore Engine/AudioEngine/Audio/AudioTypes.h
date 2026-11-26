#pragma once

struct CriAtomExPlayerTag;
struct CriAtomEx3dSourceObjTag;
struct CriAtomEx3dListenerObjTag;
struct CriAtomExAcbTag;

namespace SeedCore
{
	/// [JP] 音を鳴らすプレイヤーの目印
	/// [EN] A handle to a player that plays sound
	using ScAudioPlayerHandle = ::CriAtomExPlayerTag*;

	/// [JP] 音が出る場所の目印
	/// [EN] A handle to a place sound comes from
	using ScAudioSourceHandle = ::CriAtomEx3dSourceObjTag*;

	/// [JP] 音を聞く側の目印
	/// [EN] A handle to the listener
	using ScAudioListenerHandle = ::CriAtomEx3dListenerObjTag*;

	/// [JP] キューシートの目印
	/// [EN] A handle to a cue sheet
	using ScAudioBankHandle = ::CriAtomExAcbTag*;
}

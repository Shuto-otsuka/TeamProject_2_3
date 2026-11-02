#pragma once
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	class TextureResource;
	class BindlessHeap;
	class D3D12Context;

	/**
	* [EN]
	* Keeps the textures TextureResource holds within a GPU memory budget.
	* Every texture a renderer draws with is resolved through here, which
	* stamps the frame it was last used on and recreates it from its file
	* if it had been evicted. Once a frame, EvictBudget releases the
	* longest-unused textures until the resident total fits the budget.
	* A pinned texture (one with no file to reload from) is never evicted.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* TextureResource が持つテクスチャを、GPU メモリの予算内に収める。
	* 描画に使うテクスチャは全てここを通して解決する。最後に使ったフレームを
	* 記録し、追い出されていればファイルから作り直す。フレームに1回、
	* EvictBudget が常駐量の合計が予算に収まるまで、最も長く使われていない
	* テクスチャから解放する。固定されたテクスチャ（読み直す元のファイルが
	* 無いもの）は追い出さない。
	*/
	class SEEDCORE_API TextureStreaming :public NonCopyable
	{
	public:
		/**
		* [EN]
		* Takes the texture manager whose textures are streamed, and the
		* device context and heap an evicted texture is recreated with.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* テクスチャをストリーミングする対象のマネージャと、追い出された
		* テクスチャを作り直すデバイスのコンテキストとヒープを受け取る。
		*/
		TextureStreaming(TextureResource& textureResource, D3D12Context& context, BindlessHeap& heap);
		~TextureStreaming() = default;

		/**
		* [EN]
		* Resolves a handle for drawing on frame: marks the texture used on
		* that frame and recreates it from its file when it had been
		* evicted. Returns nullptr when the handle is stale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* frame で描画に使うハンドルを解決する。そのフレームで使ったと記録し、
		* 追い出されていればファイルから作り直す。ハンドルが古ければ nullptr
		* を返す。
		*/
		Texture* Resolve(const Handle<Texture>& handle, Uint64 frame);

		/**
		* [EN]
		* The bindless index of the texture registered under assetID, for
		* drawing on frame, or SC_INVALID when there is none or it could not
		* be made resident.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で登録されたテクスチャの、frame で描画に使うバインドレスの
		* 番号。無い、または常駐させられなかった場合は SC_INVALID。
		*/
		Uint32 GetID(Uint32 assetID, Uint64 frame);

		/**
		* [EN]
		* Releases resident textures, longest-unused first, until the total
		* fits the budget. A texture used within the last few frames is
		* kept even over budget, since it is likely to be drawn again at
		* once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 合計が予算に収まるまで、常駐しているテクスチャを最も長く使われて
		* いないものから解放する。直近数フレームで使ったテクスチャは、すぐまた
		* 描かれる可能性が高いため、予算を超えていても残す。
		*/
		void EvictBudget(Uint64 currentFrame);

	private:
		/// [EN] Manager whose table lists every texture that is streamed.
		/// [JP] ストリーミング対象の全テクスチャを対応表に持つマネージャ。
		TextureResource& textureResource_;

		/// [EN] Device context an evicted texture is recreated with.
		/// [JP] 追い出されたテクスチャを作り直すデバイスのコンテキスト。
		D3D12Context& context_;

		/// [EN] Heap the textures' shader resource views are allocated in and released to.
		/// [JP] テクスチャのシェーダーリソースビューを確保し、返すヒープ。
		BindlessHeap& heap_;

		/// [EN] GPU memory the resident textures may take in total, in bytes.
		/// [JP] 常駐テクスチャが合計で使ってよい GPU メモリ（バイト）。
		Uint64 budgetBytes_ = 128ull * 1024 * 1024;

		/// [EN] Frames a texture must go unused before it may be evicted.
		/// [JP] テクスチャを追い出せるようになるまでに、使われずに経過すべきフレーム数。
		SC_CONSTEX Uint64 evictAgeFrames_ = 8;
	};
}

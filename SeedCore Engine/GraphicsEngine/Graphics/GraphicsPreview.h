#pragma once
#include <GraphicsEngine/Preview/PreviewRequest.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/Screen/Boot/BootRequest.h>

namespace SeedCore
{
	class GraphicsModule;
	class PreviewCamera;
	class RenderDevice;
	class SceneRenderer;

	/**
	* [EN]
	* Draws the editor's tool previews. Each slot owns a small SceneRenderer
	* holding only the Preview feature set (the Screen feature set for the
	* boot screen) and the texture the panel shows;
	* both are made the first time the slot is asked for, and the texture is
	* remade when the panel's size changes. Requests are collected during the
	* frame and drawn once by Render.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディタのツールのプレビューを描く。スロットごとに、Preview の組（ブート画面では
	* Screen の組）の Feature だけを持つ小さな SceneRenderer と、パネルが表示する
	* テクスチャを持つ。どちらもその
	* スロットが初めて求められたときに作り、テクスチャはパネルの大きさが変わったら
	* 作り直す。求めはフレームの間に集め、Render で 1 回だけ描く。
	*/
	class GraphicsPreview
	{
	public:
		/**
		* [EN]
		* Keeps the device and the asset module. Nothing touches the GPU until a slot is first asked for.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デバイスとアセットモジュールを受け取る。スロットが初めて求められるまで GPU には触らない。
		*/
		GraphicsPreview(RenderDevice& device, GraphicsModule& module);

		~GraphicsPreview();

		/**
		* [EN]
		* Releases every slot's output texture and renderer; the device must be idle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全スロットの出力テクスチャとレンダラーを解放する。デバイスは待機済みであること。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Asks for slot to be drawn this frame with request, seen from camera at totalTime.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* slot をこのフレームに request で描くよう求める。camera から totalTime の時刻で見る。
		*/
		void Add(PreviewSlot slot, const PreviewRequest& request, const PreviewCamera& camera, Float totalTime);

		/**
		* [EN]
		* Asks for the boot screen slot to be drawn this frame with request, at a screen of width by height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ブート画面のスロットをこのフレームに request で描くよう求める。画面の大きさは width × height。
		*/
		void Add(const BootRequest& request, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Draws every slot asked for since the last call; later calls in the same frame do nothing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出しから求められた全スロットを描く。同じフレームの 2 回目以降は何もしない。
		*/
		void Render();

	public:
		/**
		* [EN]
		* Returns the handle of slot's output texture, or a null handle before it is first drawn.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* slot の出力テクスチャのハンドルを返す。初めて描くまでは空のハンドル。
		*/
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE Display(PreviewSlot slot)const;

	private:
		/**
		* [EN]
		* One preview: its renderer, its output texture, and what was asked for this frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プレビュー 1 つ分。レンダラー、出力テクスチャ、このフレームに求められた内容。
		*/
		struct Slot
		{
			/// [EN] Renderer holding only the Preview feature set (the Screen feature set for the boot screen); made on the first request.
			/// [JP] Preview の組（ブート画面では Screen の組）の Feature だけを持つレンダラー。最初の求めで作る。
			ResourcePtr<SceneRenderer> renderer_;

			/// [EN] Texture the panel shows, imported into the graph as Preview.Output (Boot.Output and Splash.Output for the boot screen).
			/// [JP] パネルが表示するテクスチャ。グラフへ Preview.Output（ブート画面では Boot.Output と Splash.Output）として取り込む。
			RenderGraphAllocation output_;

			/// [EN] What to draw this frame.
			/// [JP] このフレームに描くもの。
			PreviewRequest request_;

			/// [EN] What the boot screen slot draws this frame.
			/// [JP] ブート画面のスロットがこのフレームに描くもの。
			BootRequest boot_;

			/// [EN] The single view the preview is drawn from.
			/// [JP] プレビューを描く 1 つのビュー。
			RenderView view_;

			/// [EN] Whether the slot was asked for since the last Render.
			/// [JP] 前回の Render のあとにこのスロットが求められたか。
			Bool requested_ = false;
		};

	private:
		/// [EN] Device the renderers and output textures are made on.
		/// [JP] レンダラーと出力テクスチャを作るデバイス。
		RenderDevice& device_;

		/// [EN] Asset module the previews read models, materials and animations from.
		/// [JP] プレビューがモデル・マテリアル・アニメーションを読むアセットモジュール。
		GraphicsModule& module_;

		/// [EN] One entry per PreviewSlot.
		/// [JP] PreviewSlot ごとに 1 つ。
		StaticArray<Slot, PreviewSlotCount> slots_;
	};
}

#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	/**
	* [EN]
	* The GPU handles of the textures the editor UI shows as images, one per
	* render view. Each view's output is a persistent resource of the render
	* graph that is recreated when the view's size changes, so GraphicsScene
	* rewrites these handles after rendering every frame; everyone else only
	* reads them. A view's handle is empty while no Feature writes its output.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エディター UI が画像として表示するテクスチャの GPU ハンドル。描画ビュー
	* ごとに 1 つ持つ。各ビューの出力はレンダーグラフの永続リソースで、ビューの
	* サイズが変わると作り直されるので、GraphicsScene が毎フレーム描画の後に書き直し、
	* ほかは読むだけにする。出力を書く Feature がない間は、そのビューのハンドルは
	* 空になる。
	*/
	class SEEDCORE_API GraphicsDisplay
	{
	public:
		/**
		* [EN]
		* Returns the shader-visible handle of view's output texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view の出力テクスチャの、シェーダー可視ハンドルを返す。
		*/
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE View(RenderViewKind view)const;

	private:
		friend class GraphicsScene;

		/// [EN] Output handle of each view, indexed by RenderViewKind.
		/// [JP] 各ビューの出力のハンドル。RenderViewKind の番号で引く。
		StaticArray<D3D12_GPU_DESCRIPTOR_HANDLE, RenderViewCount> views_ = {};
	};
}

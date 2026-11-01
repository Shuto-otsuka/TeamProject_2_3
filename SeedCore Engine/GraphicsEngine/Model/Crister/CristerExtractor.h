#pragma once
#include <GraphicsEngine/Model/Crister/CristerGeometry.h>

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Reads a Crister's triangles back out into CPU-side vertex/index
	* arrays: positions for a baked ".collision" file, and full vertices
	* for a Softbody's simulation and render meshes. Walks cluster ->
	* meshlet -> triangle corner through the CPU-resident arrays only
	* (compressedVertices_/meshlets_/vertexIndices_/primitiveIndices_/
	* clusters_), so the result does not depend on what is streamed into
	* VRAM and needs no GPU readback.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister の三角形を、CPU 側の頂点/インデックス配列へ取り出す。
	* 焼き込む ".collision" ファイル用の位置と、Softbody のシミュレーション
	* 用・描画用のメッシュのためのフル頂点を作る。クラスタ -> meshlet ->
	* 三角形の角を、CPU 常駐の配列(compressedVertices_/meshlets_/
	* vertexIndices_/primitiveIndices_/clusters_)だけでたどるため、結果は
	* VRAM に何がストリーミングされているかに左右されず、GPU リードバック
	* も要らない。
	*/
	class SEEDCORE_API CristerExtractor
	{
	public:
		/**
		* [EN]
		* Flattens the Crister's triangles into a position/index pair, for
		* MeshCollisionLoader to bake into a ".collision" file. Exact takes
		* each SubMesh's LOD 0 cluster and Proxy its coarsest one; a static
		* SubMesh is emitted once per node placement, and vertices are
		* deduplicated within each placement. Positions are dequantised
		* with the same math as the shader decode. Returns false when no
		* triangle comes out.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Crister の三角形を位置/インデックス対へ展開する。
		* MeshCollisionLoader がこれを ".collision" ファイルへ焼き込む。
		* Exact は各 SubMesh の LOD 0 クラスタを、Proxy は最も粗い
		* クラスタを使う。静的な SubMesh はノードの配置ごとに1回ずつ出力し、
		* 頂点の重複排除は配置ごとに行う。位置はシェーダーのデコードと同じ
		* 計算で逆量子化する。三角形が1つも出なければ false を返す。
		*/
		NODISCARD Bool Collision(const Crister& crister, MeshCollisionDetail detail, DynamicArray<Vector3>& outPositions, DynamicArray<Uint32>& outIndices)const;

		/**
		* [EN]
		* Reads the Crister's LOD 0 geometry into full vertices
		* (position/normal/tangent/texcoord) and indices, in the same
		* global vertex index space compressedVertices_/vertexIndices_
		* use: outVertices is a 1:1 decoded copy of compressedVertices_
		* and outIndices reference it directly. A static SubMesh is moved
		* into the placement of the first node that references its mesh.
		* Because the ordering matches what the mesh shader reads, a
		* simulation running on these vertices can write vertex i straight
		* back into the render vertex buffer at index i. Softbody seeds its
		* render-side SoftbodyMesh from this. Returns false when no
		* triangle comes out.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Crister の LOD 0 ジオメトリを、compressedVertices_/vertexIndices_
		* と同じグローバル頂点インデックス空間のまま、フル頂点(位置/法線/
		* 接線/UV)とインデックスへ読み出す。outVertices は
		* compressedVertices_ を 1:1 でデコードしたコピーで、outIndices は
		* それを直接指す。静的な SubMesh は、そのメッシュを参照する最初の
		* ノードの配置へ移す。並びがメッシュシェーダーの読むものと同じなので、
		* これらの頂点で動くシミュレーションは頂点 i をそのまま描画用の
		* 頂点バッファのインデックス i へ書き戻せる。Softbody は描画側の
		* SoftbodyMesh をこれから作る。三角形が1つも出なければ false を返す。
		*/
		NODISCARD Bool FinestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;

		/**
		* [EN]
		* Reads each SubMesh's coarsest cluster (the same range as
		* Collision's Proxy detail) into full vertices and indices,
		* compacted into its own local index space and welded by
		* position. A Softbody simulates this much smaller proxy mesh,
		* since a render-resolution mesh needs one constraint per edge
		* and is far beyond what a real-time soft body solver handles;
		* SoftbodyMesh binds each FinestVertices vertex to its nearest
		* proxy vertices and blends their simulated displacement back onto
		* the full-resolution mesh every frame. Returns false when no
		* triangle comes out.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各 SubMesh の最も粗いクラスタ(Collision の Proxy と同じ範囲)を、
		* 自身のローカルなインデックス空間に詰め、位置で溶接したフル頂点と
		* インデックスへ読み出す。描画解像度のメッシュは辺ごとに拘束が
		* 1つずつ要り、リアルタイムのソフトボディソルバーが扱える規模を
		* はるかに超えるため、Softbody はこのずっと小さいプロキシメッシュで
		* シミュレーションする。SoftbodyMesh が FinestVertices の各頂点を
		* 最も近いプロキシ頂点群へ結び付け、そのシミュレーション後の変位を
		* 毎フレームフル解像度のメッシュへ混ぜ戻す。三角形が1つも出なければ
		* false を返す。
		*/
		NODISCARD Bool CoarsestVertices(const Crister& crister, DynamicArray<Vertex>& outVertices, DynamicArray<Uint32>& outIndices)const;
	};
}

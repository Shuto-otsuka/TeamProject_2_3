#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/Model/Culling/ModelCullingBuffer.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveMesh.h>
#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframeShader.h>
#include <GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidShader.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	struct LoaderSystem;

	class ShaderCache;
	class BindlessHeap;
	class D3D12CommandList;
	class TextureResource;

	/**
	* [EN]
	* One filled shape, for the GPU: its pose, its size (what each component
	* means depends on shapeKind_), how its surfaces are colored, and which
	* meshlets of the shared unit meshes it is drawn with. textureIndex_ is
	* SC_INVALID when the shape has no texture. Mirrors PrimitiveSolid.hlsli's
	* PrimitiveSolidStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 面で描く形1つ分の、GPU 向けのデータ。姿勢、大きさ（各成分の意味は
	* shapeKind_ で決まる）、面の色の付け方、共有の単位メッシュのどの
	* メッシュレットで描くかを持つ。テクスチャが無い形の textureIndex_ は
	* SC_INVALID。PrimitiveSolid.hlsli の PrimitiveSolidStructuredBuffer と
	* 一致する。
	*/
	struct PrimitiveSolidStructuredBuffer
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Quaternion rotation_ = Quaternion::Identity;
		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };
		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector2 uvScale_ = { 1.0f, 1.0f };
		Vector2 uvOffset_ = { 0.0f, 0.0f };
		Uint32 shapeKind_ = 0;
		Uint32 textureIndex_ = SC_INVALID;
		Uint32 doubleSided_ = 0;
		Uint32 meshletOffset_ = 0;
		Uint32 meshletCount_ = 0;
	};
	SC_STATIC_ASSERT(PrimitiveSolidStructuredBuffer, 92, "Shape/Primitive/Solid/PrimitiveSolid.hlsli");

	/**
	* [EN]
	* Draws the shapes ShapeSystem gathered and the colliders ColliderSystem
	* gathered, as one set. Wireframe shapes are sorted into three batches:
	* every world shape, drawn into the editor view; the world shapes whose
	* ShapeScope is Game, drawn into the game view as well; and canvas
	* shapes, drawn onto the canvas. Each batch's instances are registered in
	* the primitive wireframe slot of only the view that draws it. Solid
	* shapes form one batch drawn into both 3D views from the shared unit
	* meshes of PrimitiveMesh, with per-meshlet culling run separately for
	* each view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ShapeSystem が集めた形と、ColliderSystem が集めたコライダーを、まとめて
	* 描く。ワイヤーフレームの形は3つのバッチに振り分ける。ワールドの形は
	* すべてエディタービューへ、そのうち ShapeScope が Game のものは
	* ゲームビューへも、Canvas の形は Canvas へ描く。各バッチのインスタンスは、
	* それを描くビューの基本形状ワイヤーフレームの枠にだけ登録する。面で描く
	* 形は1つのバッチにまとめ、PrimitiveMesh の共有の単位メッシュから両方の
	* 3D ビューへ描く。メッシュレットごとのカリングは、ビューごとに別々に行う。
	*/
	class ShapeRenderer
	{
	public:
		ShapeRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~ShapeRenderer() = default;

		/**
		* [EN]
		* Builds the unit meshes, creates the wireframe and solid buffers and
		* the solid culling buffer, and builds both shaders' pipeline states.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 単位メッシュを作り、ワイヤーフレームと面のバッファ、面のカリングの
		* バッファを作り、2つのシェーダーのパイプラインステートを作る。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderResourceIndicesSystem& shaderResourceIndicesSystem);

		/**
		* [EN]
		* Packs the shapes and the colliders into the GPU layouts (entries past
		* a batch's maximum are dropped), uploads each non-empty batch, and
		* registers the wireframe batches in each view's primitive wireframe
		* slot and the solid buffers in the primitive solid slots. Textures of
		* solid shapes are resolved to bindless indices here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 形とコライダーを GPU 用の並びに詰め（バッチの最大数を超えた分は
		* 破棄する）、空でない各バッチをアップロードする。ワイヤーフレームの
		* バッチは各ビューの基本形状ワイヤーフレームの枠に、面のバッファは
		* 基本形状の面の枠に登録する。面の形のテクスチャは、ここで bindless の
		* 番号に直す。
		*/
		void Upload(std::span<const ShapeDesc> shapes, std::span<const ShapeDesc> colliders, LoaderSystem& loader, TextureResource& textureResource);

		/**
		* [EN]
		* Draws the solid batch and then the world wireframe batch into the
		* editor view, depth-tested against the given depth view, so the lines
		* sit over the surfaces. Must be called with the editor's root
		* addresses. Empty batches are skipped.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 面のバッチ、続いてワールドのワイヤーフレームのバッチを、指定した
		* 深度ビューで深度テストしながらエディタービューへ描く。線が面の上に
		* 乗るよう、この順にする。エディターのルートアドレスで呼ぶこと。空の
		* バッチは飛ばす。
		*/
		void DrawEditor3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Draws the solid batch into the game view, depth-tested against the
		* given depth view. Filled shapes are part of the scene, so this is
		* drawn before the hudless capture. Must be called with the game's
		* root addresses. Does nothing if the batch is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 面のバッチを、指定した深度ビューで深度テストしながらゲームビューへ
		* 描く。面の形はシーンの一部なので、hudless の取得より前に描く。
		* ゲームのルートアドレスで呼ぶこと。バッチが空なら何もしない。
		*/
		void DrawGameSolid(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Draws the game wireframe batch into the game view, depth-tested
		* against the given depth view. Wireframes are a debug display, so
		* this is drawn after the hudless capture. Must be called with the
		* game's root addresses. Does nothing if the batch is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームのワイヤーフレームのバッチを、指定した深度ビューで深度テスト
		* しながらゲームビューへ描く。ワイヤーフレームはデバッグ表示なので、
		* hudless の取得より後に描く。ゲームのルートアドレスで呼ぶこと。
		* バッチが空なら何もしない。
		*/
		void DrawGameWireframe(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Draws the canvas wireframe batch onto the canvas, over everything
		* without depth. Must be called with the canvas's root addresses. Does
		* nothing if the batch is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Canvas のワイヤーフレームのバッチを、深度を使わずに Canvas の上から
		* 描く。Canvas のルートアドレスで呼ぶこと。バッチが空なら何もしない。
		*/
		void Draw2D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/**
		* [EN]
		* Packs one shape into the batch it belongs to: solid shapes in the
		* world with a unit mesh into the solid batch, wireframe shapes into
		* the canvas batch or the world batch (and the game batch when scoped
		* to the game).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 形1つを、それが入るバッチに詰める。単位メッシュがあるワールドの面の
		* 形は面のバッチへ、ワイヤーフレームの形は Canvas のバッチか
		* ワールドのバッチ（Game のものはゲームのバッチにも）へ入れる。
		*/
		void AppendShape(const ShapeDesc& shape, LoaderSystem& loader, TextureResource& textureResource);

		/**
		* [EN]
		* Records the draw of instanceCount world wireframe instances into the
		* given target, with the primitive wireframe slot of the view whose
		* root addresses are passed. Shared by DrawEditor3D and DrawGameWireframe.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワールドのワイヤーフレームのインスタンス instanceCount 個を、指定した
		* ターゲットへ描く記録をする。使う基本形状ワイヤーフレームの枠は、
		* 渡したルートアドレスのビューのもの。DrawEditor3D と DrawGameWireframe で
		* 共有する。
		*/
		void DrawWireframe(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint instanceCount);

		/**
		* [EN]
		* Records the draw of the solid batch into the given target. On D12_2
		* one mesh dispatch covers every instance; below it the meshlets are
		* culled into the single-sided and double-sided lists for this view
		* and each list is drawn indirectly. Does nothing if the batch is
		* empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 面のバッチを、指定したターゲットへ描く記録をする。D12_2 では1回の
		* メッシュのディスパッチで全インスタンスを描く。それ未満では、この
		* ビュー用にメッシュレットを片面用と両面用の一覧へカリングし、それぞれの
		* 一覧を間接描画する。バッチが空なら何もしない。
		*/
		void DrawSolid(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/// [EN] Most wireframe shapes uploaded per batch in one frame; colliders share these batches.
		/// [JP] 1フレームに1つのバッチへアップロードするワイヤーフレームの形の最大数。コライダーも同じバッチに入る。
		SC_CONST Uint maxWireframeInstanceCount_ = 16384;

		/// [EN] Most solid shapes uploaded in one frame.
		/// [JP] 1フレームにアップロードする面の形の最大数。
		SC_CONST Uint maxSolidInstanceCount_ = 8192;

		/// [EN] Pipelines that draw the wireframe batches.
		/// [JP] ワイヤーフレームのバッチを描くパイプライン。
		PrimitiveWireframeShader wireframeShader_;

		/// [EN] Pipelines that draw the solid batch.
		/// [JP] 面のバッチを描くパイプライン。
		PrimitiveSolidShader solidShader_;

		/// [EN] Unit meshes of every solid shape kind, split into meshlets.
		/// [JP] 面で描く形の種類ごとの単位メッシュ。メッシュレットに分けてある。
		PrimitiveMesh primitiveMesh_;

		/// [EN] Visible-meshlet lists and indirect arguments for the solid culling below D12_2.
		/// [JP] D12_2 未満で面のカリングに使う、見えるメッシュレットの一覧と間接描画の引数。
		ModelCullingBuffer solidCullingBuffer_;

		/// [EN] World wireframe shapes packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めたワールドのワイヤーフレームの形。
		DynamicArray<PrimitiveWireframeStructuredBuffer> worldInstances_;

		/// [EN] World wireframe shapes whose ShapeScope is Game, packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めた、ShapeScope が Game のワールドのワイヤーフレームの形。
		DynamicArray<PrimitiveWireframeStructuredBuffer> gameInstances_;

		/// [EN] Canvas wireframe shapes packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めた Canvas のワイヤーフレームの形。
		DynamicArray<PrimitiveWireframeStructuredBuffer> canvasInstances_;

		/// [EN] Solid shapes packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めた面の形。
		DynamicArray<PrimitiveSolidStructuredBuffer> solidInstances_;

		/// [EN] GPU copy of worldInstances_.
		/// [JP] worldInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>> worldInstanceBuffer_;

		/// [EN] GPU copy of gameInstances_.
		/// [JP] gameInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>> gameInstanceBuffer_;

		/// [EN] GPU copy of canvasInstances_.
		/// [JP] canvasInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>> canvasInstanceBuffer_;

		/// [EN] GPU copy of solidInstances_.
		/// [JP] solidInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveSolidStructuredBuffer>> solidInstanceBuffer_;

		/// [EN] GPU copies of the unit meshes: vertices, meshlets, meshlet bounds, vertex indices and primitive indices.
		/// [JP] 単位メッシュの GPU 側のコピー。頂点、メッシュレット、メッシュレットの範囲、頂点番号、三角形番号。
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveVertex>> solidVertexBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletDesc>> solidMeshletBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<MeshletBound>> solidMeshletBoundBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<Uint32>> solidVertexIndicesBuffer_;
		ResourcePtr<ReadOnlyByteAddressBuffer> solidPrimitiveIndicesBuffer_;

		/// [EN] Index tables the buffers are registered in, owned by Renderer.
		/// [JP] バッファを登録するインデックステーブル。Renderer が所有する。
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;

		/// [EN] Descriptor heap textures are resolved into, owned by Renderer.
		/// [JP] テクスチャを解決するディスクリプタヒープ。Renderer が所有する。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Frame counter passed to texture streaming, so resolved textures stay resident while used.
		/// [JP] テクスチャのストリーミングに渡すフレームの番号。使っている間、解決したテクスチャが常駐し続けるようにする。
		Uint64 streamingFrame_ = 0;
	};
}

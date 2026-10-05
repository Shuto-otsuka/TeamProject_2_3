#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/Renderer/ColliderRenderer.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveWireframeShader.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	class ShaderCache;
	class BindlessHeap;
	class D3D12CommandList;

	/**
	* [EN]
	* Draws the shapes ShapeSystem gathered. Upload sorts the wireframe
	* shapes into two batches by ShapeSpace: world shapes, drawn into the
	* editor view by Draw3D, and canvas shapes, drawn onto the canvas by
	* Draw2D. Both are drawn by PrimitiveWireframeShader, whose lines are
	* built the same way as the collider lines, so the instances use the
	* ColliderStructuredBuffer layout; each batch's constants are registered
	* in the primitive wireframe slot of only the view that draws it. Filled
	* (Solid) shapes have no drawing path yet and are skipped.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ShapeSystem が集めた形を描く。Upload はワイヤーフレームの形を
	* ShapeSpace で2つのバッチに振り分ける。ワールドの形は Draw3D が
	* エディタービューへ、Canvas の形は Draw2D が Canvas へ描く。どちらも
	* PrimitiveWireframeShader で描き、線の作り方はコライダーの線と同じなので、
	* インスタンスは ColliderStructuredBuffer の並びを使う。各バッチの定数は、
	* それを描くビューの基本形状ワイヤーフレームの枠にだけ登録する。面で描く
	* 形（Solid）はまだ描く手段がないので飛ばす。
	*/
	class ShapeRenderer
	{
	public:
		ShapeRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~ShapeRenderer() = default;

		/**
		* [EN]
		* Creates both batches' instance buffers and constants, and the
		* shader's pipeline states.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのバッチのインスタンスバッファと定数、シェーダーのパイプライン
		* ステートを作る。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem);

		/**
		* [EN]
		* Packs the wireframe shapes into the GPU layout, sorting them into the
		* world or canvas batch (entries past maxInstanceCount_ in a batch are
		* dropped), uploads each non-empty batch with its constants, and
		* registers the world constants in the editor's primitive wireframe
		* slot and the canvas constants in the canvas's.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワイヤーフレームの形を GPU 用の並びに詰め、ワールドか Canvas の
		* バッチへ振り分ける（バッチで maxInstanceCount_ を超えた分は破棄する）。
		* 空でない各バッチを定数と一緒にアップロードし、ワールドの定数を
		* エディターの、Canvas の定数を Canvas の基本形状ワイヤーフレームの
		* 枠へ登録する。
		*/
		void Upload(std::span<const ShapeDesc> shapes);

		/**
		* [EN]
		* Draws the world batch into the editor view, depth-tested against the
		* given depth view. Must be called with the editor's root addresses.
		* Does nothing if the batch is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワールドのバッチを、指定した深度ビューで深度テストしながら
		* エディタービューへ描く。エディターのルートアドレスで呼ぶこと。
		* バッチが空なら何もしない。
		*/
		void Draw3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Draws the canvas batch onto the canvas, over everything without
		* depth. Must be called with the canvas's root addresses. Does nothing
		* if the batch is empty.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Canvas のバッチを、深度を使わずに Canvas の上から描く。Canvas の
		* ルートアドレスで呼ぶこと。バッチが空なら何もしない。
		*/
		void Draw2D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/// [EN] Most wireframe shapes uploaded per batch in one frame.
		/// [JP] 1フレームに1つのバッチへアップロードするワイヤーフレームの形の最大数。
		SC_CONST Uint maxInstanceCount_ = 8192;

		/// [EN] Pipelines that draw both batches.
		/// [JP] 2つのバッチを描くパイプライン。
		PrimitiveWireframeShader wireframeShader_;

		/// [EN] World shapes packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めたワールドの形。
		DynamicArray<ColliderStructuredBuffer> worldInstances_;

		/// [EN] Canvas shapes packed into the GPU layout this frame.
		/// [JP] このフレームに GPU 用の並びに詰めた Canvas の形。
		DynamicArray<ColliderStructuredBuffer> canvasInstances_;

		/// [EN] GPU copy of worldInstances_.
		/// [JP] worldInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>> worldInstanceBuffer_;

		/// [EN] GPU copy of canvasInstances_.
		/// [JP] canvasInstances_ の GPU 側のコピー。
		ResourcePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>> canvasInstanceBuffer_;

		/// [EN] Where the world instances are and how many, for the shader.
		/// [JP] シェーダーに渡す、ワールドのインスタンスの場所と数。
		ResourcePtr<StaticConstantBuffer<ColliderConstantBuffer>> worldConstantsBuffer_;

		/// [EN] Where the canvas instances are and how many, for the shader.
		/// [JP] シェーダーに渡す、Canvas のインスタンスの場所と数。
		ResourcePtr<StaticConstantBuffer<ColliderConstantBuffer>> canvasConstantsBuffer_;

		/// [EN] Index tables the constants are registered in, owned by Renderer.
		/// [JP] 定数を登録するインデックステーブル。Renderer が所有する。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
	};
}

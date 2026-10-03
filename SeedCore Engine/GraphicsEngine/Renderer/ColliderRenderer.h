#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ColliderInstance.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/Shape/Collider/ColliderLineShader.h>

namespace SeedCore
{
	class ShaderCache;
	class BindlessHeap;
	class D3D12CommandList;
	class ConstantIndicesSystem;

	/// [EN] Per-frame constants for the collider instance shader, one per
	///      batch (3D or 2D): which bindless structured-buffer index holds
	///      that batch's collider instances and how many there are, and how
	///      many mesh-shader groups each instance spans (groupsPerInstance_ —
	///      a capsule has more lines than one 128-thread group covers).
	///      Mirrors ColliderLine.hlsli's ColliderConstantBuffer byte-for-byte.
	/// [JP] コライダーインスタンスシェーダ用の毎フレーム定数で、バッチ（3D か 2D）
	///      ごとに1つずつ持つ: そのバッチのコライダーインスタンスを保持する
	///      bindless 構造化バッファのインデックスと個数、各インスタンスが何個の
	///      メッシュシェーダグループにまたがるか(groupsPerInstance_ — カプセルは
	///      1グループの128スレッドより線が多いため)。ColliderLine.hlsli の
	///      ColliderConstantBuffer とバイト単位で一致する。
	struct ColliderConstantBuffer
	{
		Uint instanceBufferIndex_ = 0;
		Uint instanceCount_ = 0;
		Uint groupsPerInstance_ = 0;
		Uint colliderConstantBufferPadding0_ = 0;
	};

	/**
	* [EN]
	* Editor-only debug wireframe renderer for collider visualization.
	* Deliberately does NOT inherit JPH::DebugRenderer — Renderer::Gather
	* walks each Box/Sphere/Capsule/Cylinder/Rect/CircleCollider component and
	* each CharacterController's capsule in the World directly (regardless of Play/Stop state, since it no longer
	* depends on live JPH::Body instances) and calls AddInstance with each
	* collider's own shape/transform data. The mesh shader (or, below D12_2,
	* the vertex shader) then expands each instance's wireframe geometry on
	* the GPU (see ColliderLineMS.hlsl / ColliderLineVS.hlsl) — this class
	* only uploads the small per-instance descriptor batches and issues the
	* draws.
	*
	* Instances are kept in two independent batches: spatial (3D colliders,
	* drawn into the editor's 3D view by Draw3D) and planar (Rect/Circle,
	* drawn onto the canvas by Draw2D). Each batch has its own instance and
	* constant buffer, and its constant index is registered only in the
	* index table of the view that draws it, so each draw reads nothing but
	* its own batch with no shader-side filtering.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダー可視化用の、エディタ専用デバッグワイヤーフレームレンダラー。
	* 意図的に JPH::DebugRenderer を継承しない — Renderer::Gather が
	* World 内の各 Box/Sphere/Capsule/Cylinder/Rect/CircleCollider
	* コンポーネントと各 CharacterController のカプセルを直接走査し（生きた JPH::Body に依存しなくなったため
	* Play/Stop を問わず動作する）、各コライダー自身の形状/変換データで
	* AddInstance を呼ぶ。ワイヤーフレーム形状の展開はメッシュシェーダ
	* （D12_2 未満では頂点シェーダ）が GPU上で行う（ColliderLineMS.hlsl /
	* ColliderLineVS.hlsl 参照）— このクラスは小さなインスタンス記述子
	* バッチをアップロードし、描画を発行するだけ。
	*
	* インスタンスは独立した2つのバッチに分けて持つ: spatial（3D コライダー。
	* Draw3D がエディタの 3D ビューへ描く）と planar（Rect/Circle。Draw2D が
	* Canvas へ描く）。各バッチは専用のインスタンスバッファと定数バッファを持ち、
	* その定数インデックスは、それを描くビューのインデックステーブルにだけ
	* 登録する。そのため各描画は自分のバッチだけを読み、シェーダ側での
	* 振り分けは要らない。
	*/
	class ColliderRenderer
	{
	public:
		ColliderRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~ColliderRenderer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem);

		/// [EN] Resets both CPU-side instance batches (spatial and planar) for
		///      a new frame. Called by Renderer::Gather before it repopulates
		///      them from the World's collider components.
		/// [JP] 新しいフレームに向けて、CPU 側の2つのインスタンスバッチ
		///      （spatial と planar）をリセットする。Renderer::Gather が World の
		///      コライダーコンポーネントから再び積み込む前に呼ぶ。
		void Clear();

		/// [EN] Appends one collider instance to the batch its shape belongs
		///      to: Rect/Circle go to the planar batch, every other shape to
		///      the spatial batch. Instances past maxInstanceCount_ in that
		///      batch are dropped.
		/// [JP] コライダーインスタンスを1つ、その形状が属するバッチへ追加する:
		///      Rect/Circle は planar バッチ、それ以外の形状はすべて spatial
		///      バッチ。そのバッチで maxInstanceCount_ を超えた分は破棄する。
		void AddInstance(ColliderShapeKind shapeKind, const Vector3& position, const Quaternion& rotation, const Vector3& dimensions, const Color& color);

		/// [EN] Uploads each non-empty batch to its own instance and constant
		///      buffer, then registers the spatial constants in the editor's
		///      index table and the planar constants in the canvas's.
		/// [JP] 空でない各バッチを専用のインスタンスバッファ/定数バッファへ
		///      アップロードし、spatial の定数をエディタのインデックステーブルへ、
		///      planar の定数を Canvas のインデックステーブルへ登録する。
		void Upload();

		/// [EN] Draws the spatial (3D) batch Upload() sent this frame into the
		///      editor's 3D view, depth-tested against the given depth view.
		///      Must be called with the editor's root addresses, since the
		///      spatial constants are registered only in the editor's index
		///      table. Does nothing if the spatial batch is empty. Takes raw
		///      views/viewport rather than FrameBuffer*/GeometryBuffer* so the
		///      caller picks the target - PostProcessRenderer's post-tonemap
		///      output + a matching depth view (see Renderer::EndEditorFrame).
		///      Caller owns every resource's state transitions.
		/// [JP] このフレームに Upload() が送った spatial（3D）バッチを、エディタの
		///      3D ビューへ、指定された深度ビューで深度テストしながら描く。
		///      spatial の定数はエディタのインデックステーブルにだけ登録されるので、
		///      エディタのルートアドレスで呼ぶこと。spatial バッチが空なら何もしない。
		///      FrameBuffer*/GeometryBuffer* ではなく生のビュー/ビューポートを
		///      受け取るので、描画先は呼び出し側が決める - PostProcessRenderer の
		///      トーンマップ後出力 + 対応する深度ビュー(Renderer::EndEditorFrame参照)。
		///      各リソースの状態遷移は呼び出し側の責任。
		void Draw3D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/// [EN] Draws the planar (2D) batch Upload() sent this frame onto the
		///      canvas. Must be called with the canvas's root addresses, since
		///      the planar constants are registered only in the canvas's index
		///      table. Binds no depth view and draws with depth testing off:
		///      canvas sprites lie on the same plane as the collider outlines,
		///      so a depth test would hide the outlines wherever a sprite is.
		///      Does nothing if the planar batch is empty.
		/// [JP] このフレームに Upload() が送った planar（2D）バッチを Canvas へ描く。
		///      planar の定数は Canvas のインデックステーブルにだけ登録されるので、
		///      Canvas のルートアドレスで呼ぶこと。深度ビューは bind せず、
		///      深度テストなしで描く: Canvas のスプライトはコライダーの輪郭と
		///      同じ平面にあるため、深度テストをするとスプライトのある場所で
		///      輪郭が隠れてしまう。planar バッチが空なら何もしない。
		void Draw2D(D3D12CommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	public:
		/// [EN] Line count of the densest shape, the capsule: two 32-segment
		///      rings, four side lines, four 16-segment cap arcs, and the
		///      silhouette (two 16-segment cap half circles and two side lines).
		///      Must match COLLIDER_CAPSULE_LINE_COUNT in ColliderLine.hlsli.
		/// [JP] 最も線の多い形状であるカプセルの線数: 32分割のリング2本、側面の
		///      縦線4本、16分割のキャップの半円4本、輪郭線(16分割のキャップの
		///      半円2本と側面の線2本)。ColliderLine.hlsli の
		///      COLLIDER_CAPSULE_LINE_COUNT と一致させること。
		static constexpr Uint maxLinesPerInstance_ = 2 * 32 + 4 + 6 * 16 + 2;

		/// [EN] Threads per mesh-shader group; each thread emits one line as a 4-vertex quad, so 64 lines fill the 256-vertex output limit. Must match COLLIDER_LINES_PER_GROUP in ColliderLine.hlsli.
		/// [JP] メッシュシェーダの1グループのスレッド数。各スレッドが線を1本、頂点4つの四角形として出すので、64本で出力上限の256頂点に達する。ColliderLine.hlsli の COLLIDER_LINES_PER_GROUP と一致させること。
		static constexpr Uint threadsPerGroup_ = 64;

		/// [EN] Vertices per line on the vertex-shader path: its quad drawn as two triangles. Must match ColliderLineVS.hlsl.
		/// [JP] 頂点シェーダ経路での1本あたりの頂点数。線の四角形を三角形2つで描く。ColliderLineVS.hlsl と一致させること。
		static constexpr Uint verticesPerLine_ = 6;

		/// [EN] Mesh-shader groups one instance spans, enough to cover the densest shape.
		/// [JP] 1インスタンスがまたがるメッシュシェーダグループ数。最も線の多い形状を賄える数にする。
		static constexpr Uint groupsPerInstance_ = (maxLinesPerInstance_ + threadsPerGroup_ - 1) / threadsPerGroup_;

	private:
		/// [EN] Capacity of each batch's collider-instance structured buffer
		///      (spatial and planar each get this many). Instances beyond this
		///      cap within a single frame are silently dropped rather than
		///      reallocating mid-frame.
		/// [JP] 各バッチのコライダーインスタンス構造化バッファの容量（spatial と
		///      planar がそれぞれこの数を持つ）。1フレーム内でこれを超えた
		///      インスタンスは、フレーム途中の再確保を避けるため黙って破棄される。
		static constexpr Uint maxInstanceCount_ = 8192;

		ColliderLineShader colliderLineShader_;

		/// [EN] CPU-side instance batches for the current frame: spatial holds the 3D colliders, planar holds Rect/Circle.
		/// [JP] 現フレームの CPU 側インスタンスバッチ: spatial は 3D コライダー、planar は Rect/Circle を保持する。
		DynamicArray<ColliderStructuredBuffer> spatialInstances_;
		DynamicArray<ColliderStructuredBuffer> planarInstances_;

		/// [EN] GPU copies of the two batches, read by the shader through the constant buffer of the same batch.
		/// [JP] 2つのバッチの GPU 側コピー。シェーダは同じバッチの定数バッファを経由して読む。
		ResourcePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>> spatialInstanceBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<ColliderStructuredBuffer>> planarInstanceBuffer_;

		/// [EN] Per-batch constants. The spatial one is registered in the editor's index table, the planar one in the canvas's.
		/// [JP] バッチごとの定数。spatial はエディタの、planar は Canvas のインデックステーブルに登録する。
		ResourcePtr<StaticConstantBuffer<ColliderConstantBuffer>> spatialInstanceConstantsBuffer_;
		ResourcePtr<StaticConstantBuffer<ColliderConstantBuffer>> planarInstanceConstantsBuffer_;

		BindlessHeap* bindlessHeap_ = nullptr;
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
	};
}

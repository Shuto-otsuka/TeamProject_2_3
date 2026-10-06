#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/Refraction/RefractionShader.h>

namespace SeedCore
{
	struct RootAddresses;

	class BindlessHeap;
	class ConstantIndicesSystem;
	class D3D12CommandList;
	class ShaderCache;
	class ShaderResourceIndicesSystem;
	class UnorderedAccessIndicesSystem;

	/**
	* [EN]
	* Tuning values of the refraction pass. Mirrors RefractionRayConstantBuffer
	* in Raytracing/Refraction/RefractionRT.hlsl, which both RefractionRT.hlsl
	* and DeferredLightingPS.hlsl read through constant_indices.refraction_index_,
	* so the layout must match the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 屈折パスの調整値。Raytracing/Refraction/RefractionRT.hlsl の
	* RefractionRayConstantBuffer と対応する。RefractionRT.hlsl と
	* DeferredLightingPS.hlsl の両方が constant_indices.refraction_index_
	* 経由で読むため、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct RefractionRayConstantBuffer
	{
		/// [EN] Maximum distance a refracted ray travels before it counts as a miss.
		/// [JP] 屈折したレイがミスとして扱われるまでに進む最大距離。
		Float rayTMax_ = 1000.0f;

		/// [EN] Offset along the surface normal applied to each ray origin, so a ray does not hit the surface it leaves.
		/// [JP] 各レイの原点に法線方向へ加えるオフセット。出発した面にレイがヒットしないようにする。
		Float normalBias_ = 0.01f;

		/// [EN] Overall refraction intensity applied in DeferredLightingPS.hlsl.
		/// [JP] DeferredLightingPS.hlsl で適用する屈折の全体強度。
		Float strength_ = 1.0f;

		/// [EN] Pads the structure to a full 16-byte register.
		/// [JP] 構造体を 16 バイトのレジスタ 1 つ分に揃える詰め物。
		Float refractionPadding_ = 0.0f;

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The padding is
		* not serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。詰め物はシリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("normalBias", normalBias_);
			archive.TryField("strength", strength_);
		}
	};

	/**
	* [EN]
	* Runs the ray-traced refraction pass. It owns the pass's raytracing
	* pipeline (RefractionShader), one raw output texture and a one-record
	* shader table. There is no denoiser: the ray follows a deterministic
	* Snell-refracted path rather than a randomly sampled lobe like
	* Reflection's GGX, so there is no roughness-driven noise to remove. The
	* shader table holds only the ray-generation record, because the whole
	* bounce chain runs as an inline RayQuery loop inside ray generation.
	* The pass has no instance or material table of its own: RefractionRT.hlsl
	* reuses Reflection's per-triangle tables (same TLAS, same instance order,
	* same ReflectionMaterialData), so RaytracingRenderer only updates them
	* once through ReflectionRenderer::UpdateInstanceTable.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによる屈折パスを実行する。パスのレイトレーシング
	* パイプライン（RefractionShader）、生の出力テクスチャ 1 枚、1 レコードの
	* シェーダーテーブルを持つ。デノイザは無い。このレイは Reflection の GGX
	* のような確率的にサンプルするローブではなく、スネルの法則に従う決定論的な
	* 経路をたどるため、roughness に起因するノイズが生じない。バウンスの連鎖は
	* すべてレイ生成シェーダー内のインライン RayQuery ループで行うため、
	* シェーダーテーブルはレイ生成のレコードだけを持つ。インスタンスや
	* マテリアルのテーブルは持たない。RefractionRT.hlsl は Reflection の
	* 三角形単位のテーブル（同じ TLAS、同じインスタンス順序、同じ
	* ReflectionMaterialData）を再利用するため、RaytracingRenderer は
	* ReflectionRenderer::UpdateInstanceTable で 1 度更新するだけでよい。
	*/
	class RefractionRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and raytracing-state cache that the
		* refraction shader compiles into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 屈折シェーダーのコンパイル先となる、共有のルートシグネチャと
		* レイトレーシングステートキャッシュを関連付ける。
		*/
		RefractionRenderer(RootSignature& rootSignature, RaytracingStateObject& raytracingStateObject);

		/**
		* [EN]
		* Destroys the renderer. GPU resources are released together with the
		* bindless heap at shutdown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レンダラーを破棄する。GPU リソースは終了時に bindless ヒープと共に
		* 解放される。
		*/
		~RefractionRenderer() = default;

		/**
		* [EN]
		* Compiles the raytracing pipeline, creates the tuning constant buffer
		* and the shader table, and allocates the output texture at width x
		* height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイトレーシングパイプラインをコンパイルし、調整値の定数バッファと
		* シェーダーテーブルを作成して、width x height の出力テクスチャを確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the output texture for a new render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて出力テクスチャを作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Uploads the tuning values, records whether the pass runs this frame,
		* and publishes this pass's bindless indices to the index systems. It
		* records no GPU work, and must run before the index systems upload
		* this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 調整値をアップロードし、今フレームにパスを実行するかを記録して、
		* このパスの bindless インデックスを各インデックスシステムへ公開する。
		* GPU 処理は記録せず、各インデックスシステムが今フレームのインデックスを
		* アップロードする前に呼び出す。
		*/
		void Prepare(const RefractionRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass: dispatches rays into the output texture, or clears
		* it to 0 when the pass is off, and leaves the texture in
		* shader-resource state. The G-Buffer depth, normals and visibility IDs
		* must already be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスを記録する。出力テクスチャへレイをディスパッチするか、パスが無効
		* なら 0 でクリアし、テクスチャをシェーダーリソース状態で終える。
		* G-Buffer の深度、法線、可視性 ID が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ output texture and its bindless and
		* clear descriptors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の出力テクスチャと、その bindless ディスクリプタ
		* およびクリア用ディスクリプタを作成する。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees the output texture's bindless descriptors and defers the
		* texture's destruction until the GPU has finished with it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 出力テクスチャの bindless ディスクリプタを解放し、テクスチャ自体の
		* 破棄は GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] Raytracing pipeline of RefractionRT.hlsl.
		/// [JP] RefractionRT.hlsl のレイトレーシングパイプライン。
		RefractionShader refractionShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<StaticConstantBuffer<RefractionRayConstantBuffer>> tuningBuffer_;

		/// [EN] Refracted radiance written by the pass and read by deferred lighting.
		/// [JP] パスが書き込み、ディファードライティングが読む屈折後の放射輝度。
		Microsoft::WRL::ComPtr<ID3D12Resource> outputResource_;

		/// [EN] Resource state the output texture is currently in, tracked to issue only the barriers that are needed.
		/// [JP] 出力テクスチャの現在のリソース状態。必要なバリアだけを発行するために追跡する。
		D3D12_RESOURCE_STATES outputState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless index of the output texture's write view.
		/// [JP] 出力テクスチャの書き込み用ビューの bindless インデックス。
		Uint32 outputUnorderedAccessViewIndex_ = 0;

		/// [EN] Bindless index of the output texture's read view.
		/// [JP] 出力テクスチャの読み取り用ビューの bindless インデックス。
		Uint32 outputShaderResourceViewIndex_ = 0;

		/// [EN] Upload-heap shader table holding the single ray-generation record.
		/// [JP] レイ生成のレコード 1 つだけを持つ、アップロードヒープ上のシェーダーテーブル。
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTableResource_;

		/// [EN] Size of one shader-table record: the 32-byte shader identifier rounded up to the 64-byte record alignment.
		/// [JP] シェーダーテーブルのレコード 1 つのサイズ。32 バイトのシェーダー識別子を 64 バイトのレコードアライメントへ切り上げたもの。
		SC_CONST Uint32 shaderTableRecordSize_ = 64;

		/// [EN] Non-shader-visible heap holding the CPU-side write view that ClearUnorderedAccessViewFloat requires alongside the shader-visible one.
		/// [JP] ClearUnorderedAccessViewFloat がシェーダー可視のビューと併せて要求する、CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the output texture's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での出力テクスチャの書き込み用ビューのインデックス。
		Uint32 clearOutputIndex_ = 0;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Size of the output texture, equal to the native render size.
		/// [JP] 出力テクスチャのサイズ。ネイティブのレンダーサイズと等しい。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		/// [EN] Whether the pass traces this frame: the pass is switched on and the scene has a TLAS.
		/// [JP] 今フレームにパスをトレースするか。パスが有効で、かつシーンに TLAS がある場合に true。
		Bool enabled_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool stateObjectMissingLogged_ = false;
	};
}

#pragma once
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>

namespace SeedCore
{
	class BindlessHeap;
	class ShaderCache;
	class D3D12CommandList;
	class VertexShader;
	class MeshShader;
	class PixelShader;
	struct RootAddresses;

	/**
	* [EN]
	* Screen-space outline around every mesh whose material asks for one
	* (Surface::outlineColor_/outlineWidth_, carried per instance in
	* ModelShading). Opaque and alpha-masked meshes are found through the
	* VisibilityBuffer; transparent meshes write no VisibilityBuffer id, so
	* the transparent pass records them itself: ClearTransparent empties
	* transparentResource_ before it, and each outlined transparent fragment
	* that passes the depth test InterlockedMax-es (depth << 16) | (instance
	* + 1) into it, which keeps the nearest one per pixel. Draw then runs in
	* three steps: ResetMaxWidth clears the widest width on screen, Resolve
	* writes each native-resolution pixel's material outline into
	* resolveResource_ - the transparent record when there is one, since it
	* passed the depth test and so lies in front of the opaque surface,
	* otherwise the VisibilityBuffer instance - and reduces the widest width,
	* then the composite draws fullscreen onto the tone-mapped display
	* texture, painting the band just outside each outlined mesh. When no
	* material on screen has an outline, every composite pixel stops at its
	* first read and discards, so what remains is the resolve dispatch
	* itself. Only the nearest transparent mesh per pixel is outlined, and
	* the 16-bit instance field limits the transparent record to 65535
	* instances.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* マテリアルが縁取りを指定した(Surface::outlineColor_/outlineWidth_、
	* インスタンスごとに ModelShading で運ばれる)すべてのメッシュの周りに
	* 描く、スクリーン空間のアウトライン。不透明とアルファマスクのメッシュ
	* は VisibilityBuffer から見つける。半透明のメッシュは VisibilityBuffer
	* の id を書かないので、半透明のパス自身が記録する: その前に
	* ClearTransparent が transparentResource_ を空にし、深度テストを通った
	* 縁取りのある半透明のフラグメントが (深度 << 16) | (インスタンス + 1)
	* を InterlockedMax で書き込み、ピクセルごとに一番手前のものが残る。
	* Draw はその後 3 段階で動く: ResetMaxWidth が画面上で一番太い幅を 0 に
	* 戻し、Resolve がネイティブ解像度の各ピクセルのマテリアルの縁取りを
	* resolveResource_ へ書き(半透明の記録があればそれを使う — 深度テストを
	* 通っているので不透明の面より手前にあるため。無ければ VisibilityBuffer
	* のインスタンスを使う)、一番太い幅も求める。最後に合成がトーンマップ
	* 済みの表示用テクスチャへフルスクリーンで描き、縁取るメッシュのすぐ
	* 外側に帯を塗る。画面上に縁取りを持つマテリアルが無いときは、合成の
	* 各ピクセルが最初の読み取りで discard するので、残る負荷は resolve の
	* ディスパッチだけになる。縁取られる半透明はピクセルごとに一番手前の
	* 1 つだけで、インスタンスの欄が 16 ビットなので、半透明の記録が扱える
	* インスタンスは 65535 個まで。
	*/
	class MaterialOutline
	{
	private:
		struct MaterialOutlineConstantBuffer
		{
			/// [EN] Bindless UAV of resolveResource_, written by Resolve.
			/// [JP] Resolve が書き込む resolveResource_ の bindless UAV。
			Uint resolveUnorderedAccessIndex_ = 0;

			/// [EN] Bindless SRV of resolveResource_, read by the composite.
			/// [JP] 合成が読む resolveResource_ の bindless SRV。
			Uint resolveShaderResourceIndex_ = 0;

			/// [EN] Bindless UAV of maxWidthResource_, cleared by ResetMaxWidth and accumulated by Resolve.
			/// [JP] ResetMaxWidth が 0 に戻し、Resolve が最大を積み上げる maxWidthResource_ の bindless UAV。
			Uint maxWidthUnorderedAccessIndex_ = 0;

			/// [EN] Bindless SRV of maxWidthResource_, read by the composite.
			/// [JP] 合成が読む maxWidthResource_ の bindless SRV。
			Uint maxWidthShaderResourceIndex_ = 0;

			/// [EN] Bindless UAV of transparentResource_, emptied by ClearTransparent and read by Resolve.
			/// [JP] ClearTransparent が空にし、Resolve が読む transparentResource_ の bindless UAV。
			Uint transparentUnorderedAccessIndex_ = 0;

			Uint materialOutlinePadding0_ = 0;
			Uint materialOutlinePadding1_ = 0;
			Uint materialOutlinePadding2_ = 0;
		};

	public:
		MaterialOutline() = default;
		~MaterialOutline() = default;

		/**
		* [EN]
		* Creates the native-resolution resolve texture and transparent
		* record, the max-width buffer and the four pipeline states.
		* width/height are the native (VisibilityBuffer) resolution.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ネイティブ解像度の resolve テクスチャと半透明の記録、最大幅の
		* バッファ、4 つのパイプラインステートを作る。width/height は
		* ネイティブ(VisibilityBuffer)の解像度。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Releases the resources and their bindless indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* リソースと、その bindless インデックスを解放する。
		*/
		void Destroy(BindlessHeap* bindlessHeap);

		/**
		* [EN]
		* Recreates the resources for a new native resolution.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいネイティブ解像度に合わせてリソースを作り直す。
		*/
		void Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Empties the transparent record. Call before the transparent pass of
		* the view that Draw will resolve, so only this frame's fragments are
		* in it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 半透明の記録を空にする。Draw で解決するビューの半透明のパスより前に
		* 呼び、このフレームのフラグメントだけが入るようにする。
		*/
		void ClearTransparent(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Resolves this frame's material outlines and composites them onto
		* renderTargetView (the R8G8B8A8_UNORM tone-mapped display texture,
		* already in RENDER_TARGET). The caller must have the
		* VisibilityBuffer and depth shader-readable (GeometryBuffer::End /
		* EndDepth) and addresses must be the view whose GeometryBuffer and
		* model instances are being outlined.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このフレームのマテリアルの縁取りを求め、renderTargetView(既に
		* RENDER_TARGET の、R8G8B8A8_UNORM のトーンマップ済み表示用
		* テクスチャ)へ合成する。呼び出し側は VisibilityBuffer と深度を
		* シェーダから読める状態(GeometryBuffer::End / EndDepth)にしておく
		* 必要があり、addresses は縁取る GeometryBuffer とモデルの
		* インスタンスを持つビューのものでなければならない。
		*/
		void Draw(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView, D3D12_VIEWPORT viewport, const RootAddresses& addresses);

		/**
		* [EN]
		* Bindless UAV of the transparent record, for the transparent pass to
		* write outlined fragments into (UnorderedAccessIndicesSystem::
		* SetOITMaterialOutlineIndex).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 半透明のパスが縁取りのあるフラグメントを書き込む、半透明の記録の
		* bindless UAV(UnorderedAccessIndicesSystem::
		* SetOITMaterialOutlineIndex)。
		*/
		NODISCARD Uint32 TransparentUnorderedAccessViewIndex()const;

	private:
		/// [EN] Native-resolution RG32_UINT texture: x = material outline color packed as RGBA8, y = asuint(width).
		/// [JP] ネイティブ解像度の RG32_UINT テクスチャ。x がマテリアルの縁取りの色(RGBA8 に詰めたもの)、y が asuint(幅)。
		Microsoft::WRL::ComPtr<ID3D12Resource> resolveResource_;
		D3D12_RESOURCE_STATES resolveState_ = D3D12_RESOURCE_STATE_COMMON;
		Uint32 resolveUnorderedAccessViewIndex_ = 0;
		Uint32 resolveShaderResourceViewIndex_ = 0;

		/// [EN] One-element buffer holding asuint() of the widest outline on screen this frame.
		/// [JP] このフレームの画面上で一番太い縁取りの asuint() を持つ、要素 1 つのバッファ。
		Microsoft::WRL::ComPtr<ID3D12Resource> maxWidthResource_;
		D3D12_RESOURCE_STATES maxWidthState_ = D3D12_RESOURCE_STATE_COMMON;
		Uint32 maxWidthUnorderedAccessViewIndex_ = 0;
		Uint32 maxWidthShaderResourceViewIndex_ = 0;

		/// [EN] Native-resolution R32_UINT texture: (depth << 16) | (instance + 1) of the nearest outlined transparent fragment, 0 when none. Kept in UNORDERED_ACCESS for its whole life, since every pass that touches it reads or writes it as a UAV.
		/// [JP] ネイティブ解像度の R32_UINT テクスチャ。一番手前の縁取りのある半透明のフラグメントの (深度 << 16) | (インスタンス + 1)、無ければ 0。触るパスはすべて UAV として読み書きするので、生存期間中ずっと UNORDERED_ACCESS のままにする。
		Microsoft::WRL::ComPtr<ID3D12Resource> transparentResource_;
		Uint32 transparentUnorderedAccessViewIndex_ = 0;

		/// [EN] Native resolution the resolve texture is sized to.
		/// [JP] resolve テクスチャの大きさ(ネイティブ解像度)。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		ResourcePtr<StaticConstantBuffer<MaterialOutlineConstantBuffer>> constantBuffer_;

		RootSignature* rootSignature_ = nullptr;

		Handle<ComputeShader> clearTransparentShader_;
		Handle<ComputeShader> resetMaxWidthShader_;
		Handle<ComputeShader> resolveShader_;
		Handle<VertexShader> compositeVertexShader_;
		Handle<MeshShader> compositeMeshShader_;
		Handle<PixelShader> compositePixelShader_;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> clearTransparentPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> resetMaxWidthPipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> resolvePipelineState_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> compositePipelineState_;
	};
}

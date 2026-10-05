#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/FrameRing.h>
#include <GraphicsEngine/Model/Morph/MorphBlendShader.h>
#include <GraphicsEngine/Model/Skin/SkinBlendShader.h>
#include <GraphicsEngine/Raytracing/BottomLevelAccelerationStructure.h>
#include <GraphicsEngine/Raytracing/RaytracingContext.h>
#include <GraphicsEngine/Raytracing/RaytracingDispatch.h>
#include <GraphicsEngine/Raytracing/TopLevelAccelerationStructure.h>
#include <GraphicsEngine/Renderer/AmbientOcclusionRenderer.h>
#include <GraphicsEngine/Renderer/GlobalIlluminationRenderer.h>
#include <GraphicsEngine/Renderer/ReflectionRenderer.h>
#include <GraphicsEngine/Renderer/RefractionRenderer.h>
#include <GraphicsEngine/Renderer/ShadowRenderer.h>
#include <GraphicsEngine/Renderer/SubsurfaceScatteringRenderer.h>
#include <GraphicsEngine/Renderer/VolumetricCloudScapesRenderer.h>
#include <GraphicsEngine/Renderer/VolumetricLightRenderer.h>
#include <GraphicsEngine/Renderer/VolumetricStarRenderer.h>
#include <GraphicsEngine/Renderer/WeatherParticleRenderer.h>
#include <GraphicsEngine/System/WeatherSystem.h>

namespace SeedCore
{
	struct LoaderSystem;
	struct RootAddresses;

	class BindlessHeap;
	class ConstantIndicesSystem;
	class Crister;
	class D3D12CommandList;
	class FrameBuffer;
	class GeometryBuffer;
	class ModelRenderer;
	class ModelResource;
	class PipelineStateObject;
	class RootSignature;
	class ShaderCache;
	class ShaderResourceIndicesSystem;
	class UnorderedAccessIndicesSystem;
	class World;

	/**
	* [EN]
	* Owns the scene's raytracing acceleration structures and every ray-traced
	* pass that consumes them, so Renderer talks to this one class for anything
	* raytraced.
	*
	* Static meshes get one BLAS per unique Crister, built once and cached.
	* Skinned and morphed actors get a BLAS per entity and frame-ring slot,
	* rebuilt every frame from positions deformed on the GPU (MorphBlendCS,
	* then SkinBlendCS). The TLAS is rebuilt every frame from the instances
	* collected by Gather.
	*
	* Gather does its own minimal ECS traversal instead of reusing
	* ModelRenderer's, because it only needs one (Crister, world matrix) pair
	* per actor, not meshlet/LOD dispatch batches.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンのレイトレーシング アクセラレーション構造と、それを使う全ての
	* レイトレーシングのパスを保持する。Renderer はレイトレ関連について
	* このクラス 1 つとだけやり取りする。
	*
	* 静的メッシュはユニークな Crister ごとに BLAS を 1 つ持ち、1 度だけ
	* 構築してキャッシュする。スキンやモーフのあるアクターは、エンティティと
	* フレームリングスロットごとに BLAS を持ち、GPU で変形した位置
	* (MorphBlendCS、続いて SkinBlendCS)から毎フレーム再構築する。TLAS は
	* Gather が集めたインスタンスから毎フレーム再構築する。
	*
	* Gather は ModelRenderer の走査を使い回さず、独自の最小限の ECS 走査を
	* 行う。必要なのはアクターごとの (Crister, ワールド行列) の組 1 つだけで、
	* メシュレット/LOD のディスパッチバッチは要らないため。
	*/
	class RaytracingRenderer
	{
	public:
		/**
		* [EN]
		* Creates every pass renderer, sharing the engine-wide root signature and
		* pipeline-state / raytracing-state caches with them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全パスのレンダラーを生成し、エンジン共通のルートシグネチャと
		* パイプラインステート/レイトレーシングステートのキャッシュを共有させる。
		*/
		RaytracingRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject, RaytracingStateObject& raytracingStateObject);
		~RaytracingRenderer() = default;

		/**
		* [EN]
		* Reserves one TLAS bindless slot per frame-ring slot, creates every pass
		* at width x height, and compiles the morph and skin blend shaders.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームリングスロットごとに TLAS の bindless スロットを 1 つ確保し、
		* 全パスを width x height で生成し、モーフとスキンのブレンドシェーダを
		* コンパイルする。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Resizes every screen-sized pass to the new native resolution.
		* VolumetricLight is left alone because its froxel volume is a fixed grid,
		* and the BLAS/TLAS are left alone because they are sized by geometry, not
		* by the screen.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 画面サイズに依存する全パスを新しいネイティブ解像度にリサイズする。
		* VolumetricLight は froxel ボリュームが固定グリッドのため対象外。
		* BLAS/TLAS も画面ではなくジオメトリでサイズが決まるため対象外。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Collects one PendingInstance per active Mesh actor, queues meshes whose
		* static BLAS is not cached yet, and prunes the caches of meshes and
		* entities that are gone. Records no GPU work.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 有効な Mesh アクターごとに PendingInstance を 1 つ集め、静的 BLAS が
		* 未キャッシュのメッシュを積み、消えたメッシュやエンティティの
		* キャッシュを刈る。GPU の処理は記録しない。
		*/
		void Gather(LoaderSystem& loaderSystem, ModelResource& modelResource, World& world, const ModelRenderer& modelRenderer);

		/**
		* [EN]
		* Records this frame's BLAS/TLAS work (static BLAS builds, morph and skin
		* blending, per-entity BLAS rebuilds, TLAS rebuild), publishes the TLAS
		* index to ShaderResourceIndicesSystem, then prepares every pass for the
		* frame. None of this needs the G-Buffer, so it runs before any view
		* uploads its indices (see Renderer::PrepareFrame).
		*
		* The morph and skin blend passes run on the shared root signature, so
		* they need heap and addresses like every other pass.
		*
		* deltaTime and nightFactor drive the shooting stars of VolumetricStar.
		* cameraPosition, totalTime and weather drive WeatherParticle: the
		* particle volume follows the camera, and weather carries the rain/snow
		* tuning of the scene's Weather component.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今フレームの BLAS/TLAS の処理(静的 BLAS の構築、モーフとスキンの
		* ブレンド、エンティティごとの BLAS 再構築、TLAS 再構築)を記録し、
		* TLAS のインデックスを ShaderResourceIndicesSystem へ公開してから、
		* 全パスをこのフレーム用に準備する。どれも G-Buffer を必要としないため、
		* どのビューがインデックスをアップロードするよりも前に実行する
		* (Renderer::PrepareFrame 参照)。
		*
		* モーフとスキンのブレンドパスは共有のルートシグネチャで動くため、
		* 他のパスと同じく heap と addresses を必要とする。
		*
		* deltaTime と nightFactor は VolumetricStar の流れ星を進める。
		* cameraPosition、totalTime、weather は WeatherParticle を進める。
		* パーティクルのボリュームはカメラに追従し、weather はシーンの Weather
		* コンポーネントの雨/雪の調整値を運ぶ。
		*/
		void Build(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, ID3D12Device* device, const ModelRenderer& modelRenderer, Float deltaTime, Float nightFactor, const Vector3& cameraPosition, Float totalTime, const WeatherGpuState& weather);

		/**
		* [EN]
		* Records the ray-traced pass type for view: the pass's own work, or
		* its fallback clear when it is off this frame. It needs the G-Buffer
		* depth and normals, so it runs later in the frame than Build. Passes
		* that keep no per-view history (SubsurfaceScattering, Refraction,
		* VolumetricCloudScapes, VolumetricStar) ignore view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view について、レイトレーシングのパス type を記録する。パス本来の
		* 処理か、今フレームに無効ならそのフォールバックのクリアを行う。
		* G-Buffer の深度と法線が必要なため、フレームの中で Build より後に
		* 呼び出す。ビューごとの履歴を持たないパス（SubsurfaceScattering、
		* Refraction、VolumetricCloudScapes、VolumetricStar）は view を使わない。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingType type, RaytracingView view);

		/**
		* [EN]
		* Records the rain/snow particle simulation. It advances one shared
		* world-space simulation, so it runs once per frame, not once per view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 雨/雪パーティクルのシミュレーションを記録する。ワールド空間で共有する
		* シミュレーションを 1 つ進めるため、ビューごとではなくフレームに 1 回
		* 実行する。
		*/
		void SimulateWeatherParticles(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Draws the rain/snow particles for one view from its own camera. The
		* G-Buffer depth must already be written, and the caller must be inside
		* a GeometryBuffer::BeginDepth()/EndDepth() scope.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 つのビューについて、そのカメラから雨/雪パーティクルを描画する。
		* G-Buffer の深度が書き込み済みで、呼び出し側が
		* GeometryBuffer::BeginDepth()/EndDepth() のスコープ内にいること。
		*/
		void DrawWeatherParticles(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		/**
		* [EN]
		* Copies each pass's tuning values and on/off switch. While every pass
		* that traces the TLAS is off, Build skips the BLAS/TLAS work entirely,
		* so turning everything off costs nothing on the GPU.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各パスの調整値とオン/オフを写す。TLAS をトレースするパスが全て
		* オフの間は Build が BLAS/TLAS の処理を丸ごと省くため、全てオフに
		* すれば GPU のコストはかからない。
		*/
		void SetRaytracingSettings(const RaytracingContext& settings);

	private:
		/// [EN] Maximum number of instances put into the TLAS. Must equal ReflectionRenderer's instance-table capacity, since the ray-traced passes look up that table with InstanceID().
		/// [JP] TLAS に入れるインスタンスの最大数。レイトレーシングのパスは InstanceID() で ReflectionRenderer のインスタンステーブルを引くため、そのテーブルの容量と等しくする。
		SC_CONST Uint32 maxInstances_ = 4096;

		/**
		* [EN]
		* One actor collected by Gather, waiting to become a TLAS instance in
		* Build.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Gather が集めたアクター 1 体。Build で TLAS インスタンスになるのを
		* 待っている。
		*/
		struct PendingInstance
		{
			/// [EN] The mesh the actor draws.
			/// [JP] アクターが描画するメッシュ。
			const Crister* crister_ = nullptr;

			/// [EN] The actor's world matrix, in the engine's row-vector convention.
			/// [JP] アクターのワールド行列。エンジンの行ベクトル規約。
			Matrix worldMatrix_ = Matrix::Identity;

			/// [EN] Key of the per-entity BLAS and blend-buffer caches.
			/// [JP] エンティティごとの BLAS とブレンドバッファのキャッシュのキー。
			EntityID entityID_;

			/// [EN] True when an Animator posed the skeleton this frame; the instance then uses a skinned BLAS.
			/// [JP] 今フレーム Animator が骨格のポーズを付けたとき true。インスタンスはスキン済み BLAS を使う。
			Bool hasSkeletalPose_ = false;

			/// [EN] First bone matrix of this actor in ModelRenderer's shared bone buffer.
			/// [JP] ModelRenderer の共有ボーンバッファにおける、このアクターの先頭ボーン行列。
			Uint32 boneOffset_ = 0;

			/// [EN] Sampled morph weights, indexed like crister_->SubMeshes(). An entry is empty when that SubMesh has no morph targets or no animated weights this frame.
			/// [JP] サンプリング済みのモーフウェイト。crister_->SubMeshes() と同じ添字で引く。その SubMesh にモーフターゲットが無いか、今フレームのアニメーションのウェイトが無ければ空。
			DynamicArray<DynamicArray<Float>> morphWeights_;

			/// [EN] True when at least one entry of morphWeights_ is non-empty; gates the whole morph path for this instance.
			/// [JP] morphWeights_ に空でない要素が 1 つでもあれば true。このインスタンスのモーフ経路全体の条件になる。
			Bool hasMorphWeights_ = false;
		};

		/**
		* [EN]
		* A GPU-writable position buffer that receives morph-blended or skinned
		* vertex positions, grown on demand and reused across frames. Its views
		* live in the bindless heap, so the blend passes reach it through
		* their DispatchBuffers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モーフブレンド済み、またはスキン済みの頂点位置を受け取る、GPU から
		* 書き込める位置バッファ。必要に応じて拡張し、フレームを跨いで使い回す。
		* ビューは bindless ヒープに置き、ブレンドパスは DispatchBuffer 経由で
		* 参照する。
		*/
		struct SkinnedPositionBuffer
		{
			/// [EN] The buffer, float3 per vertex.
			/// [JP] バッファ本体。頂点ごとに float3。
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

			/// [EN] Number of vertices resource_ can hold.
			/// [JP] resource_ が保持できる頂点数。
			Uint32 capacity_ = 0;

			/// [EN] The state resource_ was left in. The buffer is reused across frames, so the first barrier of a frame starts from the state the previous frame ended in.
			/// [JP] resource_ が置かれている状態。バッファはフレームを跨いで使い回すため、フレーム最初のバリアは前フレームが終えた状態から遷移する。
			D3D12_RESOURCE_STATES state_ = D3D12_RESOURCE_STATE_COMMON;

			/// [EN] Bindless SRV of resource_, read when the morph-blended positions feed the skin pass.
			/// [JP] resource_ の bindless SRV。モーフブレンド済み位置をスキンのパスへ渡すときに読む。
			Uint32 shaderResourceViewIndex_ = SC_INVALID;

			/// [EN] Bindless UAV of resource_, written by the blend pass.
			/// [JP] resource_ の bindless UAV。ブレンドパスが書き込む。
			Uint32 unorderedAccessViewIndex_ = SC_INVALID;
		};

		/**
		* [EN]
		* What one MorphBlendCS dispatch of one (entity, SubMesh) reads besides
		* the RT proxy: this frame's weights and the dispatch's constants. Both
		* are frame-ring buffers in the bindless heap, created once, since the
		* SubMesh's morph target count never changes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 つの (エンティティ, SubMesh) の MorphBlendCS のディスパッチが、
		* RT プロキシ以外に読むもの。今フレームのウェイトと、ディスパッチの
		* 定数。どちらも bindless ヒープ上のフレームリングのバッファで、
		* SubMesh のモーフターゲット数は変わらないため 1 度だけ生成する。
		*/
		struct MorphBlendBuffer
		{
			/// [EN] This frame's weights, one float per morph target.
			/// [JP] 今フレームのウェイト。モーフターゲットごとに float 1 つ。
			ResourcePtr<ReadOnlyStructuredBuffer<Float>> weights_;

			/// [EN] The dispatch's MorphBlendDispatchBuffer.
			/// [JP] ディスパッチの MorphBlendDispatchBuffer。
			ResourcePtr<StaticConstantBuffer<MorphBlendDispatchBuffer>> dispatchBuffer_;
		};

		/**
		* [EN]
		* The per-Crister tables that Reflection.hlsli's ResolveReflectionMaterial
		* reads on a ray hit: the material array, and the index into it for every
		* triangle of the RT proxy. Both are small, built once per unique mesh, and
		* read once per hit, so they live on an UPLOAD heap and are written with a
		* single Map/memcpy instead of a DEFAULT heap plus a copy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイが当たったときに Reflection.hlsli の ResolveReflectionMaterial が
		* 読む、Crister ごとのテーブル。マテリアル配列と、RT プロキシの
		* 三角形ごとのそこへのインデックス。どちらも小さく、ユニークな
		* メッシュごとに 1 度だけ構築し、ヒットごとに 1 回読むだけなので、
		* DEFAULT ヒープとコピーではなく UPLOAD ヒープに置き、1 回の
		* Map/memcpy で書き込む。
		*/
		struct MaterialTable
		{
			/// [EN] ReflectionMaterialData per material.
			/// [JP] マテリアルごとの ReflectionMaterialData。
			Microsoft::WRL::ComPtr<ID3D12Resource> materialsResource_;

			/// [EN] Bindless slot of the view over materialsResource_.
			/// [JP] materialsResource_ のビューの bindless スロット。
			Uint32 materialsShaderResourceViewIndex_ = 0;

			/// [EN] Material index per RT-proxy triangle.
			/// [JP] RT プロキシの三角形ごとのマテリアルインデックス。
			Microsoft::WRL::ComPtr<ID3D12Resource> triangleMaterialIndexResource_;

			/// [EN] Bindless slot of the view over triangleMaterialIndexResource_.
			/// [JP] triangleMaterialIndexResource_ のビューの bindless スロット。
			Uint32 triangleMaterialIndexShaderResourceViewIndex_ = 0;
		};

		/// [EN] Material tables per unique mesh, kept as long as the mesh's static BLAS.
		/// [JP] ユニークなメッシュごとのマテリアルテーブル。メッシュの静的 BLAS と同じ期間保持する。
		std::unordered_map<const Crister*, MaterialTable> materialTableCache_;

		/// [EN] Bind-pose BLAS per unique mesh, shared by every frame-ring slot.
		/// [JP] ユニークなメッシュごとのバインドポーズの BLAS。全フレームリングスロットで共有する。
		std::unordered_map<const Crister*, ResourcePtr<BottomLevelAccelerationStructure>> blasCache_;

		/// [EN] Meshes no longer seen by Gather, with the number of frames left before their blasCache_ and materialTableCache_ entries are erased. blasCache_ is shared by every frame-ring slot, so an entry must outlive the frames still in flight that trace it.
		/// [JP] Gather で見えなくなったメッシュと、その blasCache_ と materialTableCache_ のエントリを消去するまでの残りフレーム数。blasCache_ は全フレームリングスロットで共有するため、エントリはそれをトレースする実行中のフレームより長く生きる必要がある。
		std::unordered_map<const Crister*, Uint32> pendingBlasEviction_;

		/// [EN] BLAS per skinned entity, per frame-ring slot.
		/// [JP] スキンのあるエンティティごと、フレームリングスロットごとの BLAS。
		std::unordered_map<EntityID, ResourcePtr<BottomLevelAccelerationStructure>> skinnedBlasCache_[FrameRing::frameCount];

		/// [EN] Skinned positions per skinned entity, per frame-ring slot; the vertex input of skinnedBlasCache_.
		/// [JP] スキンのあるエンティティごと、フレームリングスロットごとのスキン済み位置。skinnedBlasCache_ の頂点入力。
		std::unordered_map<EntityID, SkinnedPositionBuffer> skinnedPositionBuffers_[FrameRing::frameCount];

		/// [EN] SkinBlendDispatchBuffer per skinned entity. StaticConstantBuffer rings over the frame-ring slots itself, so one per entity is enough.
		/// [JP] スキンのあるエンティティごとの SkinBlendDispatchBuffer。StaticConstantBuffer 自体がフレームリングスロットを巡回するため、エンティティごとに 1 つでよい。
		std::unordered_map<EntityID, ResourcePtr<StaticConstantBuffer<SkinBlendDispatchBuffer>>> skinBlendDispatchBuffers_;

		/// [EN] Compute shader that skins RT-proxy positions with the bone matrices.
		/// [JP] RT プロキシの位置をボーン行列でスキニングするコンピュートシェーダ。
		SkinBlendShader skinBlendShader_;

		/// [EN] Morph-blended positions per morphed entity, per frame-ring slot: the base positions with the vertex ranges of weighted SubMeshes overwritten by MorphBlendCS. Morph composes before skin, so this is the skin input of a skinned-and-morphed entity, and the BLAS input of a morph-only one.
		/// [JP] モーフのあるエンティティごと、フレームリングスロットごとのモーフブレンド済み位置。ベース位置のうち、ウェイトのある SubMesh の頂点範囲を MorphBlendCS が上書きしたもの。モーフはスキンより前に合成するため、スキンとモーフの両方を持つエンティティではスキンの入力に、モーフのみのエンティティでは BLAS の入力になる。
		std::unordered_map<EntityID, SkinnedPositionBuffer> morphedPositionBuffers_[FrameRing::frameCount];

		/// [EN] MorphBlendBuffer per morphed entity, indexed like crister_->SubMeshes(). Its buffers ring over the frame-ring slots themselves, so one per entity is enough.
		/// [JP] モーフのあるエンティティごとの MorphBlendBuffer。crister_->SubMeshes() と同じ添字で引く。中のバッファ自体がフレームリングスロットを巡回するため、エンティティごとに 1 つでよい。
		std::unordered_map<EntityID, DynamicArray<MorphBlendBuffer>> morphBlendBuffers_;

		/// [EN] BLAS per morph-only entity, per frame-ring slot, built straight from morphedPositionBuffers_.
		/// [JP] モーフのみのエンティティごと、フレームリングスロットごとの BLAS。morphedPositionBuffers_ から直接構築する。
		std::unordered_map<EntityID, ResourcePtr<BottomLevelAccelerationStructure>> morphedBlasCache_[FrameRing::frameCount];

		/// [EN] Compute shader that adds weighted morph deltas to RT-proxy positions.
		/// [JP] RT プロキシの位置に、ウェイトを掛けたモーフの差分を加えるコンピュートシェーダ。
		MorphBlendShader morphBlendShader_;

		/// [EN] One entry per active Mesh actor, collected by Gather and turned into TLAS instances by Build.
		/// [JP] 有効な Mesh アクターごとに 1 エントリ。Gather が集め、Build が TLAS インスタンスにする。
		DynamicArray<PendingInstance> pendingInstances_;

		/// [EN] Meshes seen by Gather without a cached static BLAS, built at the start of Build where a command list is available.
		/// [JP] Gather で見つかった、静的 BLAS が未キャッシュのメッシュ。コマンドリストのある Build の先頭で構築する。
		DynamicArray<const Crister*> pendingBlasBuilds_;

		/// [EN] The scene TLAS, rebuilt every frame.
		/// [JP] シーンの TLAS。毎フレーム再構築する。
		TopLevelAccelerationStructure tlas_;

		ResourcePtr<ShadowRenderer> shadowRenderer_;
		ShadowRayConstantBuffer shadowSettings_;
		Bool shadowEnabled_ = true;

		ResourcePtr<AmbientOcclusionRenderer> ambientOcclusionRenderer_;
		AmbientOcclusionRayConstantBuffer ambientOcclusionSettings_;
		Bool ambientOcclusionEnabled_ = false;

		ResourcePtr<SubsurfaceScatteringRenderer> subsurfaceScatteringRenderer_;
		SubsurfaceScatteringRayConstantBuffer subsurfaceScatteringSettings_;
		Bool subsurfaceScatteringEnabled_ = false;

		ResourcePtr<ReflectionRenderer> reflectionRenderer_;
		ReflectionRayConstantBuffer reflectionSettings_;
		Bool reflectionEnabled_ = false;

		ResourcePtr<RefractionRenderer> refractionRenderer_;
		RefractionRayConstantBuffer refractionSettings_;
		Bool refractionEnabled_ = false;

		ResourcePtr<GlobalIlluminationRenderer> globalIlluminationRenderer_;
		GlobalIlluminationRayConstantBuffer globalIlluminationSettings_;
		Bool globalIlluminationEnabled_ = false;

		ResourcePtr<VolumetricCloudScapesRenderer> volumetricCloudScapesRenderer_;
		VolumetricCloudScapesRayConstantBuffer volumetricCloudScapesSettings_;
		Bool volumetricCloudScapesEnabled_ = false;

		ResourcePtr<VolumetricStarRenderer> volumetricStarRenderer_;
		VolumetricStarRayConstantBuffer volumetricStarSettings_;
		Bool volumetricStarEnabled_ = false;

		ResourcePtr<WeatherParticleRenderer> weatherParticleRenderer_;

		ResourcePtr<VolumetricLightRenderer> volumetricLightRenderer_;
		VolumetricLightRayConstantBuffer volumetricLightSettings_;
		Bool volumetricLightEnabled_ = false;

		/// [EN] Heap the TLAS views and material-table views are written into.
		/// [JP] TLAS のビューとマテリアルテーブルのビューを書き込むヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Receives the TLAS index every frame in Build.
		/// [JP] Build で毎フレーム TLAS のインデックスを受け取る。
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;

		/// [EN] Bindless slot of the TLAS view per frame-ring slot, so a new frame never overwrites the view an in-flight frame still reads.
		/// [JP] フレームリングスロットごとの TLAS ビューの bindless スロット。新しいフレームが、実行中のフレームが読むビューを上書きしないようにする。
		Uint32 tlasBindlessIndices_[FrameRing::frameCount] = { 0xFFFFFFFF, 0xFFFFFFFF };

		/// [EN] Each failure below is logged only on its first occurrence, not every frame.
		/// [JP] 以下の各失敗は毎フレームではなく、初回のみログに出す。
		Bool blasBuildFailureLogged_ = false;
		Bool tlasBuildFailureLogged_ = false;
		Bool skinnedBlasBuildFailureLogged_ = false;
		Bool morphedBlasBuildFailureLogged_ = false;
		Bool degenerateInstanceLogged_ = false;
		Bool rtProxyNotReadyLogged_ = false;
		Bool instanceLimitLogged_ = false;
		Bool deviceRemovedLogged_ = false;
	};
}

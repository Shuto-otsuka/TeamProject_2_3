#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/D3D12/Buffer/FrameBuffer.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/RaytracingStateObject.h>
#include <GraphicsEngine/System/IndicesSystem.h>
#include <GraphicsEngine/System/LightSystem.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/System/WeatherSystem.h>
#include <GraphicsEngine/System/AnimationSystem.h>
#include <GraphicsEngine/System/ConstraintSystem.h>

#include <GraphicsEngine/D3D12/Buffer/GeometryBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/HiZBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/DepthResizeBuffer.h>
#include <GraphicsEngine/Model/Material/MaterialResolveShader.h>
#include <GraphicsEngine/Model/Material/MaterialSortBuffer.h>
#include <GraphicsEngine/Renderer/TextureRenderer.h>
#include <GraphicsEngine/Renderer/FontRenderer.h>
#include <GraphicsEngine/Renderer/MovieRenderer.h>
#include <GraphicsEngine/Renderer/ModelRenderer.h>
#include <GraphicsEngine/Renderer/OutlineRenderer.h>
#include <GraphicsEngine/Renderer/HUDComposeRenderer.h>
#include <GraphicsEngine/Renderer/ColliderRenderer.h>
#include <GraphicsEngine/Renderer/ShapeRenderer.h>
#include <GraphicsEngine/Renderer/RaytracingRenderer.h>
#include <GraphicsEngine/Renderer/SkyRenderer.h>
#include <GraphicsEngine/Renderer/TimelineRenderer.h>
#include <GraphicsEngine/Renderer/ModelTransformRenderer.h>
#include <GraphicsEngine/Renderer/MaterialRenderer.h>
#include <GraphicsEngine/Renderer/SkeletonControllerRenderer.h>
#include <GraphicsEngine/Renderer/AvatarRenderer.h>
#include <GraphicsEngine/Renderer/EffekseerRenderer.h>
#include <GraphicsEngine/Renderer/PostProcessRenderer.h>
#include <GraphicsEngine/D3D12/Buffer/HudlessBuffer.h>
#include <GraphicsEngine/Renderer/DlssRayReconstructionRenderer.h>
#include <GraphicsEngine/Renderer/TaauUpsamplingRenderer.h>
#include <GraphicsEngine/Renderer/ViewMode.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/Profiler/GpuProfiler.h>

namespace SeedCore
{
	struct LoaderSystem;
	class ResourceCache;
	class BindlessHeap;
	class D3D12CommandList;
	class World;
	class ShaderCache;
	class ShaderHotReload;
	class SceneSystem;

	class Renderer :public NonCopyable
	{
	public:
		Renderer();
		~Renderer() = default;

		void Create(ID3D12Device* device, ID3D12CommandQueue* commandQueue, Uint32 swapBufferCount, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderHotReload& shaderHotReload, Uint32 width, Uint32 height);

		void Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderHotReload& shaderHotReload, Uint32 nativeWidth, Uint32 nativeHeight, Uint32 outputWidth, Uint32 outputHeight);

	public:
		void PrepareFrame(D3D12CommandList* cmdList, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world, const SceneConstantBuffer& scene, Float deltaTime, std::span<const Entity> selectedEntities);

		void BeginEditorFrame(D3D12CommandList* cmdList);

		void EndEditorFrame(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void BeginGameFrame(D3D12CommandList* cmdList);

		void EndGameFrame(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void BeginCanvasFrame(D3D12CommandList* cmdList);

		void EndCanvasFrame(D3D12CommandList* cmdList);

		void BeginTimelineFrame(D3D12CommandList* cmdList);

		void EndTimelineFrame(D3D12CommandList* cmdList);

		void BeginModelTransformFrame(D3D12CommandList* cmdList);

		void EndModelTransformFrame(D3D12CommandList* cmdList);

		void BeginMaterialFrame(D3D12CommandList* cmdList);

		void EndMaterialFrame(D3D12CommandList* cmdList);

		void BeginSkeletonControllerFrame(D3D12CommandList* cmdList);

		void EndSkeletonControllerFrame(D3D12CommandList* cmdList);

		void BeginAvatarFrame(D3D12CommandList* cmdList);

		void EndAvatarFrame(D3D12CommandList* cmdList);

	public:
		void UploadColliders(std::span<const ColliderDesc> colliders);

		void UploadShapes(std::span<const ShapeDesc> shapes);

		void GatherTimelinePreview(LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix);

		void GatherModelTransformPreview(LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix);

		void GatherMaterialPreview(LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 surfaceAssetId, const Matrix& worldMatrix);

		void GatherSkeletonControllerPreview(LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix, Int selectedNodeIndex);

		void GatherAvatarPreview(const AvatarMesh& mesh, Uint32 boneCount, const Matrix& worldMatrix, std::span<const Uint32> regionTextureIndices);

	public:
		void Raytracing(const RaytracingContext& settings);

		void Upscale(Bool dlssRayReconstructionEnabled, UpscaleMode upscaleMode);

		[[nodiscard]] Vector2 PostProcessOutputSize()const;

	public:
		void EditorFlush(D3D12CommandList* cmdList, SceneSystem* sceneSystem, ViewMode viewMode);

		void GameFlush(D3D12CommandList* cmdList, SceneSystem* sceneSystem, Float deltaTime, Bool hasActiveCamera);

		void CanvasFlush(D3D12CommandList* cmdList, SceneSystem* sceneSystem);

		void TimelineFlush(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void ModelTransformFlush(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void MaterialFlush(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void SkeletonControllerFlush(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

		void AvatarFlush(D3D12CommandList* cmdList, const SceneConstantBuffer& scene);

	public:
		[[nodiscard]] const GpuProfiler& GetGpuProfiler()const;

		[[nodiscard]] FrameBuffer* GetEditorFrameBuffer()const;

		[[nodiscard]] FrameBuffer* GetGameFrameBuffer()const;

		[[nodiscard]] FrameBuffer* GetCanvasFrameBuffer()const;

		[[nodiscard]] ID3D12Resource* GameDisplayResource()const;

	public:
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE EditorDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GameDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE CanvasDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE TimelineDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE ModelTransformDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE MaterialDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE SkeletonControllerDisplayGPUHandle()const;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE AvatarDisplayGPUHandle()const;

	private:
		RootSignature rootSignature_;
		PipelineStateObject pipelineStateObject_;
		RaytracingStateObject raytracingStateObject_;

		ResourcePtr<EffekseerManager> effekseerManager_;

		ResourcePtr<ConstantIndicesSystem> constantIndicesSystem_;
		ResourcePtr<ShaderResourceIndicesSystem> shaderResourceIndicesSystem_;
		ResourcePtr<UnorderedAccessIndicesSystem> unorderedAccessIndicesSystem_;

		ResourcePtr<LightSystem> lightSystem_;

		AnimationSystem animationSystem_;

		ConstraintSystem constraintSystem_;

	private:
		ResourcePtr<TextureRenderer> textureRenderer_;

		ResourcePtr<FontRenderer> fontRenderer_;

		ResourcePtr<MovieRenderer> movieRenderer_;

		ResourcePtr<ModelRenderer> modelRenderer_;

		ResourcePtr<OutlineRenderer> outlineRenderer_;

		ResourcePtr<HUDComposeRenderer> hudComposeRenderer_;

		ResourcePtr<ColliderRenderer> colliderRenderer_;

		ResourcePtr<ShapeRenderer> shapeRenderer_;

		ResourcePtr<RaytracingRenderer> raytracingRenderer_;

		ResourcePtr<SkyRenderer> skyRenderer_;

		ResourcePtr<TimelineRenderer> timelineRenderer_;

		ResourcePtr<ModelTransformRenderer> modelTransformRenderer_;

		ResourcePtr<MaterialRenderer> materialRenderer_;

		ResourcePtr<SkeletonControllerRenderer> skeletonControllerRenderer_;

		ResourcePtr<AvatarRenderer> avatarRenderer_;

		ResourcePtr<EffekseerRenderer> effekseerRenderer_;

		ResourcePtr<PostProcessRenderer> postProcessRenderer_;

		ResourcePtr<DlssRayReconstructionRenderer> dlssRayReconstructionRenderer_;

		ResourcePtr<TaauUpsamplingRenderer> taauUpsamplingRenderer_;

		UpscaleMode upscaleMode_ = UpscaleMode::Balanced;

		Uint32 nativeWidth_ = 0;
		Uint32 nativeHeight_ = 0;

		Float skyTotalTime_ = 0.0f;

		Bool daySystemEnabled_ = false;
		DaySystemConstantBuffer daySystem_;

		Bool sunLightEnabled_ = false;
		SunLightSettings sunLight_;

		Bool moonLightEnabled_ = false;
		MoonLightSettings moonLight_;

		CelestialResult celestialResult_;

		WeatherGpuState weatherState_;

		WeatherSystem weatherSystem_;

		Vector3 lastCameraPosition_ = { 0.0f, 0.0f, 0.0f };

		GeometryBuffer geometryBuffer_;
		HiZBuffer hiZBuffer_;

		ResourcePtr<MaterialResolveShader> materialResolveShader_;

		MaterialSortBuffer materialSortBuffer_;

		DepthResizeBuffer debugDepthResizeBuffer_;

		GpuProfiler gpuProfiler_;

		DescriptorHeap silhouetteRenderTargetViewHeap_;
		ResourcePtr<FrameBuffer> silhouetteFrameBuffer_;

		DescriptorHeap editorRenderTargetViewHeap_;

		DescriptorHeap editorDepthStencilViewHeap_;

		DescriptorHeap gameRenderTargetViewHeap_;

		DescriptorHeap gameDepthStencilViewHeap_;

		DescriptorHeap canvasRenderTargetViewHeap_;

		DescriptorHeap canvasDepthStencilViewHeap_;

		DescriptorHeap uiColorAlphaRenderTargetViewHeap_;

		ResourcePtr<FrameBuffer> editorFrameBuffer_;

		ResourcePtr<FrameBuffer> gameFrameBuffer_;

		ResourcePtr<FrameBuffer> canvasFrameBuffer_;

		ResourcePtr<FrameBuffer> uiColorAlphaFrameBuffer_;

		HudlessBuffer hudlessBuffer_;

		BindlessHeap* bindlessHeap_ = nullptr;

		ID3D12Device* device_ = nullptr;
	};
}

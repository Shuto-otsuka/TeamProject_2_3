#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/D3D12/Buffer/FrameBuffer.h>
#include <GraphicsEngine/Model/ModelShader.h>
#include <GraphicsEngine/Model/Culling/ModelCullingBuffer.h>
#include <GraphicsEngine/Model/ModelRecord.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveWireframeShader.h>
#include <GraphicsEngine/System/SceneSystem.h>
#include <GraphicsEngine/System/IndicesSystem.h>

namespace SeedCore
{
	struct LoaderSystem;
	class ModelResource;
	class AnimationResource;
	class Crister;
	class BindlessHeap;
	class ShaderCache;
	class RootSignature;
	class PipelineStateObject;
	class D3D12CommandList;

	class SEEDCORE_API SkeletonControllerRenderer :public NonCopyable
	{
	public:
		SkeletonControllerRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~SkeletonControllerRenderer();

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, Uint32 width, Uint32 height);

		void Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, Uint32 width, Uint32 height);

		void Gather(LoaderSystem& loaderSystem, ModelResource& modelResource, AnimationResource& animationResource, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix, Int selectedNodeIndex);

		void Upload();

		void Begin(D3D12CommandList* cmdList);

		void Draw(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const SceneConstantBuffer& scene);

		void DrawBones(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap);

		void End(D3D12CommandList* cmdList);

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE DisplayGPUHandle()const;

	private:
		SC_CONST Uint maxBoneInstanceCount_ = 2048;

		ModelShader modelShader_;
		ModelCullingBuffer modelCullingBuffer_;

		DynamicArray<ModelStructuredBuffer> opaqueInstances_;
		DynamicArray<ModelStructuredBuffer> transparentInstances_;
		DynamicArray<Matrix> boneMatrices_;

		ResourcePtr<ReadOnlyStructuredBuffer<ModelStructuredBuffer>> instanceBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<Matrix>> boneBuffer_;

		Bool hasSkinnedOpaque_ = false;
		Bool uploaded_ = false;

		Uint maxInstanceCount_ = 0;
		Uint maxBoneCount_ = 0;

		Uint64 streamingFrame_ = 0;
		DynamicArray<std::pair<Crister*, Uint32>> streamingRequests_;

		BindlessHeap* bindlessHeap_ = nullptr;

		DescriptorHeap renderTargetViewHeap_;
		DescriptorHeap depthStencilViewHeap_;
		ResourcePtr<FrameBuffer> frameBuffer_;

		ResourcePtr<SceneSystem> sceneSystem_;

		ConstantIndices constantIndices_{};
		ShaderResourceIndices shaderResourceIndices_{};
		ResourcePtr<StaticConstantBuffer<ConstantIndices>> constantIndicesBuffer_;
		ResourcePtr<StaticConstantBuffer<ShaderResourceIndices>> shaderResourceIndicesBuffer_;

		PrimitiveWireframeShader boneLineShader_;

		DynamicArray<PrimitiveWireframeStructuredBuffer> boneInstances_;
		ResourcePtr<ReadOnlyStructuredBuffer<PrimitiveWireframeStructuredBuffer>> boneInstanceBuffer_;
	};
}

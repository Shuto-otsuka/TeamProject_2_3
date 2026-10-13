#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <FoundationEngine/Log/Assert.h>
#include <GraphicsEngine/Model/ModelShader.h>
#include <GraphicsEngine/Model/Transparent/OITBuffer.h>
#include <GraphicsEngine/Model/Culling/ModelCullingBuffer.h>
#include <GraphicsEngine/Model/ModelRecord.h>
#include <GraphicsEngine/Model/SoftbodyMesh.h>
#include <GraphicsEngine/System/SceneSystem.h>

namespace SeedCore
{
	struct RootAddresses;
	struct GraphicsResourceSystem;

	class Crister;
	class World;
	class BindlessHeap;
	class ShaderCache;
	class PipelineStateObject;
	class ConstantIndicesSystem;
	class ShaderResourceIndicesSystem;
	class UnorderedAccessIndicesSystem;
	class D3D12CommandList;
	class FrameBuffer;
	class GeometryBuffer;

	/**
	* [EN]
	* Where the shell-fur forward pass' fur instances live in the shared
	* instance buffer. Mirrors the HLSL FurConstantBuffer
	* (Model/Model.hlsli); 16 bytes. Reached through
	* ConstantIndices::furIndex_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シェルファー前方パスのファーインスタンスが共有インスタンスバッファの
	* どこにあるか。HLSL の FurConstantBuffer（Model/Model.hlsli）と
	* 一致。16 バイト。ConstantIndices::furIndex_
	* 経由で参照する。
	*/
	struct FurConstantBuffer
	{
		Uint furInstanceOffset_ = 0;
		Uint furInstanceCount_ = 0;
		Vector2 furConstantBufferPadding0_;
	};
	SC_STATIC_ASSERT(FurConstantBuffer, 16, "Model/Model.hlsli");

	class SEEDCORE_API ModelRenderer
	{
	public:
		ModelRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~ModelRenderer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		void Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, ConstantIndicesSystem& constantIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		void Gather(GraphicsResourceSystem& resources, World& world, const SceneConstantBuffer& scene, std::span<const Entity> selectedEntities = {});

		void Upload();

		void DrawDepthPrepass(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawOpaque(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void Compose(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawWireframe(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawMeshlet(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawTransparent(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawFurShell(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawSilhouette(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		[[nodiscard]] Uint BoneMatrixBufferIndex()const;

		[[nodiscard]] Bool TryGetAnimatedBoneOffset(EntityID entityID, Uint32& outBoneOffset)const;

		[[nodiscard]] Bool TryGetAnimatedMorphWeights(EntityID entityID, Int nodeIndex, DynamicArray<Float>& outWeights)const;

	private:
		SC_CONST Uint32 furShellMax_ = 32;

		DynamicArray<ModelStructuredBuffer> opaqueInstances_;
		DynamicArray<ModelStructuredBuffer> transparentInstances_;
		DynamicArray<ModelStructuredBuffer> furInstances_;

		ResourcePtr<StaticConstantBuffer<FurConstantBuffer>> modelFurConstantBuffer_;

		ResourcePtr<ReadOnlyStructuredBuffer<ModelStructuredBuffer>> instanceBuffer_;

		DynamicArray<Matrix> boneMatrices_;

		DynamicArray<Matrix> previousBoneMatrices_;

		ResourcePtr<ReadOnlyStructuredBuffer<Matrix>> boneBuffer_;

		ResourcePtr<ReadOnlyStructuredBuffer<Matrix>> previousBoneBuffer_;

		std::unordered_map<EntityID, Uint32> animatedBoneOffsets_;

		std::unordered_map<EntityID, DynamicArray<Matrix>> animatedBonePalettes_;

		std::unordered_map<EntityID, DynamicArray<Matrix>> previousAnimatedBonePalettes_;

		DynamicArray<Float> morphWeights_;

		DynamicArray<Float> previousMorphWeights_;

		ResourcePtr<ReadOnlyStructuredBuffer<Float>> morphWeightBuffer_;

		ResourcePtr<ReadOnlyStructuredBuffer<Float>> previousMorphWeightBuffer_;

		std::unordered_map<EntityID, std::unordered_map<Int, DynamicArray<Float>>> animatedMorphWeights_;

		std::unordered_map<EntityID, std::unordered_map<Int, DynamicArray<Float>>> previousAnimatedMorphWeights_;

		struct AnimatorHistory
		{
			Int stateIndex_ = -1;
			Float time_ = 0.0f;
		};

		std::unordered_map<EntityID, AnimatorHistory> animatorHistories_;

		std::unordered_map<EntityID, AnimatorHistory> previousAnimatorHistories_;

		std::unordered_map<EntityID, Matrix> previousWorldMatrices_;

		std::unordered_map<EntityID, ResourcePtr<SoftbodyMesh>> softbodyMeshes_;

		Bool hasSkinnedOpaque_ = false;
		Bool hasSkinnedTransparent_ = false;
		Bool hasSelectedInstance_ = false;
		Bool hasSelectedSkinned_ = false;
		Bool uploaded_ = false;

		ModelShader modelShader_;
		OITBuffer oitBuffer_;
		ModelCullingBuffer modelCullingBuffer_;

		ID3D12Device* device_ = nullptr;
		BindlessHeap* bindlessHeap_ = nullptr;
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;

		Uint maxInstanceCount_ = 0;
		Uint maxBoneCount_ = 0;
		Uint maxMorphWeightCount_ = 0;

		struct TextureStreamingRequest
		{
			Crister* crister_ = nullptr;
			Uint32 textureIndex_ = 0;
			Uint32 desiredMip_ = 0;
		};

		Uint64 streamingFrame_ = 0;
		DynamicArray<std::pair<Crister*, Uint32>> geometryStreamingRequests_;
		DynamicArray<TextureStreamingRequest> textureStreamingRequests_;
	};
}

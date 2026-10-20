#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/Texture/TextureShader.h>

namespace SeedCore
{
	struct RootAddresses;
	class TextureResource;
	class World;
	class BindlessHeap;
	class ShaderCache;
	class PipelineStateObject;
	class ShaderResourceIndicesSystem;

	class TextureRenderer
	{
	private:
		struct TextureSpriteStructuredBuffer
		{
			Vector2 position_;
			Float rotation_;
			Vector2 scale_;
			Uint batch_;
			Vector2 textureSize_;
			Vector2 texturePosition_;
			Vector2 pivot_;
			Vector2 padding1_;
			Color color_;
			Uint textureIndex_;
			Float scrollSpeed_;
			Vector2 scrollDirection_;
			Uint motionType_;
			Uint selected_;
			Vector3 padding2_;
		};

		struct TextureBillboardStructuredBuffer
		{
			Vector3 position_;
			Vector3 rotation_;
			Vector2 scale_;
			Vector2 textureSize_;
			Vector2 texturePosition_;
			Vector2 pivot_;
			Vector2 padding0_;
			Color color_;
			Uint textureIndex_;
			Float scrollSpeed_;
			Vector2 scrollDirection_;
			Uint motionType_;
			Uint faceCamera_;
			Uint selected_;
			Uint batch_;
			Float padding1_;
		};

		struct SpriteBatch
		{
			Int order_;
			Uint spriteStart_;
			Uint spriteCount_;
			Uint canvasStart_;
			Uint canvasCount_;
		};

		struct OrderedEntity
		{
			Int order_;
			EntityID entityID_;
		};

	public:
		TextureRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~TextureRenderer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderResourceIndicesSystem& shaderResourceIndicesSystem);

		void Gather(TextureResource& resource, World& world, Vector2 nativeScreenSize, std::span<const Entity> selectedEntities = {});

		void Upload();

		NODISCARD Size GetSpriteBatchCount()const;

		NODISCARD Int GetSpriteBatchOrder(Size index)const;

		void DrawSpriteBatch(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Size index);

		void DrawCanvasBatch(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Size index);

		void DrawWorldBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawSilhouetteSprite(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

		void DrawSilhouetteBillboard(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		void DrawSpriteRange(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint start, Uint count);

		void DrawBillboardRange(ID3D12GraphicsCommandList6* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, Uint start, Uint count);

	private:
		DynamicArray<TextureSpriteStructuredBuffer> spriteInstances_;
		DynamicArray<TextureBillboardStructuredBuffer> billboardInstances_;
		DynamicArray<TextureBillboardStructuredBuffer> canvasInstances_;

		DynamicArray<SpriteBatch> spriteBatches_;
		DynamicArray<OrderedEntity> orderedEntities_;

		Uint worldBillboardCount_ = 0;
		Uint batchSerial_ = 0;

		ResourcePtr<ReadOnlyStructuredBuffer<TextureSpriteStructuredBuffer>> spriteBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<TextureBillboardStructuredBuffer>> billboardBuffer_;

		Bool hasSelectedSpriteInstance_ = false;
		Bool hasSelectedBillboardInstance_ = false;

		TextureShader imageShader_;

		BindlessHeap* bindlessHeap_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;

		Uint maxCount_ = 0;

		Uint64 streamingFrame_ = 0;
	};
}

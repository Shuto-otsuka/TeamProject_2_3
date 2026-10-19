#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <GraphicsEngine/Font/FontShader.h>

namespace SeedCore
{
	struct RootAddresses;
	class World;
	class BindlessHeap;
	class ShaderCache;
	class PipelineStateObject;
	class ShaderResourceIndicesSystem;
	class FontResource;

	class FontRenderer
	{
	private:
		struct FontSpriteStructuredBuffer
		{
			Vector2 position_;
			Vector2 size_;
			Vector2 uvMin_;
			Vector2 uvMax_;

			Color color_;
			Color outlineColor_;
			Color glowColor_;

			Uint textureIndex_;
			Float outlineWidth_;
			Float glowPower_;
			Float padding1_;

			Vector2 unitRange_;
			Uint selected_;
			Uint batch_;
		};

		struct FontBillboardStructuredBuffer
		{
			Vector3 position_;
			Float padding0_;
			Vector3 rotation_;
			Float padding1_;

			Vector2 localPosition_;
			Vector2 localSize_;
			Vector2 uvMin_;
			Vector2 uvMax_;

			Color color_;
			Color outlineColor_;
			Color glowColor_;

			Uint textureIndex_;
			Float outlineWidth_;
			Float glowPower_;
			Uint batch_;

			Vector2 unitRange_;
			Uint faceCamera_;
			Uint selected_;
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
		FontRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~FontRenderer() = default;

		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ShaderResourceIndicesSystem& shaderResourceIndicesSystem);

		void Gather(FontResource& fontResource, World& world, Vector2 nativeScreenSize, std::span<const Entity> selectedEntities = {});

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
		DynamicArray<FontSpriteStructuredBuffer> spriteInstances_;
		DynamicArray<FontBillboardStructuredBuffer> billboardInstances_;
		DynamicArray<FontBillboardStructuredBuffer> canvasInstances_;

		DynamicArray<SpriteBatch> spriteBatches_;
		DynamicArray<OrderedEntity> orderedEntities_;

		Uint worldBillboardCount_ = 0;
		Uint batchSerial_ = 0;

		ResourcePtr<ReadOnlyStructuredBuffer<FontSpriteStructuredBuffer>> spriteBuffer_;
		ResourcePtr<ReadOnlyStructuredBuffer<FontBillboardStructuredBuffer>> billboardBuffer_;

		FontShader fontShader_;

		BindlessHeap* bindlessHeap_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;

		Uint maxCount_ = 0;
	};
}

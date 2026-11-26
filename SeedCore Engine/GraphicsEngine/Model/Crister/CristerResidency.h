#pragma once

namespace SeedCore
{
	class Crister;
	class BindlessHeap;
	class D3D12TransferLane;
	struct Bitmap;

	class SEEDCORE_API CristerResidency
	{
	public:
		CristerResidency() = default;

		~CristerResidency();

		CristerResidency(CristerResidency&&)noexcept = default;

		CristerResidency& operator=(CristerResidency&&)noexcept = default;

		void Release(BindlessHeap* heap);

		void Upload(Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap);

		NODISCARD Vector3 PlacedPositionMin()const;

		NODISCARD Vector3 PlacedPositionExtent()const;

		NODISCARD D3D12_GPU_VIRTUAL_ADDRESS PositionBufferAddress()const;

		NODISCARD Uint PositionBufferIndex()const;

		NODISCARD Uint32 VertexCount()const;

		NODISCARD D3D12_GPU_VIRTUAL_ADDRESS IndexBufferAddress()const;

		NODISCARD Uint32 IndexCount()const;

		NODISCARD Uint TextureBindlessIndex(Uint32 textureIndex)const;

		NODISCARD Bool MaterialsDirty()const;

		void ClearMaterialsDirty()const;

		NODISCARD Uint VertexBufferIndex()const;

		NODISCARD Uint IndexBufferIndex()const;

		NODISCARD Uint SkinVertexBufferIndex()const;

		NODISCARD Uint MorphDeltaBufferIndex()const;

		NODISCARD Uint VertexMorphSourceBufferIndex()const;

		NODISCARD Bool ProxySkinned()const;

		NODISCARD Uint ProxySkinVertexBufferIndex()const;

		NODISCARD Uint ProxyMorphDeltaBufferIndex()const;

		NODISCARD std::span<const Matrix> SubMeshPlacement(Size subMeshIndex)const;

		void CopyMorph(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* destination)const;

		void MakeClusterResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 clusterIndex);

		void MakeTextureMipResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap, Uint32 textureIndex, Uint32 targetMip);

		void MakePoolResident(const Crister& crister, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* heap);

		void EvictCluster(BindlessHeap* heap, Uint32 clusterIndex);

		void EvictTextureMip(BindlessHeap* heap, Uint32 textureIndex);

		void EvictPool(BindlessHeap* heap);

		void TouchCluster(Uint32 clusterIndex, Uint64 frame);

		void TouchTexture(Uint32 textureIndex, Uint64 frame);

		NODISCARD Bool ClusterResident(Uint32 clusterIndex)const;

		NODISCARD Uint64 Version()const;

		NODISCARD Bool TextureResident(Uint32 textureIndex)const;

		static void EvictClusterBudget(BindlessHeap* heap, Uint64 currentFrame);

		static void EvictTextureBudget(BindlessHeap* heap, Uint64 currentFrame);

		NODISCARD Uint ClusterVertexBufferIndex(Uint32 clusterIndex)const;

		NODISCARD Uint ClusterMeshletBufferIndex(Uint32 clusterIndex)const;

		NODISCARD Uint ClusterMeshletBoundBufferIndex(Uint32 clusterIndex)const;

		NODISCARD Uint ClusterVertexIndicesBufferIndex(Uint32 clusterIndex)const;

		NODISCARD Uint ClusterPrimitiveIndicesBufferIndex(Uint32 clusterIndex)const;

		NODISCARD Bool StandaloneVertices(Uint32 clusterIndex)const;

		NODISCARD Uint32 TextureFinestMip(Uint32 textureIndex)const;

		NODISCARD Uint32 TextureDesiredMip(const Crister& crister, Uint32 textureIndex, Float texcoordDensity, Float worldScale, Float pixelsPerUnit)const;

	private:
		static Uint CreateStructuredShaderResourceView(ID3D12Device* device, BindlessHeap* heap, ID3D12Resource* resource, Uint elementCount, Uint stride);

		static HRESULT CreateStaticBufferUnbounded(ID3D12Device* device, D3D12TransferLane& lane, const void* data, Uint64 count, Uint64 stride, ID3D12Resource** outResource);

		static void ComputeTextureMipLayout(const Bitmap& bitmap, Uint32 mipIndex, Uint32& outWidth, Uint32& outHeight, Uint64& outRowPitch, Uint64& outSlicePitch, Uint64& outByteOffset);

		struct StreamingGeometry
		{
			Uint32 vertexIndexBegin_ = 0;

			Uint32 vertexIndexEnd_ = 0;

			Uint32 primitiveBegin_ = 0;

			Uint32 primitiveEnd_ = 0;

			Uint32 vertexBegin_ = 0;

			Uint32 vertexEnd_ = 0;

			Bool ownsVertices_ = false;

			Bool pinned_ = false;

			Microsoft::WRL::ComPtr<ID3D12Resource> meshletResource_;

			Microsoft::WRL::ComPtr<ID3D12Resource> meshletBoundResource_;

			Microsoft::WRL::ComPtr<ID3D12Resource> vertexIndicesResource_;

			Microsoft::WRL::ComPtr<ID3D12Resource> primitiveIndicesResource_;

			Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

			Uint meshletBufferIndex_ = SC_INVALID;

			Uint meshletBoundBufferIndex_ = SC_INVALID;

			Uint vertexIndicesBufferIndex_ = SC_INVALID;

			Uint primitiveIndicesBufferIndex_ = SC_INVALID;

			Uint vertexBufferIndex_ = SC_INVALID;

			Bool resident_ = false;

			Uint64 sizeBytes_ = 0;

			Uint64 lastUsedFrame_ = 0;
		};

		struct TextureMipLevel
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

			Uint bindlessIndex_ = SC_INVALID;

			Uint64 sizeBytes_ = 0;
		};

		struct StreamingTexture
		{
			TextureMipLevel pinnedMip_;

			TextureMipLevel currentMip_;

			Uint32 mipCount_ = 0;

			Uint32 topResidentMip_ = 0;

			Uint64 lastUsedFrame_ = 0;

			Bool valid_ = false;
		};

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> positionResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skinVertexResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> raytracingSkinVertexResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> raytracingMorphDeltaResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> morphDeltaResource_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexMorphSourceResource_;

		Uint morphDeltaBufferIndex_ = SC_INVALID;

		Uint vertexMorphSourceBufferIndex_ = SC_INVALID;

		Uint vertexBufferIndex_ = SC_INVALID;

		Uint skinVertexBufferIndex_ = SC_INVALID;

		Uint indexBufferIndex_ = SC_INVALID;

		Uint positionBufferIndex_ = SC_INVALID;

		Uint raytracingSkinVertexBufferIndex_ = SC_INVALID;

		Uint raytracingMorphDeltaBufferIndex_ = SC_INVALID;

		Uint32 triangleIndexCount_ = 0;

		Uint32 proxyVertexCount_ = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> poolResource_;

		Uint poolBufferIndex_ = SC_INVALID;

		Uint32 poolVertexEnd_ = 0;

		Uint32 poolResidentReferences_ = 0;

		Bool poolResident_ = false;

		Bool poolPinned_ = false;

		Uint64 poolSizeBytes_ = 0;

		Uint64 poolLastUsedFrame_ = 0;

		Uint64 version_ = 0;

		DynamicArray<StreamingGeometry> streamingGeometry_;

		DynamicArray<StreamingTexture> streamingTextures_;

		mutable Bool materialsDirty_ = false;

		Bool uploaded_ = false;

		DynamicArray<DynamicArray<Matrix>> subMeshPlacement_;

		Vector3 placedPositionMin_ = { 0,0,0 };

		Vector3 placedPositionExtent_ = { 1,1,1 };

		static inline DynamicArray<CristerResidency*> streamingRegistry_;

		static inline Uint64 totalResidentGeometryBytes_ = 0;

		static inline Uint64 totalResidentTextureBytes_ = 0;

		static inline Uint64 geometryBudgetBytes_ = 512ull * 1024 * 1024;

		static inline Uint64 textureBudgetBytes_ = 256ull * 1024 * 1024;

		SC_CONSTEX Uint64 evictAgeFrames_ = 8;
	};
}
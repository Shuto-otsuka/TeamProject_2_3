#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Raytracing/BottomLevelShader.h>

namespace SeedCore
{
	class BindlessHeap;

	class BottomLevelFeature :public RenderFeature<BottomLevelFeature>
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 65536;

		SC_CONSTEX Uint32 tableCapacity_ = 131072;

		SC_CONSTEX Uint32 vertexCapacity_ = 4194304;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Table, RenderResourceDesc::FixedBuffer(sizeof(BottomLevelTableStructuredBuffer), tableCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_ACCUMULATION(Vertices, RenderResourceDesc::FixedBuffer(sizeof(Vector3), vertexCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(TableUpdates, RenderResourceDesc::FixedBuffer(sizeof(BottomLevelTableUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct UpdateStage;

		struct BuildStage;

	public:
		void Create(RenderDevice& device);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		struct Entry
		{
			Uint64 key_ = 0;

			Bool used_ = false;

			Bool dynamic_ = false;

			Bool requested_ = false;

			Bool built_ = false;

			Uint32 unseenFrames_ = 0;

			Uint32 slot_ = SC_INVALID;

			Uint32 vertexOffset_ = SC_INVALID;

			Uint64 capacity_ = 0;

			Uint64 sentAddress_ = 0;

			Uint32 sentVertexOffset_ = SC_INVALID;

			D3D12_RAYTRACING_GEOMETRY_DESC geometry_{};

			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
		};

		struct Build
		{
			ID3D12Resource* resource_ = nullptr;

			D3D12_RAYTRACING_GEOMETRY_DESC geometry_{};

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags_ = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

			Uint32 vertexOffset_ = SC_INVALID;

			Uint64 scratchOffset_ = 0;
		};

	private:
		NODISCARD Uint32 Acquire(Uint64 key, Bool dynamic);

		void Release(Uint32 handle);

		void Mark(Uint32 slot);

	private:
		ComputePipeline<BottomLevelClearDescription> clearPipeline_;

		ComputePipeline<BottomLevelUpdateDescription> updatePipeline_;

		Microsoft::WRL::ComPtr<ID3D12Device5> device_;

		BindlessHeap* heap_ = nullptr;

		std::unordered_map<Uint64, Uint32> lookup_;

		DynamicArray<Entry> entries_;

		DynamicArray<Uint32> freeHandles_;

		DynamicArray<Uint32> slots_;

		DynamicArray<Uint8> marked_;

		DynamicArray<Uint32> markedSlots_;

		DynamicArray<BottomLevelTableUpdate> updates_;

		DynamicArray<Build> staticBuilds_;

		DynamicArray<Build> dynamicBuilds_;

		Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;

		Uint64 scratchCapacity_ = 0;

		Bool clearing_ = true;

		Bool updating_ = false;

		Bool buildingStatic_ = false;

		Bool buildingDynamic_ = false;
	};
}

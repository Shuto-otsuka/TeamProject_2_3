#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	struct RenderScene;

	class Crister;
	class RenderDevice;
	class RenderFrame;

	class ModelRaytracing :public NonTransferable
	{
	public:
		ModelRaytracing();

		~ModelRaytracing();

		void Gather(const RenderScene& scene, RenderDevice& device, RenderFrame& frame);

	public:
		NODISCARD std::span<const ModelRaytracingUpdate> Updates()const;

		NODISCARD std::span<const ModelDeformStructuredBuffer> Deforms()const;

		NODISCARD Uint32 DeformVertexCount()const;

		NODISCARD Uint32 Count()const;

		NODISCARD Uint32 SubmissionCount()const;

	private:
		struct Proxy
		{
			Bool used_ = false;

			Uint64 generation_ = 0;

			Bool seen_ = false;

			Bool requested_ = false;

			D3D12_RAYTRACING_GEOMETRY_DESC geometry_{};

			Microsoft::WRL::ComPtr<ID3D12Resource> surfaces_;

			Uint surfaceBufferIndex_ = SC_INVALID;
		};

		struct Slot
		{
			Bool used_ = false;

			Bool seen_ = false;

			Bool sent_ = false;

			ModelRaytracingStructuredBuffer row_;
		};

	private:
		DynamicArray<Proxy> proxies_;

		DynamicArray<Slot> slots_;

		DynamicArray<ModelRaytracingUpdate> updates_;

		DynamicArray<ModelDeformStructuredBuffer> deforms_;

		Uint32 deformVertexCount_ = 0;

		Uint32 count_ = 0;

		Uint32 submissionCount_ = 0;
	};
}

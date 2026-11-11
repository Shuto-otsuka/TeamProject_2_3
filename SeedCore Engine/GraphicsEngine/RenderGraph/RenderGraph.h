#pragma once
#include <GraphicsEngine/RenderGraph/RenderStage.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/Profiler/GpuProfiler.h>

namespace SeedCore
{
	struct RenderGraphResource
	{
		String featureName_;

		String resourceName_;

		RenderResourceDesc desc_;

		RenderResourceScope scope_ = RenderResourceScope::View;

		RenderResourceKind kind_ = RenderResourceKind::Internal;

		RenderResourceLifetime lifetime_ = RenderResourceLifetime::Transient;

		RenderFallback fallback_ = RenderFallback::None;

		Bool shaderResource_ = false;

		Bool unorderedAccess_ = false;

		Bool renderTarget_ = false;

		Bool depthStencil_ = false;

		Bool history_ = false;

		String passThroughName_;

		Uint32 passThroughIndex_ = SC_INVALID;

		void* feature_ = nullptr;
	};

	struct RenderGraphAccess
	{
		String resourceName_;

		RenderAccessType type_ = RenderAccessType::Read;

		Uint32 index_ = SC_INVALID;
	};

	struct RenderGraphStage
	{
		String featureName_;

		String stageName_;

		Uint32 order_ = 0;

		RenderStageKind kind_ = RenderStageKind::Compute;

		RenderQueue queue_ = RenderQueue::Frame;

		RenderViewMask viewMask_ = RenderViewMask::Frame;

		Bool async_ = false;

		DynamicArray<RenderGraphAccess> accesses_;

		void* feature_ = nullptr;

		Bool (*enabled_)(const void* feature, RenderViewKind view) = nullptr;

		void (*execute_)(void* feature, RenderPass& pass) = nullptr;
	};

	struct RenderGraphPass
	{
		Uint32 stage_ = SC_INVALID;

		Uint32 view_ = SC_INVALID;

		Bool signal_ = false;

		Uint32 wait_ = SC_INVALID;

		Uint64 fence_ = 0;

		D3D12_GPU_VIRTUAL_ADDRESS bindings_ = 0;

		DynamicArray<D3D12_RESOURCE_BARRIER> barriers_;

		DynamicArray<D3D12_RESOURCE_BARRIER> handoff_;

		DynamicArray<D3D12_RESOURCE_BARRIER> release_;
	};

	struct RenderGraphAllocation
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

		D3D12_RESOURCE_STATES state_ = D3D12_RESOURCE_STATE_COMMON;

		Uint32 writer_ = SC_INVALID;

		StaticArray<Uint32, 2> readers_ = { SC_INVALID, SC_INVALID };

		RenderResourceShape shape_ = RenderResourceShape::Texture2D;

		DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;

		Uint32 width_ = 0;

		Uint32 height_ = 0;

		Uint32 depth_ = 1;

		Uint32 mipLevels_ = 1;

		Uint32 stride_ = 0;

		Uint32 count_ = 0;

		Bool shaderResource_ = false;

		Bool unorderedAccess_ = false;

		Bool renderTarget_ = false;

		Bool depthStencil_ = false;

		Uint32 shaderResourceView_ = SC_INVALID;

		Uint32 unorderedAccessView_ = SC_INVALID;

		Uint32 renderTargetView_ = SC_INVALID;

		Uint32 depthStencilView_ = SC_INVALID;

		Uint32 depthReadOnlyView_ = SC_INVALID;

		Bool used_ = false;

		Uint32 idleFrames_ = 0;
	};

	struct RenderGraphInstance
	{
		Uint32 resource_ = SC_INVALID;

		Uint32 view_ = SC_INVALID;

		Uint32 width_ = 0;

		Uint32 height_ = 0;

		Uint32 depth_ = 1;

		Uint32 count_ = 0;

		Bool accessed_ = false;

		Bool written_ = false;

		Uint32 target_ = SC_INVALID;

		RenderFallback fallback_ = RenderFallback::None;

		Uint32 allocation_ = SC_INVALID;

		Uint32 history_ = SC_INVALID;

		RenderGraphAllocation* memory_ = nullptr;
	};

	struct RenderGraphImport
	{
		String resourceName_;

		RenderViewKind view_ = RenderViewKind::Game;

		RenderGraphAllocation allocation_;
	};

	class RenderGraph :public NonTransferable
	{
	private:
		friend class RenderPass;

	public:
		explicit RenderGraph(RenderDevice& device);

		~RenderGraph();

		void Initialize(Uint64 capacity);

		void Remove(const void* feature);

		void Sort();

		void Compile(const RenderScene& scene);

		void Execute(const RenderScene& scene);

		NODISCARD RenderUpload& Upload();

		NODISCARD const GpuProfiler& Profiler()const;

	public:
		template<class Feature, class... Stages>
		void Add(Feature& feature)
		{
			String featureName = String(RenderName::FeatureName<Feature>());
			for (const RenderResource* resource : std::span<const RenderResource* const>(Feature::resources_))
			{
				String passThroughName = resource->PassThrough() ? String(resource->PassThrough()->Name()) : String();
				resources_.push_back(RenderGraphResource{ featureName, String(resource->Name()), resource->Desc(), resource->Scope(), resource->Kind(), resource->Lifetime(), resource->Fallback(), false, false, false, false, false, passThroughName, SC_INVALID, &feature });
			}
			Uint32 order = 0;
			([&]()
				{
					RenderGraphStage& stage = stages_.emplace_back();
					stage.featureName_ = featureName;
					stage.stageName_ = String(RenderStageName<Feature, Stages>.View());
					stage.order_ = order;
					++order;
					stage.kind_ = Stages::kind_;
					stage.queue_ = Stages::queue_;
					stage.viewMask_ = Stages::viewMask_;
					stage.async_ = Stages::async_;
					for (const RenderAccess& access : Stages::accesses_)
					{
						stage.accesses_.push_back(RenderGraphAccess{ String(access.Resource().Name()), access.Type() });
					}
					stage.feature_ = &feature;
					stage.enabled_ = [](const void* owner, RenderViewKind view) -> Bool
						{
							using Condition = std::remove_cv_t<decltype(Stages::enableCondition_)>;
							if constexpr (std::same_as<Condition, std::nullptr_t>)
							{
								return true;
							}
							else if constexpr (std::same_as<Condition, Bool Feature::*>)
							{
								return static_cast<const Feature*>(owner)->*Stages::enableCondition_;
							}
							else
							{
								return (static_cast<const Feature*>(owner)->*Stages::enableCondition_)[static_cast<Size>(view)];
							}
						};
					stage.execute_ = [](void* owner, RenderPass& pass)
						{
							Stages::Execute(*static_cast<Feature*>(owner), pass);
						};
				}(), ...);
			dirty_ = true;
		}

	private:
		RenderDevice& device_;

		DynamicArray<RenderGraphResource> resources_;

		DynamicArray<RenderGraphStage> stages_;

		DynamicArray<RenderGraphPass> passes_;

		DynamicArray<RenderGraphInstance> instances_;

		DynamicArray<RenderGraphAllocation> pool_;

		DynamicArray<RenderGraphAllocation> persistent_;

		DynamicArray<RenderGraphAllocation> uploads_;

		DynamicArray<RenderGraphAllocation> imports_;

		DynamicArray<D3D12_RESOURCE_BARRIER> opening_;

		DynamicArray<D3D12_RESOURCE_BARRIER> closing_;

		StaticArray<Uint32, 8> nulls_ = {};

		StaticArray<RenderGraphAllocation, 2> fallbacks_;

		StaticArray<ResourcePtr<StagingBuffer>, 2> bindings_;

		StaticArray<Uint32, RenderViewCount * 2> scenes_ = {};

		StaticArray<D3D12_GPU_VIRTUAL_ADDRESS, RenderViewCount> views_ = {};

		GpuProfiler profiler_;

#if SC_DEBUG
		DynamicArray<std::pair<String, IDxcBlob*>> verified_;
#endif

		RenderUpload upload_;

		Uint32 frame_ = 0;

		Bool dirty_ = true;
	};
}
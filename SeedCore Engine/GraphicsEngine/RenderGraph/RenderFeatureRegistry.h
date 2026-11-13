#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/RenderGraph/RenderStageRules.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderFrame.h>

namespace SeedCore
{
	class RenderFeatureInstance
	{
	public:
		virtual ~RenderFeatureInstance() = default;

		virtual void Initialize(RenderDevice& device, RenderGraph& graph) = 0;

		virtual void Gather(const RenderScene& scene, RenderFrame& frame) = 0;

		virtual void Prepare(const RenderFrame& frame, RenderUpload& upload) = 0;
	};

	template<class Feature, class... Stages>
	class RenderFeatureInstanceOf :public RenderFeatureInstance
	{
	public:
		void Initialize(RenderDevice& device, RenderGraph& graph)override
		{
			feature_.Create(device);
			graph.Add<Feature, Stages...>(feature_);
		}

		void Gather(const RenderScene& scene, RenderFrame& frame)override
		{
			feature_.Gather(scene, frame);
		}

		void Prepare(const RenderFrame& frame, RenderUpload& upload)override
		{
			feature_.Prepare(frame, upload);
		}

	private:
		Feature feature_;
	};

	struct RenderFeatureEntry
	{
		std::string_view featureName_;

		RenderFeatureSet set_ = RenderFeatureSet::Scene;

		ResourcePtr<RenderFeatureInstance>(*create_)() = nullptr;
	};

	class RenderFeatureRegistry
	{
	public:
		template<class Feature, class... Slots>
		static Bool Register()
		{
			SC_STATIC_ASSERT(RenderStageRules::HasSlotOrder<Slots...>(), "Stage は Update / Deform / Submit / Build / Culling / Draw / Trace / Filter / Resolve / Compose のどれかにして、この順に 1 回ずつ並べること");
			return Expand<Feature>(typename RenderJoin<typename RenderExpand<Slots>::Type...>::Type());
		}

		NODISCARD static std::span<const RenderFeatureEntry> Entries();

	private:
		template<class Feature, class... Stages>
		static Bool Expand(RenderSequence<Stages...>)
		{
			SC_STATIC_ASSERT(RenderStageRules::IsRenderFeature<Feature>, "Feature は RenderFeature<自分> を継承し、Create / Gather / Prepare を public にすること");
			SC_STATIC_ASSERT((RenderStageRules::IsRenderStage<Stages, Feature> && ...), "Stage は RenderStage を継承し、queue_ / accesses_ / Execute を持つこと");
			SC_STATIC_ASSERT(RenderStageRules::HasOwnedResources<Feature>(), "SC_RENDER_RESOURCES は自分の Feature の中に書くこと");
			SC_STATIC_ASSERT(RenderStageRules::HasUniqueStageAccesses<Stages...>(), "同じ Stage が同じリソースを2回宣言している");
			SC_STATIC_ASSERT((RenderStageRules::HasOwnedContractWrites<Feature, Stages...>()), "他の Feature の契約のリソースに書いている");
			SC_STATIC_ASSERT(RenderStageRules::HasAccumulationAppends<Stages...>(), "Append できるのは蓄積（SC_RENDER_ACCUMULATION）のリソースだけ");
			SC_STATIC_ASSERT((RenderStageRules::HasWrittenContracts<Feature, Stages...>()), "自分の契約のリソースを書く Stage が無い");
			entries_.push_back(RenderFeatureEntry{ RenderName::FeatureName<Feature>(), Feature::set_, []() -> ResourcePtr<RenderFeatureInstance> { return MakePtr<RenderFeatureInstanceOf<Feature, Stages...>>(); } });
			return true;
		}

	private:
		static inline DynamicArray<RenderFeatureEntry> entries_;
	};
}

#define SC_RENDER_STAGE_ARGUMENT(Feature, Stage) , Feature::Stage

#define REGISTER_RENDER_FEATURE(Feature, ...) \
	static const ::SeedCore::Bool Feature##_renderFeatureRegistered = ::SeedCore::RenderFeatureRegistry::Register<Feature SC_RENDER_FOR_EACH(SC_RENDER_STAGE_ARGUMENT, Feature, __VA_ARGS__)>()

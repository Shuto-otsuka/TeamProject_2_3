#pragma once
#include <GraphicsEngine/RenderGraph/RenderResourceDeclaration.h>
#include <GraphicsEngine/RenderGraph/RenderStage.h>

namespace SeedCore
{
	struct RenderScene;
	struct RenderFrame;

	class RenderDevice;
	class RenderGraph;
	class RenderUpload;

	class RenderFeatureAccess
	{
	public:
		template<class Feature, class... Stages>
		static consteval Bool IsRenderStageList(RenderStageList<Stages...>)
		{
			if constexpr ((RenderStageAccess::IsRenderStage<Stages, Feature> && ...))
			{
				return ((Stages::name_.FeatureName() == Feature::name_.Name()) && ...);
			}
			else
			{
				return false;
			}
		}

		template<class Feature>
		SC_CONSTEX Bool IsRenderFeature = std::is_default_constructible_v<Feature> && requires(Feature & feature, RenderDevice & device, RenderGraph & graph, const RenderScene & scene, RenderFrame & frame, RenderUpload & upload)
		{
			{ feature.Create(device) } -> std::same_as<void>;
			{ feature.Register(graph) } -> std::same_as<void>;
			{ feature.Gather(scene, frame) } -> std::same_as<void>;
			{ feature.Prepare(std::as_const(frame), upload) } -> std::same_as<void>;
				requires std::same_as<std::remove_cv_t<decltype(Feature::name_)>, RenderFeatureName>;
			std::span<const RenderResourceDeclaration>(Feature::resources_);
			typename Feature::Stages;
				requires IsRenderStageList<Feature>(typename Feature::Stages{});
		};

		template<class Feature>
		static consteval RenderFeatureName NameOf()
		{
			return Feature::name_;
		}

		template<class Feature>
		static consteval typename Feature::Stages StagesOf()
		{
			return typename Feature::Stages{};
		}

		template<class Feature>
		static consteval std::span<const RenderResourceDeclaration> ResourcesOf()
		{
			return Feature::resources_;
		}

		template<class Feature>
		static void Create(Feature& feature, RenderDevice& device)
		{
			static_assert(IsRenderFeature<Feature>, "Feature は Create / Register / Gather / Prepare と name_ / Stages / resources_ を持つこと");
			feature.Create(device);
		}

		template<class Feature>
		static void Register(Feature& feature, RenderGraph& graph)
		{
			static_assert(IsRenderFeature<Feature>, "Feature は Create / Register / Gather / Prepare と name_ / Stages / resources_ を持つこと");
			feature.Register(graph);
		}

		template<class Feature>
		static void Gather(Feature& feature, const RenderScene& scene, RenderFrame& frame)
		{
			static_assert(IsRenderFeature<Feature>, "Feature は Create / Register / Gather / Prepare と name_ / Stages / resources_ を持つこと");
			feature.Gather(scene, frame);
		}

		template<class Feature>
		static void Prepare(Feature& feature, const RenderFrame& frame, RenderUpload& upload)
		{
			static_assert(IsRenderFeature<Feature>, "Feature は Create / Register / Gather / Prepare と name_ / Stages / resources_ を持つこと");
			feature.Prepare(frame, upload);
		}
	};
}
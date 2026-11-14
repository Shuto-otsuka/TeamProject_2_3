#pragma once
#include <GraphicsEngine/RenderGraph/RenderStage.h>

namespace SeedCore
{
	struct RenderScene;

	class RenderFrame;
	class RenderDevice;
	class RenderUpload;

	class RenderStageRules
	{
	public:
		template<class Feature>
		SC_CONSTEX Bool IsRenderFeature = std::is_default_constructible_v<Feature> && requires(Feature & feature, RenderDevice & device, const RenderScene & scene, RenderFrame & frame, RenderUpload & upload)
		{
			{ feature.Create(device) } -> std::same_as<void>;
			{ feature.Gather(scene, frame) } -> std::same_as<void>;
			{ feature.Prepare(std::as_const(frame), upload) } -> std::same_as<void>;
			std::span<const RenderResource* const>(Feature::resources_);
		};

		template<class Stage, class Feature>
		SC_CONSTEX Bool IsRenderStage = requires(Feature & feature, RenderPass & pass)
		{
			{ Stage::Execute(feature, pass) } -> std::same_as<void>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::kind_)>, RenderStageKind>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::queue_)>, RenderQueue>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::viewMask_)>, RenderViewMask>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::async_)>, Bool>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, std::nullptr_t> || std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, Bool Feature::*> || std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, Bool(Feature::*)[RenderViewCount]>;
			requires std::is_array_v<std::remove_reference_t<decltype(Stage::accesses_)>> && std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<decltype(Stage::accesses_)>>>, RenderAccess>;
		};

		SC_CONSTEX std::string_view slots_[] = { "Update", "Deform", "Submit", "Build", "Culling", "Draw", "Trace", "Filter", "Resolve", "Compose" };

		template<class... Slots>
		static consteval Bool HasSlotOrder()
		{
			Bool ordered = true;
			Size previous = 0;
			Bool first = true;
			([&]()
				{
					Size found = std::size(slots_);
					for (Size index = 0;index < std::size(slots_);++index)
					{
						if (slots_[index] == RenderName::StageName<Slots>())
						{
							found = index;
						}
					}
					if (found == std::size(slots_) || (!first && found <= previous))
					{
						ordered = false;
					}
					previous = found;
					first = false;
				}(), ...);
			return ordered;
		}

		template<class Feature>
		static consteval Bool HasOwnedResources()
		{
			for (const RenderResource* resource : std::span<const RenderResource* const>(Feature::resources_))
			{
				if (resource->FeatureName() != RenderName::FeatureName<Feature>())
				{
					return false;
				}
			}
			return true;
		}

		template<class... Stages>
		static consteval Bool HasUniqueStageAccesses()
		{
			Bool unique = true;
			([&]()
				{
					for (Size index = 0;index < std::size(Stages::accesses_);++index)
					{
						for (Size otherIndex = index + 1;otherIndex < std::size(Stages::accesses_);++otherIndex)
						{
							if (Stages::accesses_[index].Resource() == Stages::accesses_[otherIndex].Resource() && (Stages::accesses_[index].Type() == RenderAccessType::History) == (Stages::accesses_[otherIndex].Type() == RenderAccessType::History))
							{
								unique = false;
							}
						}
					}
				}(), ...);
			return unique;
		}

		template<class Feature, class... Stages>
		static consteval Bool HasOwnedContractWrites()
		{
			Bool owned = true;
			([&]()
				{
					for (const RenderAccess& access : Stages::accesses_)
					{
						Bool write = access.Type() == RenderAccessType::Write || access.Type() == RenderAccessType::Target || access.Type() == RenderAccessType::DepthWrite;
						if (write && access.Resource().Kind() == RenderResourceKind::Contract && access.Resource().FeatureName() != RenderName::FeatureName<Feature>())
						{
							owned = false;
						}
					}
				}(), ...);
			return owned;
		}

		template<class... Stages>
		static consteval Bool HasAccumulationAppends()
		{
			Bool accumulation = true;
			([&]()
				{
					for (const RenderAccess& access : Stages::accesses_)
					{
						if (access.Type() == RenderAccessType::Append && access.Resource().Kind() != RenderResourceKind::Accumulation)
						{
							accumulation = false;
						}
					}
				}(), ...);
			return accumulation;
		}

		template<class Feature, class... Stages>
		static consteval Bool HasWrittenContracts()
		{
			for (const RenderResource* resource : std::span<const RenderResource* const>(Feature::resources_))
			{
				if (resource->Kind() != RenderResourceKind::Contract || resource->Fallback() != RenderFallback::None || resource->Lifetime() == RenderResourceLifetime::Imported || resource->Lifetime() == RenderResourceLifetime::Upload || resource->PassThrough())
				{
					continue;
				}
				Bool written = false;
				([&]()
					{
						for (const RenderAccess& access : Stages::accesses_)
						{
							Bool write = access.Type() == RenderAccessType::Write || access.Type() == RenderAccessType::Target || access.Type() == RenderAccessType::DepthWrite;
							if (write && access.Resource() == *resource)
							{
								written = true;
							}
						}
					}(), ...);
				if (!written)
				{
					return false;
				}
			}
			return true;
		}
	};
}

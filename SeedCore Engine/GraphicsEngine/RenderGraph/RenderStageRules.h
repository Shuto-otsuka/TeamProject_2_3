#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeatureAccess.h>

namespace SeedCore
{
	class RenderStageRules
	{
	public:
		template<class... Features>
		static consteval Bool HasUniqueResourceNames()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			for (Size index = 0;index < resources.size();++index)
			{
				for (Size otherIndex = index + 1;otherIndex < resources.size();++otherIndex)
				{
					if (resources[index].resourceName_ == resources[otherIndex].resourceName_)
					{
						return false;
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasDeclaredAccesses()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			StaticArray stages = StageRecordsOf<Features...>();
			for (const StageRecord& stage : stages)
			{
				for (const RenderAccess& access : stage.accesses_)
				{
					Bool declared = false;
					for (const ResourceRecord& resource : resources)
					{
						if (resource.resourceName_ == access.Resource().Name())
						{
							declared = true;
						}
					}
					if (!declared)
					{
						return false;
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasDeclaredPassThroughs()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			for (const ResourceRecord& resource : resources)
			{
				if (resource.passThroughName_.empty())
				{
					continue;
				}
				Bool declared = false;
				for (const ResourceRecord& other : resources)
				{
					if (other.resourceName_ == resource.passThroughName_)
					{
						declared = true;
					}
				}
				if (!declared)
				{
					return false;
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasOwnedContractWrites()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			StaticArray stages = StageRecordsOf<Features...>();
			for (const StageRecord& stage : stages)
			{
				for (const RenderAccess& access : stage.accesses_)
				{
					if (access.Type() != RenderAccessType::Write && access.Type() != RenderAccessType::Target && access.Type() != RenderAccessType::DepthWrite)
					{
						continue;
					}
					for (const ResourceRecord& resource : resources)
					{
						if (resource.resourceName_ == access.Resource().Name() && resource.kind_ == RenderResourceKind::Contract && resource.featureName_ != stage.featureName_)
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasOwnedInternalAccesses()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			StaticArray stages = StageRecordsOf<Features...>();
			for (const StageRecord& stage : stages)
			{
				for (const RenderAccess& access : stage.accesses_)
				{
					for (const ResourceRecord& resource : resources)
					{
						if (resource.resourceName_ != access.Resource().Name() || resource.featureName_ == stage.featureName_)
						{
							continue;
						}
						if (resource.kind_ == RenderResourceKind::Internal)
						{
							return false;
						}
						if (resource.kind_ == RenderResourceKind::Debug && stage.featureName_ != "DebugView")
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasReadableContracts()
		{
			StaticArray resources = ResourceRecordsOf<Features...>();
			StaticArray stages = StageRecordsOf<Features...>();
			for (const ResourceRecord& resource : resources)
			{
				if (resource.kind_ != RenderResourceKind::Contract || resource.fallback_ != RenderFallback::None || resource.lifetime_ == RenderResourceLifetime::Imported || resource.lifetime_ == RenderResourceLifetime::Upload || !resource.passThroughName_.empty())
				{
					continue;
				}
				Bool read = false;
				Bool written = false;
				for (const StageRecord& stage : stages)
				{
					for (const RenderAccess& access : stage.accesses_)
					{
						if (access.Resource().Name() != resource.resourceName_)
						{
							continue;
						}
						if (access.Type() == RenderAccessType::Read || access.Type() == RenderAccessType::History || access.Type() == RenderAccessType::Constant || access.Type() == RenderAccessType::DepthTest)
						{
							read = true;
						}
						if (access.Type() == RenderAccessType::Write || access.Type() == RenderAccessType::Target || access.Type() == RenderAccessType::DepthWrite)
						{
							written = true;
						}
					}
				}
				if (read && !written)
				{
					return false;
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasUniqueStageAccesses()
		{
			StaticArray stages = StageRecordsOf<Features...>();
			for (const StageRecord& stage : stages)
			{
				for (Size index = 0;index < stage.accesses_.size();++index)
				{
					for (Size otherIndex = index + 1;otherIndex < stage.accesses_.size();++otherIndex)
					{
						if (stage.accesses_[index].Resource() == stage.accesses_[otherIndex].Resource())
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasUniqueFeatureNames()
		{
			StaticArray<std::string_view, sizeof...(Features)> featureNames = { RenderFeatureAccess::NameOf<Features>().Name()... };
			for (Size index = 0;index < featureNames.size();++index)
			{
				for (Size otherIndex = index + 1;otherIndex < featureNames.size();++otherIndex)
				{
					if (featureNames[index] == featureNames[otherIndex])
					{
						return false;
					}
				}
			}
			return true;
		}

		template<class... Features>
		static consteval Bool HasUniqueStageNames()
		{
			StaticArray stages = StageRecordsOf<Features...>();
			for (Size index = 0;index < stages.size();++index)
			{
				for (Size otherIndex = index + 1;otherIndex < stages.size();++otherIndex)
				{
					if (stages[index].stageName_ == stages[otherIndex].stageName_)
					{
						return false;
					}
				}
			}
			return true;
		}

	private:
		struct ResourceRecord
		{
			std::string_view featureName_;

			std::string_view resourceName_;

			RenderResourceKind kind_ = RenderResourceKind::Internal;

			RenderResourceLifetime lifetime_ = RenderResourceLifetime::Transient;

			RenderFallback fallback_ = RenderFallback::None;

			std::string_view passThroughName_;
		};

		struct StageRecord
		{
			std::string_view featureName_;

			std::string_view stageName_;

			std::span<const RenderAccess> accesses_;
		};

	private:
		template<class... Features>
		static consteval StaticArray<ResourceRecord, (RenderFeatureAccess::ResourcesOf<Features>().size() + ... + 0)> ResourceRecordsOf()
		{
			StaticArray<std::string_view, sizeof...(Features)> featureNames = { RenderFeatureAccess::NameOf<Features>().Name()... };
			StaticArray<std::span<const RenderResourceDeclaration>, sizeof...(Features)> declarationLists = { RenderFeatureAccess::ResourcesOf<Features>()... };
			StaticArray<ResourceRecord, (RenderFeatureAccess::ResourcesOf<Features>().size() + ... + 0)> records{};
			Size count = 0;
			for (Size featureIndex = 0;featureIndex < featureNames.size();++featureIndex)
			{
				for (const RenderResourceDeclaration& declaration : declarationLists[featureIndex])
				{
					records[count] = ResourceRecord{ featureNames[featureIndex], declaration.resource_.Name(), declaration.kind_, declaration.lifetime_, declaration.fallback_, declaration.passThrough_.Name() };
					++count;
				}
			}
			return records;
		}

		template<class... Features>
		static consteval StaticArray<StageRecord, (decltype(RenderFeatureAccess::StagesOf<Features>())::names_.size() + ... + 0)> StageRecordsOf()
		{
			StaticArray<std::string_view, sizeof...(Features)> featureNames = { RenderFeatureAccess::NameOf<Features>().Name()... };
			StaticArray<std::span<const std::string_view>, sizeof...(Features)> stageNameLists = { decltype(RenderFeatureAccess::StagesOf<Features>())::names_... };
			StaticArray<std::span<const std::span<const RenderAccess>>, sizeof...(Features)> accessLists = { decltype(RenderFeatureAccess::StagesOf<Features>())::accesses_... };
			StaticArray<StageRecord, (decltype(RenderFeatureAccess::StagesOf<Features>())::names_.size() + ... + 0)> records{};
			Size count = 0;
			for (Size featureIndex = 0;featureIndex < featureNames.size();++featureIndex)
			{
				for (Size stageIndex = 0;stageIndex < stageNameLists[featureIndex].size();++stageIndex)
				{
					records[count] = StageRecord{ featureNames[featureIndex], stageNameLists[featureIndex][stageIndex], accessLists[featureIndex][stageIndex] };
					++count;
				}
			}
			return records;
		}
	};
}
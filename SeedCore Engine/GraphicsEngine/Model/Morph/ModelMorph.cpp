#include <GraphicsEngine/Model/Morph/ModelMorph.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Animation/Animation.h>
#include <GraphicsEngine/Model/Animation/AnimationResource.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	ModelMorph::ModelMorph()
	{
		/// No Code
	}

	ModelMorph::~ModelMorph() = default;

	void ModelMorph::Begin()
	{
		current_.clear();
		previous_.clear();
	}

	Uint32 ModelMorph::Write(Uint32 slot, Bool historyValid, Actor& actor, Crister& crister, const ModelMeshRecord& record, AnimationResource& animationResource)
	{
		if (slots_.size() <= slot)
		{
			slots_.resize(static_cast<Size>(slot) + 1);
		}
		if (record.morphWeightCount_ == 0 || current_.size() + record.morphWeightCount_ > capacity_)
		{
			slots_[slot].clear();
			return SC_INVALID;
		}

		Animator* animator = actor.GetComponent<Animator>();
		if (!animator)
		{
			slots_[slot].clear();
			return SC_INVALID;
		}
		Int stateIndex = animator->CurrentStateIndex();
		if (stateIndex < 0 || static_cast<Size>(stateIndex) >= animator->states_.size())
		{
			slots_[slot].clear();
			return SC_INVALID;
		}
		const AnimationState& state = animator->states_[stateIndex];
		Uint32 animationAssetID = static_cast<Uint32>(state.animationID_);
		Handle<Animation> animationHandle = animationAssetID != 0 ? animationResource.GetHandle(animationAssetID) : Handle<Animation>{};
		Animation* animation = animationHandle.empty() ? nullptr : animationResource.Resolve(animationHandle);
		if (!animation)
		{
			slots_[slot].clear();
			return SC_INVALID;
		}

		Float duration = animation->Duration();
		Float sampleTime = animator->CurrentTime();
		if (duration > 0.0f)
		{
			sampleTime = state.loop_ ? std::fmod(sampleTime, duration) : Min(sampleTime, duration);
		}
		animator->UpdateCurrentClipDuration(duration);
		std::unordered_map<Int, DynamicArray<Float>> nodeWeights;
		animation->SampleMorphWeights(sampleTime, nodeWeights);

		Uint32 base = static_cast<Uint32>(current_.size());
		current_.resize(current_.size() + record.morphWeightCount_, 0.0f);
		const DynamicArray<SubMesh>& subMeshes = crister.SubMeshes();
		for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
		{
			if (subMeshes[subMeshIndex].morphs_.empty())
			{
				continue;
			}
			auto found = nodeWeights.find(record.morphNodes_[subMeshIndex]);
			if (found == nodeWeights.end())
			{
				continue;
			}
			Size count = Min(found->second.size(), subMeshes[subMeshIndex].morphs_.size());
			std::copy_n(found->second.begin(), count, current_.begin() + base + record.morphWeightOffsets_[subMeshIndex]);
		}

		DynamicArray<Float>& history = slots_[slot];
		if (historyValid && history.size() == record.morphWeightCount_)
		{
			previous_.insert(previous_.end(), history.begin(), history.end());
		}
		else
		{
			previous_.insert(previous_.end(), current_.begin() + base, current_.end());
		}
		history.assign(current_.begin() + base, current_.end());
		return base;
	}

	void ModelMorph::Release(Uint32 slot)
	{
		if (slot < slots_.size())
		{
			slots_[slot].clear();
		}
	}

	void ModelMorph::End()
	{
		weights_.clear();
		weights_.insert(weights_.end(), current_.begin(), current_.end());
		weights_.insert(weights_.end(), previous_.begin(), previous_.end());
	}

	std::span<const Float> ModelMorph::Weights()const
	{
		return weights_;
	}
}

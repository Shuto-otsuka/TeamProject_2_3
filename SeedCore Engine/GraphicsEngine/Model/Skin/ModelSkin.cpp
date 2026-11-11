#include <GraphicsEngine/Model/Skin/ModelSkin.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	ModelSkin::ModelSkin()
	{
		/// No Code
	}

	ModelSkin::~ModelSkin() = default;

	void ModelSkin::Begin()
	{
		staticBases_.clear();
		current_.clear();
		previous_.clear();
	}

	Bool ModelSkin::Advance(Uint32 slot, Bool reset, Animator* animator)
	{
		if (slots_.size() <= slot)
		{
			slots_.resize(static_cast<Size>(slot) + 1);
		}
		Slot& entry = slots_[slot];
		if (reset)
		{
			entry = Slot{};
		}
		if (!animator)
		{
			entry.animatorValid_ = false;
			return true;
		}

		Bool historyValid = true;
		Int state = animator->CurrentStateIndex();
		Float time = animator->CurrentTime();
		if (entry.animatorValid_)
		{
			Bool poseJumped = entry.animatorState_ != state || entry.animatorTime_ > time;
			Bool crossFading = animator->Blending() && animator->blendDuration_ > 0.0f;
			historyValid = !poseJumped || crossFading;
		}
		entry.animatorValid_ = true;
		entry.animatorState_ = state;
		entry.animatorTime_ = time;
		return historyValid;
	}

	Uint32 ModelSkin::Write(Uint32 slot, Bool historyValid, Actor& actor, Crister& crister, const ModelMeshRecord& record)
	{
		if (record.boneCount_ == 0)
		{
			return SC_INVALID;
		}
		if (current_.size() + record.boneCount_ > capacity_)
		{
			return SC_INVALID;
		}

		const DynamicArray<Skin>& skins = crister.Skins();
		const Skeleton* skeleton = actor.GetComponent<Skeleton>();
		Bool posed = skeleton && skeleton->Animated() && skeleton->GlobalTransforms().size() == crister.Nodes().size();
		if (!posed)
		{
			auto found = staticBases_.find(&crister);
			if (found != staticBases_.end())
			{
				return found->second;
			}
		}

		Uint32 base = static_cast<Uint32>(current_.size());
		for (const Skin& skin : skins)
		{
			for (Size joint = 0;joint < skin.joints_.size();++joint)
			{
				const Matrix& globalTransform = posed ? skeleton->GlobalTransforms()[skin.joints_[joint]] : crister.Nodes()[skin.joints_[joint]].globalTransform_;
				current_.push_back(joint < skin.inverseBindMatrices_.size() ? skin.inverseBindMatrices_[joint] * globalTransform : globalTransform);
			}
		}

		if (!posed)
		{
			previous_.insert(previous_.end(), current_.begin() + base, current_.end());
			staticBases_[&crister] = base;
			return base;
		}

		Slot& entry = slots_[slot];
		if (historyValid && entry.palette_.size() == record.boneCount_)
		{
			previous_.insert(previous_.end(), entry.palette_.begin(), entry.palette_.end());
		}
		else
		{
			previous_.insert(previous_.end(), current_.begin() + base, current_.end());
		}
		entry.palette_.assign(current_.begin() + base, current_.end());
		return base;
	}

	void ModelSkin::Release(Uint32 slot)
	{
		if (slot < slots_.size())
		{
			slots_[slot] = Slot{};
		}
	}

	void ModelSkin::End()
	{
		bones_.clear();
		bones_.insert(bones_.end(), current_.begin(), current_.end());
		bones_.insert(bones_.end(), previous_.begin(), previous_.end());
	}

	std::span<const Matrix> ModelSkin::Bones()const
	{
		return bones_;
	}
}

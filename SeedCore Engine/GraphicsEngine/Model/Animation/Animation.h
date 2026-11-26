#pragma once

namespace SeedCore
{
	struct AnimationChannel
	{
		Int sampler_ = -1;
		Int targetNode_ = -1;
		std::string targetPath_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("sampler", sampler_);
			archive.Field("target_node", targetNode_);
			archive.Field("target_path", targetPath_);
		}
	};

	struct AnimationSampler
	{
		Int input_ = -1;
		Int output_ = -1;
		std::string interpolation_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("input", input_);
			archive.Field("output", output_);
			archive.Field("interpolation", interpolation_);
		}
	};

	struct AnimationSpeedKeyframe
	{
		Float time_ = 0.0f;
		Float value_ = 0.0f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("time", time_);
			archive.Field("value", value_);
		}
	};

	struct AnimationNotifyEvent
	{
		Float time_ = 0.0f;
		std::string label_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("time", time_);
			archive.Field("label", label_);
		}
	};

	class SEEDCORE_API Animation :public NonCopyable
	{
	private:
		friend class AnimationLoader;

		std::string name_;
		Float duration_ = 0.0f;

		DynamicArray<AnimationChannel> channels_;
		DynamicArray<AnimationSampler> samplers_;

		std::unordered_map<Int, DynamicArray<Float>> timelines_;
		std::unordered_map<Int, DynamicArray<Vector3>> scales_;
		std::unordered_map<Int, DynamicArray<Quaternion>> rotations_;
		std::unordered_map<Int, DynamicArray<Vector3>> translations_;

		std::unordered_map<Int, DynamicArray<DynamicArray<Float>>> weights_;

		DynamicArray<AnimationSpeedKeyframe> speedCurve_;
		DynamicArray<AnimationNotifyEvent> notifyEvents_;

	public:
		Animation() = default;
		~Animation() = default;

		Animation(Animation&&)noexcept = default;
		Animation& operator=(Animation&&)noexcept = default;

		Float Duration()const;

		void SamplePose(Float time, std::unordered_map<Int, Vector3>& outTranslations, std::unordered_map<Int, Quaternion>& outRotations, std::unordered_map<Int, Vector3>& outScales)const;

		void SampleMorphWeights(Float time, std::unordered_map<Int, DynamicArray<Float>>& outWeights)const;

		NODISCARD DynamicArray<AnimationSpeedKeyframe>& SpeedCurve();
		NODISCARD const DynamicArray<AnimationSpeedKeyframe>& SpeedCurve()const;

		NODISCARD DynamicArray<AnimationNotifyEvent>& NotifyEvents();
		NODISCARD const DynamicArray<AnimationNotifyEvent>& NotifyEvents()const;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("duration", duration_);
			archive.Field("channels", channels_);
			archive.Field("samplers", samplers_);
			archive.Field("timelines", timelines_);
			archive.Field("scales", scales_);
			archive.Field("rotations", rotations_);
			archive.Field("translations", translations_);
			archive.Field("weights", weights_);
			archive.Field("speed_curve", speedCurve_);
			archive.Field("notify_events", notifyEvents_);
		}
	};
}

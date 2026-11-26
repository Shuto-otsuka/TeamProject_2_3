#pragma once

namespace SeedCore
{
	class Crister;
	class Animator;
	struct IKTarget;
	struct JointConstraint;

	class FullBodyIK
	{
	public:
		static void Apply(const Crister& crister, const Animator& animator, DynamicArray<Matrix>& poseGlobalTransforms);

	private:
		static void SolveSingle(const Crister& crister, Int endNodeIndex, const IKTarget& target, const std::unordered_map<std::string, JointConstraint>& jointConstraints, DynamicArray<Matrix>& poseGlobalTransforms);

		static void SolveGroup(const Crister& crister, const DynamicArray<std::pair<Int, const IKTarget*>>& groupTargets, const std::unordered_map<std::string, JointConstraint>& jointConstraints, DynamicArray<Matrix>& poseGlobalTransforms);
	};
}

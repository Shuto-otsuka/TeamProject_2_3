#pragma once

namespace SeedCore
{
	class AnimalCharacterModel;

	class SEEDCORE_API AnimalCharacterEvaluator :public NonCopyable
	{
	public:
		AnimalCharacterEvaluator() = default;
		~AnimalCharacterEvaluator() = default;

		void SetModel(const AnimalCharacterModel& model);

		void SetAxisWeight(Uint32 axisIndex, Float weight);

		void ResetAxisWeights();

		NODISCARD Float AxisWeight(Uint32 axisIndex)const;

		void Evaluate();

		NODISCARD std::span<const Vector3> Positions()const;

		NODISCARD std::span<const Vector3> Normals()const;

		NODISCARD std::span<const Vector3> JointHeads()const;

	private:
		SC_CONSTEX Float featureBudget_ = 2.5f;

		const AnimalCharacterModel* model_ = nullptr;

		DynamicArray<Float> axisWeights_;
		DynamicArray<Vector3> positions_;
		DynamicArray<Vector3> normals_;
		DynamicArray<Vector3> jointHeads_;
	};
}

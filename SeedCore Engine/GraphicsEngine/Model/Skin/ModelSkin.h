#pragma once
#include <GraphicsEngine/Model/ModelMesh.h>

namespace SeedCore
{
	class Actor;
	class Animator;

	class ModelSkin :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 131072;

	public:
		ModelSkin();

		~ModelSkin();

		void Begin();

		NODISCARD Bool Advance(Uint32 slot, Bool reset, Animator* animator);

		NODISCARD Uint32 Write(Uint32 slot, Bool historyValid, Actor& actor, Crister& crister, const ModelMeshRecord& record);

		void Release(Uint32 slot);

		void End();

	public:
		NODISCARD std::span<const Matrix> Bones()const;

	private:
		struct Slot
		{
			Bool animatorValid_ = false;

			Int animatorState_ = -1;

			Float animatorTime_ = 0.0f;

			DynamicArray<Matrix> palette_;
		};

	private:
		DynamicArray<Slot> slots_;

		std::unordered_map<const Crister*, Uint32> staticBases_;

		DynamicArray<Matrix> current_;

		DynamicArray<Matrix> previous_;

		DynamicArray<Matrix> bones_;
	};
}

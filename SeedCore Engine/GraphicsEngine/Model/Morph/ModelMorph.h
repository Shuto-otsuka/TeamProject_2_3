#pragma once
#include <GraphicsEngine/Model/ModelMesh.h>

namespace SeedCore
{
	class Actor;
	class AnimationResource;

	class ModelMorph :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 131072;

	public:
		ModelMorph();

		~ModelMorph();

		void Begin();

		NODISCARD Uint32 Write(Uint32 slot, Bool historyValid, Actor& actor, Crister& crister, const ModelMeshRecord& record, AnimationResource& animationResource);

		void Release(Uint32 slot);

		void End();

	public:
		NODISCARD std::span<const Float> Weights()const;

	private:
		DynamicArray<DynamicArray<Float>> slots_;

		DynamicArray<Float> current_;

		DynamicArray<Float> previous_;

		DynamicArray<Float> weights_;
	};
}

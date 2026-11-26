#pragma once

namespace SeedCore
{
	class World;

	class TransformSystem
	{
	public:
		void Execute(World& world);

	private:
		void UpdateActor(class Actor actor, const Matrix& parentMatrix, World& world);
	};
}

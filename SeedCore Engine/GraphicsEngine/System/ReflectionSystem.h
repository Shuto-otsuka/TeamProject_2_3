#pragma once
#include <GraphicsEngine/Renderer/ReflectionRenderer.h>

namespace SeedCore
{
	class World;

	class ReflectionSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const RaytracingReflectionConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		RaytracingReflectionConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

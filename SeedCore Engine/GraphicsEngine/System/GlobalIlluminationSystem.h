#pragma once
#include <GraphicsEngine/Renderer/GlobalIlluminationRenderer.h>

namespace SeedCore
{
	class World;

	class GlobalIlluminationSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const RaytracingGlobalIlluminationConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		RaytracingGlobalIlluminationConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

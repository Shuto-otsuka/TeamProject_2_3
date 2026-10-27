#pragma once
#include <GraphicsEngine/Renderer/AmbientOcclusionRenderer.h>

namespace SeedCore
{
	class World;

	class AmbientOcclusionSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const RaytracingAmbientOcclusionConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		RaytracingAmbientOcclusionConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

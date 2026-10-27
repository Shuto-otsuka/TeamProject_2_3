#pragma once
#include <GraphicsEngine/Renderer/VolumetricStarRenderer.h>

namespace SeedCore
{
	class World;

	class VolumetricStarSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const VolumetricStarRayConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		VolumetricStarRayConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

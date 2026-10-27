#pragma once
#include <GraphicsEngine/Renderer/VolumetricCloudScapesRenderer.h>

namespace SeedCore
{
	class World;
	class CelestialSystem;

	class VolumetricCloudScapesSystem
	{
	public:
		void Execute(World& world, const CelestialSystem& celestial);

		NODISCARD const VolumetricCloudScapesRayConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		VolumetricCloudScapesRayConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

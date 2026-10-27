#pragma once
#include <GraphicsEngine/Renderer/VolumetricLightRenderer.h>

namespace SeedCore
{
	class World;

	class VolumetricLightSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const VolumetricLightRayConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		VolumetricLightRayConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

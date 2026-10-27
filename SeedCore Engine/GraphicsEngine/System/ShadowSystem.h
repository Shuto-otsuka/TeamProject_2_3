#pragma once
#include <GraphicsEngine/Renderer/ShadowRenderer.h>

namespace SeedCore
{
	class World;

	class ShadowSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const RaytracingShadowConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		RaytracingShadowConstantBuffer constantBuffer_;

		Bool enabled_ = true;
	};
}

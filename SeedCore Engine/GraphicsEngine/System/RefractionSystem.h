#pragma once
#include <GraphicsEngine/Renderer/RefractionRenderer.h>

namespace SeedCore
{
	class World;

	class RefractionSystem
	{
	public:
		void Execute(World& world);

		NODISCARD const RefractionRayConstantBuffer& ConstantBuffer()const;

		NODISCARD Bool Enabled()const;

	private:
		RefractionRayConstantBuffer constantBuffer_;

		Bool enabled_ = false;
	};
}

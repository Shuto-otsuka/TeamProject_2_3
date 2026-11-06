#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct RenderGraphImport;

	class World;
	class GraphicsResourceSystem;

	struct RenderScene
	{
		World* world_ = nullptr;

		GraphicsResourceSystem* resource_ = nullptr;

		std::span<const RenderView> views_;

		std::span<const RenderGraphImport> imports_;
	};
}
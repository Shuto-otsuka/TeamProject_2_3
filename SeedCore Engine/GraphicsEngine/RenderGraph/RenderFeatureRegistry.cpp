#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	std::span<const RenderFeatureEntry> RenderFeatureRegistry::Entries()
	{
		return entries_;
	}
}

#pragma once
#include <GraphicsEngine/RenderGraph/RenderGraph.h>

namespace SeedCore
{
	class RenderDevice;

	class RenderGraphMemory
	{
	public:
		NODISCARD static RenderGraphAllocation Create(RenderDevice& device, const RenderGraphResource& resource, const RenderGraphInstance& instance);

		NODISCARD static RenderGraphAllocation Create(RenderDevice& device, const RenderGraphResource& resource, const StagingBuffer& buffer, const RenderUploadEntry& entry);

		static void Release(RenderDevice& device, RenderGraphAllocation& allocation);

		NODISCARD static Bool Matches(const RenderGraphAllocation& allocation, const RenderGraphResource& resource, const RenderGraphInstance& instance);
	};
}
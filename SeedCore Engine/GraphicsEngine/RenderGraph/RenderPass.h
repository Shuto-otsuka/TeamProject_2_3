#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	struct RenderView;

	class RenderGraph;
	class RenderDevice;
	class D3D12CommandList;

	class RenderPass :public NonTransferable
	{
	public:
		RenderPass(RenderGraph& graph, D3D12CommandList& list, Uint32 pass, const RenderView* view);

		NODISCARD D3D12CommandList& List()const;

		NODISCARD RenderDevice& Device()const;

		NODISCARD const RenderView* View()const;

		NODISCARD Uint32 Width()const;

		NODISCARD Uint32 Height()const;

		NODISCARD ID3D12Resource* Resource(const RenderResource& resource)const;

		void ClearDepth(const RenderResource& resource);

		void ClearTarget(const RenderResource& resource);

#if SC_DEBUG
		void Verify(std::span<IDxcBlob* const> reflections);
#endif

	private:
		RenderGraph& graph_;

		D3D12CommandList& cmdList_;

		Uint32 pass_ = SC_INVALID;

		const RenderView* view_ = nullptr;
	};
}

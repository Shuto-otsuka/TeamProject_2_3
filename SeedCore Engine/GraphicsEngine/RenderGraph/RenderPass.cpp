#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	RenderPass::RenderPass(RenderGraph& graph, D3D12CommandList& list, Uint32 pass, const RenderView* view) :graph_(graph), cmdList_(list), pass_(pass), view_(view)
	{
		/// No Code
	}

	D3D12CommandList& RenderPass::List()const
	{
		return cmdList_;
	}

	RenderDevice& RenderPass::Device()const
	{
		return graph_.device_;
	}

	const RenderView* RenderPass::View()const
	{
		return view_;
	}

	Uint32 RenderPass::Width()const
	{
		return view_ ? view_->renderWidth_ : 0;
	}

	Uint32 RenderPass::Height()const
	{
		return view_ ? view_->renderHeight_ : 0;
	}

	ID3D12Resource* RenderPass::Resource(const RenderResource& resource)const
	{
		const RenderGraphPass& pass = graph_.passes_[pass_];
		const RenderGraphStage& stage = graph_.stages_[pass.stage_];
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.index_ == SC_INVALID || std::string_view(access.resourceName_) != resource.Name())
			{
				continue;
			}
			Uint32 view = graph_.resources_[access.index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
			const RenderGraphInstance& instance = graph_.instances_[access.index_ * RenderViewCount + view];
			const RenderGraphAllocation* memory = access.type_ == RenderAccessType::History && instance.history_ != SC_INVALID ? &graph_.persistent_[instance.history_] : instance.memory_;
			return memory ? memory->resource_.Get() : nullptr;
		}
		SC_LOG_ERROR("RenderPass: {} は {} を宣言していません", stage.stageName_.c_str(), resource.Name());
		return nullptr;
	}
}
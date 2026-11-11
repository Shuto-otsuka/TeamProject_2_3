#include <GraphicsEngine/Shape/HUD/HUDStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	void HUDFeature::OutputStage::Execute(HUDFeature& feature, RenderPass& pass)
	{
		const Vector4& outputRect = pass.View()->outputRect_;
		D3D12_VIEWPORT viewport{};
		viewport.TopLeftX = outputRect.x;
		viewport.TopLeftY = outputRect.y;
		viewport.Width = outputRect.z;
		viewport.Height = outputRect.w;
		viewport.MaxDepth = 1.0f;
		D3D12_RECT scissor{};
		scissor.left = static_cast<LONG>(outputRect.x);
		scissor.top = static_cast<LONG>(outputRect.y);
		scissor.right = static_cast<LONG>(outputRect.x + outputRect.z);
		scissor.bottom = static_cast<LONG>(outputRect.y + outputRect.w);
		pass.List().Get()->RSSetViewports(1, &viewport);
		pass.List().Get()->RSSetScissorRects(1, &scissor);

		feature.outputPipeline_.Draw(pass);
	}
}

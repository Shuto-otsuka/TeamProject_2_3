#include <GraphicsEngine/Graphics/GraphicsProcessor.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Texture/Compression/BC7CompressShader.h>

namespace SeedCore
{
	GraphicsProcessor::GraphicsProcessor()
	{
		/// No Code
	}

	GraphicsProcessor::~GraphicsProcessor() = default;

	void GraphicsProcessor::Initialize(RenderDevice& device)
	{
		bc7_ = MakePtr<BC7CompressShader>();
		bc7_->Create(device.ShaderCache(), device.Context().GetDevice());
	}

	void GraphicsProcessor::Finalize()
	{
		bc7_.reset();
	}

	BC7CompressShader& GraphicsProcessor::BC7()const
	{
		return *bc7_;
	}
}

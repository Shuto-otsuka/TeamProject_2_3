#pragma once

namespace SeedCore
{
	struct GraphicsSettings;

	class RenderDevice;
	class SwapChain;
	class Window;

	class GraphicsPresenter
	{
	public:
		explicit GraphicsPresenter(RenderDevice& device);

		~GraphicsPresenter();

		Bool Initialize(const Window& window);

		void Finalize();

	public:
		void Configure(const GraphicsSettings& settings);

		void Resize(Uint32 width, Uint32 height);

	public:
		void Begin();

		void Bind();

		void End();

		void Present();

	public:
		NODISCARD ID3D12Resource* BackBuffer()const;

	private:
		RenderDevice& device_;

		ResourcePtr<SwapChain> swapChain_;
	};
}

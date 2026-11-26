#pragma once

namespace SeedCore
{
	class BC7CompressShader;
	class RenderDevice;

	class SEEDCORE_API GraphicsProcessor
	{
	public:
		GraphicsProcessor();

		~GraphicsProcessor();

		void Initialize(RenderDevice& device);

		void Finalize();

	public:
		NODISCARD BC7CompressShader& BC7()const;

	private:
		ResourcePtr<BC7CompressShader> bc7_;
	};
}

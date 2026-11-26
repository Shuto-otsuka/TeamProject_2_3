#pragma once

namespace SeedCore
{
	class ShaderCache;

	class BC7CompressShader
	{
	public:
		BC7CompressShader() = default;
		~BC7CompressShader() = default;

		void Create(ShaderCache& shaderCache, ID3D12Device* device);

		NODISCARD ID3D12RootSignature* GetRootSignature()const;

		NODISCARD ID3D12PipelineState* GetPipelineState()const;

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	};
}

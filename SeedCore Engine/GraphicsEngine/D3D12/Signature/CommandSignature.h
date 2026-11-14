#pragma once

namespace SeedCore
{
	class CommandSignature
	{
	public:
		Bool Create(ID3D12Device* device, D3D12_INDIRECT_ARGUMENT_TYPE type);

		NODISCARD ID3D12CommandSignature* Signature()const;

		NODISCARD Uint32 Stride()const;

	private:
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> signature_;

		Uint32 stride_ = 0;
	};
}
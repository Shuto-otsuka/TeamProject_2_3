#pragma once

namespace SeedCore
{
	enum class RootParameter :Uint32
	{
		Stage,
		View,
		Dispatch,
	};

	class RootSignature
	{
	public:
		void Create(ID3D12Device* device);

		NODISCARD ID3D12RootSignature* Signature()const;

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	};
}
#pragma once
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12CommandQueue;

	class Skymap :public NonCopyable
	{
		friend class SkymapLoader;

	public:
		Skymap() = default;
		~Skymap() = default;

		Skymap(Skymap&&)noexcept = default;
		Skymap& operator=(Skymap&&)noexcept = default;

	public:
		NODISCARD Uint ShaderResourceViewIndex()const;

		NODISCARD Bool Valid()const;

		NODISCARD Handle<Skymap> GetHandle()const;

	private:
		Handle<Skymap> handle_;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

		Uint shaderResourceViewIndex_ = SC_INVALID;
	};
}

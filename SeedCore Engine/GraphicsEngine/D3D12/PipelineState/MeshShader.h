#pragma once
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/ArtMap.h>

namespace SeedCore
{
	class MeshShader
	{
	public:
		MeshShader() = default;
		~MeshShader() = default;

		void SetBlob(Microsoft::WRL::ComPtr<IDxcBlob> blob, Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob);

		NODISCARD D3D12_SHADER_BYTECODE Bytecode()const noexcept;

		NODISCARD IDxcBlob* Blob()const noexcept;

		NODISCARD IDxcBlob* ReflectionBlob()const noexcept;

	private:
		Microsoft::WRL::ComPtr<IDxcBlob> blob_;
		Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob_;
	};
}
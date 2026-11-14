#pragma once
#include <GraphicsEngine/Shader/ShaderPath.h>

namespace SeedCore
{
	struct ShaderCompileResult
	{
		Microsoft::WRL::ComPtr<IDxcBlob> objectBlob_;

		Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob_;

		std::string errorMessage_;
	};

	class ShaderIncludeHandler :public IDxcIncludeHandler
	{
	public:
		ShaderIncludeHandler(IDxcIncludeHandler* defaultHandler, DynamicArray<String>& dependencies);

		HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR filename, IDxcBlob** includeSource)override;

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object)override;

		ULONG STDMETHODCALLTYPE AddRef()override;

		ULONG STDMETHODCALLTYPE Release()override;

	private:
		IDxcIncludeHandler* defaultHeader_;

		DynamicArray<String>& dependencies_;
	};

	class ShaderCompiler
	{
	public:
		static ShaderCompileResult CompileVertexShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileHullShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileDomainShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileGeometryShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompilePixelShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileAmplificationShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileMeshShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileComputeShader(const std::wstring& filePath, const std::string& entryPoint = "main", ShaderMount mount = ShaderMount::Engine);

		static ShaderCompileResult CompileLibraryShader(const std::wstring& filePath, ShaderMount mount = ShaderMount::Engine);

	private:
		static ShaderCompileResult CompileInternal(String filePath, String entryPoint, String targetProfile, ShaderMount mount);
	};
}
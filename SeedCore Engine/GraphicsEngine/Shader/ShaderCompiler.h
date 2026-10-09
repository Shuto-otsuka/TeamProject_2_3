#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	struct ShaderCompileResult
	{
		Microsoft::WRL::ComPtr<IDxcBlob> objectBlob;
		Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob;
		std::string errorMessage;
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
		static ShaderCompileResult CompileVertexShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileHullShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileDomainShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileGeometryShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompilePixelShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileAmplificationShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileMeshShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileComputeShader(const std::wstring& filePath, const std::string& entryPoint = "main");

		static ShaderCompileResult CompileLibraryShader(const std::wstring& filePath);

	private:
		static ShaderCompileResult CompileInternal(String filePath, String entryPoint, String targetProfile);
	};
}
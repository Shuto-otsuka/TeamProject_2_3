#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Shader/ShaderCompiler.h>

#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/HullShader.h>
#include <GraphicsEngine/D3D12/PipelineState/DomainShader.h>
#include <GraphicsEngine/D3D12/PipelineState/GeometryShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/PipelineState/AmplificationShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/RaytracingShader.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>

namespace SeedCore
{
	namespace
	{
		StablePool<VertexShader> vertexShaderPool_;
		StablePool<HullShader> hullShaderPool_;
		StablePool<DomainShader> domainShaderPool_;
		StablePool<GeometryShader> geometryShaderPool_;
		StablePool<PixelShader> pixelShaderPool_;
		StablePool<AmplificationShader> amplificationShaderPool_;
		StablePool<MeshShader> meshShaderPool_;
		StablePool<RaytracingShader> raytracingShaderPool_;
		StablePool<ComputeShader> computeShaderPool_;
	}

	Handle<VertexShader> ShaderCache::GetOrCreateVertexShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = vertexShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileVertexShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<VertexShader>::null();
		}

		Handle<VertexShader> handle = vertexShaderPool_.Create();
		if (auto vertexShader = vertexShaderPool_.Get(handle))
		{
			vertexShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		vertexShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<VertexShader> ShaderCache::GetOrCreateVertexShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateVertexShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	VertexShader* ShaderCache::GetVertexShader(const Handle<VertexShader>& handle)noexcept
	{
		return vertexShaderPool_.Get(handle);
	}

	Handle<HullShader> ShaderCache::GetOrCreateHullShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = hullShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileHullShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<HullShader>::null();
		}

		Handle<HullShader> handle = hullShaderPool_.Create();
		if (auto hullShader = hullShaderPool_.Get(handle))
		{
			hullShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		hullShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<HullShader> ShaderCache::GetOrCreateHullShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateHullShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	HullShader* ShaderCache::GetHullShader(const Handle<HullShader>& handle)noexcept
	{
		return hullShaderPool_.Get(handle);
	}

	Handle<DomainShader> ShaderCache::GetOrCreateDomainShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = domainShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileDomainShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<DomainShader>::null();
		}

		Handle<DomainShader> handle = domainShaderPool_.Create();
		if (auto domainShader = domainShaderPool_.Get(handle))
		{
			domainShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		domainShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<DomainShader> ShaderCache::GetOrCreateDomainShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateDomainShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	DomainShader* ShaderCache::GetDomainShader(const Handle<DomainShader>& handle)noexcept
	{
		return domainShaderPool_.Get(handle);
	}

	Handle<GeometryShader> ShaderCache::GetOrCreateGeometryShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = geometryShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileGeometryShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<GeometryShader>::null();
		}

		Handle<GeometryShader> handle = geometryShaderPool_.Create();
		if (auto geometryShader = geometryShaderPool_.Get(handle))
		{
			geometryShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		geometryShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<GeometryShader> ShaderCache::GetOrCreateGeometryShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateGeometryShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	GeometryShader* ShaderCache::GetGeometryShader(const Handle<GeometryShader>& handle)noexcept
	{
		return geometryShaderPool_.Get(handle);
	}

	Handle<PixelShader> ShaderCache::GetOrCreatePixelShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = pixelShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompilePixelShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<PixelShader>::null();
		}

		Handle<PixelShader> handle = pixelShaderPool_.Create();
		if (auto pixelShader = pixelShaderPool_.Get(handle))
		{
			pixelShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		pixelShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<PixelShader> ShaderCache::GetOrCreatePixelShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreatePixelShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	PixelShader* ShaderCache::GetPixelShader(const Handle<PixelShader>& handle)noexcept
	{
		return pixelShaderPool_.Get(handle);
	}

	Handle<AmplificationShader> ShaderCache::GetOrCreateAmplificationShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = amplificationShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileAmplificationShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<AmplificationShader>::null();
		}

		Handle<AmplificationShader> handle = amplificationShaderPool_.Create();
		if (auto amplificationShader = amplificationShaderPool_.Get(handle))
		{
			amplificationShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		amplificationShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<AmplificationShader> ShaderCache::GetOrCreateAmplificationShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateAmplificationShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	AmplificationShader* ShaderCache::GetAmplificationShader(const Handle<AmplificationShader>& handle)noexcept
	{
		return amplificationShaderPool_.Get(handle);
	}

	Handle<MeshShader> ShaderCache::GetOrCreateMeshShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = meshShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileMeshShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<MeshShader>::null();
		}

		Handle<MeshShader> handle = meshShaderPool_.Create();
		if (auto meshShader = meshShaderPool_.Get(handle))
		{
			meshShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		meshShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<MeshShader> ShaderCache::GetOrCreateMeshShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateMeshShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	MeshShader* ShaderCache::GetMeshShader(const Handle<MeshShader>& handle)noexcept
	{
		return meshShaderPool_.Get(handle);
	}

	Handle<RaytracingShader> ShaderCache::GetOrCreateRaytracingShader(String filePath, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;

		if (auto found = raytracingShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileLibraryShader(filePath.w_str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<RaytracingShader>::null();
		}

		Handle<RaytracingShader> handle = raytracingShaderPool_.Create();
		if (auto raytracingShader = raytracingShaderPool_.Get(handle))
		{
			raytracingShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		raytracingShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<RaytracingShader> ShaderCache::GetOrCreateRaytracingShader(const ShaderPath& path)
	{
		return GetOrCreateRaytracingShader(String(path.Absolute().wstring()), path.Mount());
	}

	RaytracingShader* ShaderCache::GetRaytracingShader(const Handle<RaytracingShader>& handle)noexcept
	{
		return raytracingShaderPool_.Get(handle);
	}

	Handle<ComputeShader> ShaderCache::GetOrCreateComputeShader(String filePath, String entryPoint, ShaderMount mount)
	{
		Key key{};
		key.filePath_ = filePath;
		key.entryPoint_ = entryPoint;

		if (auto found = computeShaderCache_.find(key))
		{
			return *found;
		}

		ShaderCompileResult compileResult = ShaderCompiler::CompileComputeShader(filePath.w_str(), entryPoint.str(), mount);
		if (!compileResult.objectBlob_)
		{
			return Handle<ComputeShader>::null();
		}

		Handle<ComputeShader> handle = computeShaderPool_.Create();
		if (auto computeShader = computeShaderPool_.Get(handle))
		{
			computeShader->SetBlob(compileResult.objectBlob_, compileResult.reflectionBlob_);
		}

		computeShaderCache_.insert(key, handle);
		return handle;
	}

	Handle<ComputeShader> ShaderCache::GetOrCreateComputeShader(const ShaderPath& path, String entryPoint)
	{
		return GetOrCreateComputeShader(String(path.Absolute().wstring()), entryPoint, path.Mount());
	}

	ComputeShader* ShaderCache::GetComputeShader(const Handle<ComputeShader>& handle)noexcept
	{
		return computeShaderPool_.Get(handle);
	}

	void ShaderCache::Clear()noexcept
	{
		for (auto it = vertexShaderCache_.begin(); it != vertexShaderCache_.end(); ++it)
		{
			Handle<VertexShader> handle = it->second;
			vertexShaderPool_.Destroy(handle);
		}
		vertexShaderCache_.clear();

		for (auto it = hullShaderCache_.begin(); it != hullShaderCache_.end(); ++it)
		{
			hullShaderPool_.Destroy(it->second);
		}
		hullShaderCache_.clear();

		for (auto it = domainShaderCache_.begin(); it != domainShaderCache_.end(); ++it)
		{
			domainShaderPool_.Destroy(it->second);
		}
		domainShaderCache_.clear();

		for (auto it = geometryShaderCache_.begin(); it != geometryShaderCache_.end(); ++it)
		{
			geometryShaderPool_.Destroy(it->second);
		}
		geometryShaderCache_.clear();

		for (auto it = pixelShaderCache_.begin(); it != pixelShaderCache_.end(); ++it)
		{
			pixelShaderPool_.Destroy(it->second);
		}
		pixelShaderCache_.clear();

		for (auto it = amplificationShaderCache_.begin(); it != amplificationShaderCache_.end(); ++it)
		{
			amplificationShaderPool_.Destroy(it->second);
		}
		amplificationShaderCache_.clear();

		for (auto it = meshShaderCache_.begin(); it != meshShaderCache_.end(); ++it)
		{
			meshShaderPool_.Destroy(it->second);
		}
		meshShaderCache_.clear();

		for (auto it = raytracingShaderCache_.begin(); it != raytracingShaderCache_.end(); ++it)
		{
			raytracingShaderPool_.Destroy(it->second);
		}
		raytracingShaderCache_.clear();

		for (auto it = computeShaderCache_.begin(); it != computeShaderCache_.end(); ++it)
		{
			computeShaderPool_.Destroy(it->second);
		}
		computeShaderCache_.clear();
	}
}
#pragma once
#include <GraphicsEngine/Shader/ShaderPath.h>
#include <FoundationEngine/Utility/ArtMap.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	class VertexShader;
	class HullShader;
	class DomainShader;
	class GeometryShader;
	class PixelShader;
	class AmplificationShader;
	class MeshShader;
	class RaytracingShader;
	class ComputeShader;

	class ShaderCache :public NonTransferable
	{
	public:
#pragma pack(push, 1)
		struct Key
		{
			String filePath_;
			String entryPoint_;
		};
#pragma pack(pop)

	public:
		ShaderCache() = default;
		~ShaderCache() = default;

		NODISCARD Handle<VertexShader> GetOrCreateVertexShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<VertexShader> GetOrCreateVertexShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD VertexShader* GetVertexShader(const Handle<VertexShader>& handle)noexcept;

		NODISCARD Handle<HullShader> GetOrCreateHullShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<HullShader> GetOrCreateHullShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD HullShader* GetHullShader(const Handle<HullShader>& handle)noexcept;

		NODISCARD Handle<DomainShader> GetOrCreateDomainShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<DomainShader> GetOrCreateDomainShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD DomainShader* GetDomainShader(const Handle<DomainShader>& handle)noexcept;

		NODISCARD Handle<GeometryShader> GetOrCreateGeometryShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<GeometryShader> GetOrCreateGeometryShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD GeometryShader* GetGeometryShader(const Handle<GeometryShader>& handle)noexcept;

		NODISCARD Handle<PixelShader> GetOrCreatePixelShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<PixelShader> GetOrCreatePixelShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD PixelShader* GetPixelShader(const Handle<PixelShader>& handle)noexcept;

		NODISCARD Handle<AmplificationShader> GetOrCreateAmplificationShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<AmplificationShader> GetOrCreateAmplificationShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD AmplificationShader* GetAmplificationShader(const Handle<AmplificationShader>& handle)noexcept;

		NODISCARD Handle<MeshShader> GetOrCreateMeshShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<MeshShader> GetOrCreateMeshShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD MeshShader* GetMeshShader(const Handle<MeshShader>& handle)noexcept;

		NODISCARD Handle<RaytracingShader> GetOrCreateRaytracingShader(String filePath, ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<RaytracingShader> GetOrCreateRaytracingShader(const ShaderPath& path);

		NODISCARD RaytracingShader* GetRaytracingShader(const Handle<RaytracingShader>& handle)noexcept;

		NODISCARD Handle<ComputeShader> GetOrCreateComputeShader(String filePath, String entryPoint = String("main"), ShaderMount mount = ShaderMount::Engine);

		NODISCARD Handle<ComputeShader> GetOrCreateComputeShader(const ShaderPath& path, String entryPoint = String("main"));

		NODISCARD ComputeShader* GetComputeShader(const Handle<ComputeShader>& handle)noexcept;

		void Clear()noexcept;

	private:
		ArtMap<Key, Handle<VertexShader>> vertexShaderCache_;

		ArtMap<Key, Handle<HullShader>> hullShaderCache_;

		ArtMap<Key, Handle<DomainShader>> domainShaderCache_;

		ArtMap<Key, Handle<GeometryShader>> geometryShaderCache_;

		ArtMap<Key, Handle<PixelShader>> pixelShaderCache_;

		ArtMap<Key, Handle<AmplificationShader>> amplificationShaderCache_;

		ArtMap<Key, Handle<MeshShader>> meshShaderCache_;

		ArtMap<Key, Handle<RaytracingShader>> raytracingShaderCache_;

		ArtMap<Key, Handle<ComputeShader>> computeShaderCache_;
	};
}
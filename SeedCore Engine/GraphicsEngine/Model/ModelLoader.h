#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace tinygltf
{
	class Model;
}

namespace fbxsdk
{
	class FbxScene;
	class FbxNode;
	class FbxMesh;
	class FbxSurfaceMaterial;
	class FbxFileTexture;
	class FbxManager;
}

namespace SeedCore
{
	class BindlessHeap;
	class GraphicsProcessor;
	class D3D12Context;

	enum class ModelFormat
	{
		Gltf,
		Fbx,
	};

	struct GltfSource
	{
		const tinygltf::Model* model_ = nullptr;
	};

	struct FbxSource
	{
		fbxsdk::FbxScene* scene_ = nullptr;
		std::string directory_;
		std::string stem_;
		Float unitScale_ = 1.0f;
		DynamicArray<fbxsdk::FbxNode*> nodeTable_;
		DynamicArray<fbxsdk::FbxMesh*> meshTable_;
		mutable DynamicArray<fbxsdk::FbxSurfaceMaterial*> materialTable_;
		mutable DynamicArray<fbxsdk::FbxFileTexture*> textureTable_;
	};

	struct ModelSource
	{
		ModelFormat format_ = ModelFormat::Gltf;
		GltfSource gltf_;
		FbxSource fbx_;
	};

	class SEEDCORE_API ModelLoader :public AssetLoader<Crister>
	{
	private:
		friend class ModelResource;
		friend class ModelExporter;

	public:
		ModelLoader(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);
		~ModelLoader() = default;

		Handle<Crister> Load(String filePath, LoadThread thread);

		void Clear(Handle<Crister>& handle)noexcept;

	private:
		Handle<Crister> Import(String filePath, LoadThread thread, Bool forExport);

		Bool Bake(Crister& crister, String filePath);

		void FetchStages(const ModelSource& source, Crister& crister);

		void FetchNodes(const ModelSource& source, Crister& crister);

		void FetchMeshes(const ModelSource& source, Crister& crister);

		void FetchMaterials(const ModelSource& source, Crister& crister);

		void FetchSkins(const ModelSource& source, Crister& crister);

		void ResolveSubMeshSkins(const ModelSource& source, Crister& crister);

		void FetchTexture(const ModelSource& source, Crister& crister);

		void ConvertSceneConvention(fbxsdk::FbxScene* scene, fbxsdk::FbxManager* manager);

		void ConvertFlatConvention(ModelSource& source);

		void FetchLights(const ModelSource& source, Crister& crister);

	private:
		D3D12Context& context_;

		BindlessHeap& heap_;

		GraphicsProcessor& processor_;
	};
}

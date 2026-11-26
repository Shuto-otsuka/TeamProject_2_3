#ifdef __clang__
#define STBI_NO_SIMD
#endif
#include <External/TinyglTF/Include/tiny_gltf.h>
#include <External/DirectXTex/Include/DirectXTex.h>
#ifndef FBXSDK_SHARED
#define FBXSDK_SHARED
#endif
#include <External/FBXSDK/Include/fbxsdk.h>
#include <GraphicsEngine/Model/ModelLoader.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <GraphicsEngine/Model/Crister/CristerCompressor.h>
#include <GraphicsEngine/Model/Crister/CristerConverter.h>
#include <GraphicsEngine/Model/Crister/CristerClusterizer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

#ifdef _DEBUG
#pragma comment(lib, "../External/FBXSDK/Lib/Debug/libfbxsdk.lib")
#pragma comment(lib, "../External/DirectXTex/Lib/Debug/DirectXTex.lib")
#else
#pragma comment(lib, "../External/FBXSDK/Lib/Release/libfbxsdk.lib")
#pragma comment(lib, "../External/DirectXTex/Lib/Release/DirectXTex.lib")
#endif

namespace SeedCore
{
	ModelLoader::ModelLoader(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor) :context_(context), heap_(heap), processor_(processor)
	{
		/// No Code
	}

	Handle<Crister> ModelLoader::Load(String filePath, LoadThread thread)
	{
		return Import(filePath, thread, false);
	}

	void ModelLoader::Clear(Handle<Crister>& handle)noexcept
	{
		if (Crister* crister = pool_.Get(handle))
		{
			crister->Residency().Release(&heap_);
		}
		pool_.Destroy(handle);
	}

	Bool ModelLoader::Bake(Crister& crister, String filePath)
	{
		BinaryOutputArchive archive;
		crister.Serialize(archive);
		return archive.Write(filePath);
	}

	Handle<Crister> ModelLoader::Import(String filePath, LoadThread thread, Bool forExport)
	{
		ID3D12Device* device = context_.GetDevice();
		D3D12CommandQueue* cmdQueue = context_.GetDirectQueue();
		D3D12CommandQueue* copyQueue = context_.GetCopyQueue();
		D3D12TransferLane* transferLane = context_.GetLoadTransferLane(thread);
		BindlessHeap* heap = &heap_;
		GraphicsProcessor& processor = processor_;

		AssetMeta meta;
		meta.Read(std::filesystem::path(filePath.c_str()));
		AxisConvention axisConvention = meta.axisConvention_;

		Handle<Crister> handle = pool_.Create();
		Crister* crister = pool_.Get(handle);
		if (!crister)
		{
			return Handle<Crister>::null();
		}

		std::filesystem::path path(filePath.c_str());
		std::filesystem::path cristerPath = path;
		cristerPath.replace_extension(".crister");

		Bool loadedFromCache = false;
		if (path.extension() == ".crister" && std::filesystem::exists(cristerPath))
		{
			BinaryInputArchive archive;
			if (archive.Read(String(cristerPath.string())))
			{
				crister->Serialize(archive);
				loadedFromCache = true;

				for (Size nodeIndex = 0; nodeIndex < crister->scene_.nodes_.size(); nodeIndex++)
				{
					for (Int childIndex : crister->scene_.nodes_[nodeIndex].children_)
					{
						if (childIndex >= 0 && static_cast<Size>(childIndex) < crister->scene_.nodes_.size())
						{
							crister->scene_.nodes_[childIndex].parentIndex_ = static_cast<Int>(nodeIndex);
						}
					}
				}

				if (forExport)
				{
					CristerQuantizer quantizer;
					quantizer.Reconstruct(*crister);
				}
			}
			else
			{
				*crister = Crister{};
			}
		}

		if (!loadedFromCache)
		{
			std::string pathString = path.string();

			ModelFormat format = ModelFormat::Gltf;
			if (path.extension() == ".fbx")
			{
				format = ModelFormat::Fbx;
			}

			tinygltf::Model model;
			FbxManager* fbxManager = nullptr;
			FbxScene* fbxScene = nullptr;

			switch (format)
			{
			case ModelFormat::Fbx:
			{
				fbxManager = FbxManager::Create();
				fbxManager->SetIOSettings(FbxIOSettings::Create(fbxManager, IOSROOT));
				fbxScene = FbxScene::Create(fbxManager, "");

				FbxImporter* importer = FbxImporter::Create(fbxManager, "");
				Bool ok = importer->Initialize(pathString.c_str(), -1, fbxManager->GetIOSettings());
				if (ok)
				{
					ok = importer->Import(fbxScene);
				}
				importer->Destroy();

				if (!ok)
				{
					fbxManager->Destroy();
					pool_.Destroy(handle);
					return Handle<Crister>::null();
				}
				break;
			}
			case ModelFormat::Gltf:
			{
				tinygltf::TinyGLTF tinyLoader;
				std::string error, warning;
				Bool result = false;
				if (path.extension() == ".glb")
				{
					result = tinyLoader.LoadBinaryFromFile(&model, &error, &warning, pathString);
				}
				else
				{
					result = tinyLoader.LoadASCIIFromFile(&model, &error, &warning, pathString);
				}

				if (!result)
				{
					pool_.Destroy(handle);
					return Handle<Crister>::null();
				}
				break;
			}
			}

			ModelSource source{};
			source.format_ = format;
			source.gltf_.model_ = &model;
			source.fbx_.scene_ = fbxScene;
			source.fbx_.directory_ = path.parent_path().string();
			source.fbx_.stem_ = path.stem().string();

			if (format == ModelFormat::Fbx)
			{
				source.fbx_.unitScale_ = static_cast<Float>(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor()) / 100.0f;
				ConvertSceneConvention(fbxScene, fbxManager);
				ConvertFlatConvention(source);
			}

			FetchStages(source, *crister);
			FetchNodes(source, *crister);
			FetchMaterials(source, *crister);
			FetchMeshes(source, *crister);
			FetchSkins(source, *crister);
			ResolveSubMeshSkins(source, *crister);
			FetchTexture(source, *crister);

			CristerConverter converter;
			converter.ConvertAxisConvention(*crister, axisConvention);

			FetchLights(source, *crister);

			if (fbxManager)
			{
				fbxManager->Destroy();
				fbxManager = nullptr;
				fbxScene = nullptr;
				source.fbx_.scene_ = nullptr;
			}

			if (forExport)
			{
				return handle;
			}

			CristerClusterizer clusterizer;
			clusterizer.BuildMeshlets(*crister);

			CristerQuantizer quantizer;
			quantizer.BakeMesh(*crister);

			CristerCompressor compressor;
			compressor.BakeBitmap(device, cmdQueue, processor, crister->material_.bitmaps_);
		}

		if (forExport)
		{
			return handle;
		}

		transferLane->Begin(device, *copyQueue);
		crister->Residency().Upload(*crister, device, *transferLane, heap);
		copyQueue->WaitFor(transferLane->Submit(*copyQueue));

		return handle;
	}

	void ModelLoader::FetchStages(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;
			for (const tinygltf::Scene& gltfScene : model.scenes)
			{
				Stage& stage = crister.scene_.stages_.emplace_back();
				stage.name_ = gltfScene.name;
				stage.nodes_ = gltfScene.nodes;
			}
			crister.scene_.defaultStage_ = model.defaultScene < 0 ? 0 : model.defaultScene;
			break;
		}
		case ModelFormat::Fbx:
		{
			FbxScene* fbxScene = source.fbx_.scene_;
			Stage& stage = crister.scene_.stages_.emplace_back();
			stage.name_ = fbxScene->GetName();
			FbxNode* fbxRoot = fbxScene->GetRootNode();
			for (Int childIndex = 0; childIndex < fbxRoot->GetChildCount(); childIndex++)
			{
				FbxNode* fbxChild = fbxRoot->GetChild(childIndex);
				for (Size tableIndex = 0; tableIndex < source.fbx_.nodeTable_.size(); tableIndex++)
				{
					if (source.fbx_.nodeTable_[tableIndex] == fbxChild)
					{
						stage.nodes_.push_back(static_cast<Int>(tableIndex));
						break;
					}
				}
			}
			crister.scene_.defaultStage_ = 0;
			break;
		}
		}
	}

	void ModelLoader::FetchNodes(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			for (const tinygltf::Node& gltfNode : model.nodes)
			{
				Node& node = crister.scene_.nodes_.emplace_back();
				node.name_ = gltfNode.name;
				node.skin_ = gltfNode.skin;
				node.mesh_ = gltfNode.mesh;
				node.children_ = gltfNode.children;

				if (auto it = gltfNode.extensions.find("KHR_lights_punctual"); it != gltfNode.extensions.end())
				{
					if (it->second.Has("light"))
					{
						node.light_ = static_cast<Int>(it->second.Get("light").GetNumberAsDouble());
					}
				}

				if (!gltfNode.matrix.empty())
				{
					Matrix matrix{};
					for (Size row = 0;row < 4;row++)
					{
						for (Size column = 0;column < 4;column++)
						{
							matrix.m[row][column] = static_cast<Float>(gltfNode.matrix.at(4 * row + column));
						}
					}
					Vector3 cacheScale, cacheTranslation;
					Quaternion cacheRotation;
					matrix.Decompose(cacheTranslation, cacheRotation, cacheScale);
					node.scale_ = cacheScale;
					node.rotation_ = cacheRotation;
					node.translation_ = cacheTranslation;
				}
				else
				{
					if (gltfNode.scale.size() == 3)
					{
						node.scale_.x = static_cast<Float>(gltfNode.scale[0]);
						node.scale_.y = static_cast<Float>(gltfNode.scale[1]);
						node.scale_.z = static_cast<Float>(gltfNode.scale[2]);
					}
					if (gltfNode.rotation.size() == 4)
					{
						node.rotation_.x = static_cast<Float>(gltfNode.rotation[0]);
						node.rotation_.y = static_cast<Float>(gltfNode.rotation[1]);
						node.rotation_.z = static_cast<Float>(gltfNode.rotation[2]);
						node.rotation_.w = static_cast<Float>(gltfNode.rotation[3]);
					}
					if (gltfNode.translation.size() == 3)
					{
						node.translation_.x = static_cast<Float>(gltfNode.translation[0]);
						node.translation_.y = static_cast<Float>(gltfNode.translation[1]);
						node.translation_.z = static_cast<Float>(gltfNode.translation[2]);
					}
				}
			}

			for (Size nodeIndex = 0; nodeIndex < crister.scene_.nodes_.size(); nodeIndex++)
			{
				for (Int childIndex : crister.scene_.nodes_[nodeIndex].children_)
				{
					if (childIndex >= 0 && static_cast<Size>(childIndex) < crister.scene_.nodes_.size())
					{
						crister.scene_.nodes_[childIndex].parentIndex_ = static_cast<Int>(nodeIndex);
					}
				}
			}

			crister.CumulateTransforms();
			break;
		}
		case ModelFormat::Fbx:
		{
			for (Size tableIndex = 0; tableIndex < source.fbx_.nodeTable_.size(); tableIndex++)
			{
				FbxNode* fbxNode = source.fbx_.nodeTable_[tableIndex];
				Node& node = crister.scene_.nodes_.emplace_back();
				node.name_ = fbxNode->GetName();

				for (Int fbxChildIndex = 0; fbxChildIndex < fbxNode->GetChildCount(); fbxChildIndex++)
				{
					FbxNode* fbxChild = fbxNode->GetChild(fbxChildIndex);
					for (Size searchIndex = 0; searchIndex < source.fbx_.nodeTable_.size(); searchIndex++)
					{
						if (source.fbx_.nodeTable_[searchIndex] == fbxChild)
						{
							node.children_.push_back(static_cast<Int>(searchIndex));
							break;
						}
					}
				}

				FbxMesh* fbxMesh = fbxNode->GetMesh();
				if (fbxMesh)
				{
					for (Size meshIndex = 0; meshIndex < source.fbx_.meshTable_.size(); meshIndex++)
					{
						if (source.fbx_.meshTable_[meshIndex] == fbxMesh)
						{
							node.mesh_ = static_cast<Int>(meshIndex);
							break;
						}
					}
				}

				FbxAMatrix localTransform = fbxNode->EvaluateLocalTransform();
				FbxVector4 localTranslation = localTransform.GetT();
				FbxQuaternion localRotation = localTransform.GetQ();
				FbxVector4 localScale = localTransform.GetS();
				node.translation_ = Vector3(static_cast<Float>(localTranslation[0]), static_cast<Float>(localTranslation[1]), static_cast<Float>(localTranslation[2])) * source.fbx_.unitScale_;
				node.rotation_ = Quaternion(static_cast<Float>(localRotation[0]), static_cast<Float>(localRotation[1]), static_cast<Float>(localRotation[2]), static_cast<Float>(localRotation[3]));
				node.scale_ = Vector3(static_cast<Float>(localScale[0]), static_cast<Float>(localScale[1]), static_cast<Float>(localScale[2]));
			}

			for (Size nodeIndex = 0; nodeIndex < crister.scene_.nodes_.size(); nodeIndex++)
			{
				for (Int childIndex : crister.scene_.nodes_[nodeIndex].children_)
				{
					if (childIndex >= 0 && static_cast<Size>(childIndex) < crister.scene_.nodes_.size())
					{
						crister.scene_.nodes_[childIndex].parentIndex_ = static_cast<Int>(nodeIndex);
					}
				}
			}

			crister.CumulateTransforms();
			break;
		}
		}
	}

	void ModelLoader::FetchMeshes(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			Int defaultSurfaceIndex = -1;

			for (Size meshIndex = 0; meshIndex < model.meshes.size(); meshIndex++)
			{
				const tinygltf::Mesh& gltfMesh = model.meshes[meshIndex];
				for (const tinygltf::Primitive& gltfPrimitive : gltfMesh.primitives)
				{
					SubMesh& subMesh = crister.geometry_.subMeshes_.emplace_back();
					subMesh.meshIndex_ = static_cast<Int>(meshIndex);
					if (gltfPrimitive.material >= 0)
					{
						subMesh.surfaceIndex_ = static_cast<Uint32>(gltfPrimitive.material);
					}
					else
					{
						if (defaultSurfaceIndex < 0)
						{
							defaultSurfaceIndex = static_cast<Int>(crister.material_.surfaces_.size());
							crister.material_.surfaces_.emplace_back();
						}
						subMesh.surfaceIndex_ = static_cast<Uint32>(defaultSurfaceIndex);
					}

					Uint32 vertexOffset = static_cast<Uint32>(crister.geometry_.vertices_.size());
					if (!gltfPrimitive.attributes.contains("POSITION"))
					{
						continue;
					}

					const tinygltf::Accessor& positionAccessor = model.accessors.at(gltfPrimitive.attributes.at("POSITION"));
					Size vertexCount = positionAccessor.count;
					Size baseVertex = crister.geometry_.vertices_.size();
					crister.geometry_.vertices_.resize(baseVertex + vertexCount);

					crister.geometry_.vertexMorphSource_.resize(baseVertex + vertexCount);
					for (Size index = 0; index < vertexCount; index++)
					{
						crister.geometry_.vertexMorphSource_[baseVertex + index] = static_cast<Uint32>(baseVertex + index);
					}

					subMesh.vertexOffset_ = vertexOffset;
					subMesh.vertexCount_ = static_cast<Uint32>(vertexCount);

					auto decodeComponent = [](const Uchar* bytes, Int componentType, Bool normalized) -> Float
						{
							switch (componentType)
							{
							case TINYGLTF_COMPONENT_TYPE_FLOAT:
								return *reinterpret_cast<const Float*>(bytes);
							case TINYGLTF_COMPONENT_TYPE_BYTE:
							{
								Int8 raw = *reinterpret_cast<const Int8*>(bytes);
								return normalized ? Max(static_cast<Float>(raw) / 127.0f, -1.0f) : static_cast<Float>(raw);
							}
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							{
								Uchar raw = *bytes;
								return normalized ? static_cast<Float>(raw) / 255.0f : static_cast<Float>(raw);
							}
							case TINYGLTF_COMPONENT_TYPE_SHORT:
							{
								Int16 raw = *reinterpret_cast<const Int16*>(bytes);
								return normalized ? Max(static_cast<Float>(raw) / 32767.0f, -1.0f) : static_cast<Float>(raw);
							}
							case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							{
								Ushort raw = *reinterpret_cast<const Ushort*>(bytes);
								return normalized ? static_cast<Float>(raw) / 65535.0f : static_cast<Float>(raw);
							}
							default:
								return 0.0f;
							}
						};

					auto fetchAttribute = [&](const std::string& name, auto memberPtr, Size componentCount)
						{
							auto it = gltfPrimitive.attributes.find(name);
							if (it == gltfPrimitive.attributes.end())
							{
								return;
							}

							const tinygltf::Accessor& accessor = model.accessors.at(it->second);
							const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
							const Uchar* source = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;
							Size sourceStride = accessor.ByteStride(bufferView);
							Size componentByteSize = static_cast<Size>(tinygltf::GetComponentSizeInBytes(static_cast<Uint32>(accessor.componentType)));
							Bool isPlainFloat = accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT;

							for (Size index = 0;index < vertexCount;index++)
							{
								Float* destination = reinterpret_cast<Float*>(&(crister.geometry_.vertices_[baseVertex + index].*memberPtr));
								const Uchar* element = source + index * sourceStride;
								if (isPlainFloat)
								{
									memcpy(destination, element, componentCount * sizeof(Float));
								}
								else
								{
									for (Size component = 0;component < componentCount;component++)
									{
										destination[component] = decodeComponent(element + component * componentByteSize, accessor.componentType, accessor.normalized);
									}
								}
							}
						};

					fetchAttribute("POSITION", &Vertex::position_, 3);
					fetchAttribute("NORMAL", &Vertex::normal_, 3);
					fetchAttribute("TANGENT", &Vertex::tangent_, 4);
					fetchAttribute("TEXCOORD_0", &Vertex::texcoord_, 2);

					{
						auto it = gltfPrimitive.attributes.find("JOINTS_0");
						if (it != gltfPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = model.accessors.at(it->second);
							const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
							const Uchar* source = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;

							for (Size index = 0;index < vertexCount;index++)
							{
								Vertex& vertex = crister.geometry_.vertices_[baseVertex + index];
								if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
								{
									const Uint* data = reinterpret_cast<const Uint*>(source + index * accessor.ByteStride(bufferView));
									vertex.joints_ = { data[0],data[1],data[2],data[3] };
								}
								else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
								{
									const Ushort* data = reinterpret_cast<const Ushort*>(source + index * accessor.ByteStride(bufferView));
									vertex.joints_ = { data[0],data[1],data[2],data[3] };
								}
								else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
								{
									const Uchar* data = reinterpret_cast<const Uchar*>(source + index * accessor.ByteStride(bufferView));
									vertex.joints_ = { data[0],data[1],data[2],data[3] };
								}
							}
						}
					}

					{
						auto it = gltfPrimitive.attributes.find("WEIGHTS_0");
						if (it != gltfPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = model.accessors.at(it->second);
							const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
							const Uchar* source = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;

							for (Size index = 0;index < vertexCount;index++)
							{
								Vertex& vertex = crister.geometry_.vertices_[baseVertex + index];
								if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
								{
									const Float* data = reinterpret_cast<const Float*>(source + index * accessor.ByteStride(bufferView));
									vertex.weights_ = Vector4(data[0], data[1], data[2], data[3]);
								}
								else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
								{
									const Ushort* data = reinterpret_cast<const Ushort*>(source + index * accessor.ByteStride(bufferView));
									vertex.weights_ = Vector4(data[0] / 65535.0f, data[1] / 65535.0f, data[2] / 65535.0f, data[3] / 65535.0f);
								}
								else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
								{
									const Uchar* data = reinterpret_cast<const Uchar*>(source + index * accessor.ByteStride(bufferView));
									vertex.weights_ = Vector4(data[0] / 255.0f, data[1] / 255.0f, data[2] / 255.0f, data[3] / 255.0f);
								}
							}
						}
					}

					const tinygltf::Value& targetNames = gltfMesh.extras.Has("targetNames") ? gltfMesh.extras.Get("targetNames") : tinygltf::Value();
					for (Size targetIndex = 0;targetIndex < gltfPrimitive.targets.size();targetIndex++)
					{
						const std::map<std::string, int>& target = gltfPrimitive.targets[targetIndex];
						auto positionIt = target.find("POSITION");
						if (positionIt == target.end())
						{
							continue;
						}

						Morph& morph = subMesh.morphs_.emplace_back();
						morph.name_ = (targetNames.IsArray() && targetIndex < targetNames.ArrayLen() && targetNames.Get(static_cast<Int>(targetIndex)).IsString())
							? targetNames.Get(static_cast<Int>(targetIndex)).Get<std::string>()
							: "Morph_" + std::to_string(targetIndex);
						morph.positionDeltas_.resize(vertexCount);

						const tinygltf::Accessor& accessor = model.accessors.at(positionIt->second);
						const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
						const Uchar* source = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;
						Size sourceStride = accessor.ByteStride(bufferView);
						Size componentByteSize = static_cast<Size>(tinygltf::GetComponentSizeInBytes(static_cast<Uint32>(accessor.componentType)));
						Bool isPlainFloat = accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT;

						for (Size index = 0;index < vertexCount;index++)
						{
							const Uchar* element = source + index * sourceStride;
							if (isPlainFloat)
							{
								Float delta[3];
								memcpy(delta, element, sizeof(delta));
								morph.positionDeltas_[index] = Vector3(delta[0], delta[1], delta[2]);
							}
							else
							{
								morph.positionDeltas_[index] = Vector3(
									decodeComponent(element + 0 * componentByteSize, accessor.componentType, accessor.normalized),
									decodeComponent(element + 1 * componentByteSize, accessor.componentType, accessor.normalized),
									decodeComponent(element + 2 * componentByteSize, accessor.componentType, accessor.normalized));
							}
						}
					}

					subMesh.indexOffset_ = static_cast<Uint32>(crister.geometry_.vertexIndices_.size());

					if (gltfPrimitive.indices > -1)
					{
						const tinygltf::Accessor& accessor = model.accessors.at(gltfPrimitive.indices);
						const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
						const Uchar* source = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;

						Size indexBase = crister.geometry_.vertexIndices_.size();
						crister.geometry_.vertexIndices_.resize(indexBase + accessor.count);

						for (Size index = 0;index < accessor.count;index++)
						{
							Uint32 vertexIndex = 0;
							if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
							{
								vertexIndex = *reinterpret_cast<const Uint32*>(source + index * sizeof(Uint32));
							}
							else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
							{
								vertexIndex = *reinterpret_cast<const Ushort*>(source + index * sizeof(Ushort));
							}
							else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
							{
								vertexIndex = source[index];
							}
							crister.geometry_.vertexIndices_[indexBase + index] = vertexOffset + vertexIndex;
						}
					}
					else
					{
						Size indexBase = crister.geometry_.vertexIndices_.size();
						crister.geometry_.vertexIndices_.resize(indexBase + vertexCount);
						for (Size index = 0;index < vertexCount;index++)
						{
							crister.geometry_.vertexIndices_[indexBase + index] = vertexOffset + static_cast<Uint32>(index);
						}
					}

					subMesh.indexCount_ = static_cast<Uint32>(crister.geometry_.vertexIndices_.size()) - subMesh.indexOffset_;
				}
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			Int defaultSurfaceIndex = -1;

			for (Size meshTableIndex = 0; meshTableIndex < source.fbx_.meshTable_.size(); meshTableIndex++)
			{
				FbxMesh* fbxMesh = source.fbx_.meshTable_[meshTableIndex];
				FbxNode* meshNode = fbxMesh->GetNode();
				Int controlPointCount = fbxMesh->GetControlPointsCount();
				FbxVector4* controlPoints = fbxMesh->GetControlPoints();

				FbxStringList uvSetNames;
				fbxMesh->GetUVSetNames(uvSetNames);
				const Char* uvSetName = uvSetNames.GetCount() > 0 ? uvSetNames.GetStringAt(0) : nullptr;

				FbxGeometryElementMaterial* materialElement = fbxMesh->GetElementMaterial(0);

				DynamicArray<Uint32> controlPointJoints(static_cast<Size>(controlPointCount) * 4, 0);
				DynamicArray<Float> controlPointWeights(static_cast<Size>(controlPointCount) * 4, 0.0f);

				Bool meshHasSkin = fbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
				Int meshSkinIndex = -1;
				if (meshHasSkin)
				{
					Int skinCounter = 0;
					for (Size earlierMeshIndex = 0; earlierMeshIndex < meshTableIndex; earlierMeshIndex++)
					{
						if (source.fbx_.meshTable_[earlierMeshIndex]->GetDeformerCount(FbxDeformer::eSkin) > 0)
						{
							skinCounter++;
						}
					}
					meshSkinIndex = skinCounter;

					FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::eSkin));
					Uint32 localJointIndex = 0;
					for (Int clusterIndex = 0; clusterIndex < skin->GetClusterCount(); clusterIndex++)
					{
						FbxCluster* cluster = skin->GetCluster(clusterIndex);
						FbxNode* boneNode = cluster->GetLink();
						if (!boneNode)
						{
							continue;
						}
						Uint32 boneIndex = localJointIndex;
						localJointIndex++;

						Int influenceCount = cluster->GetControlPointIndicesCount();
						Int* influenceIndices = cluster->GetControlPointIndices();
						Double* influenceWeights = cluster->GetControlPointWeights();
						for (Int influence = 0; influence < influenceCount; influence++)
						{
							Int controlPoint = influenceIndices[influence];
							if (controlPoint < 0 || controlPoint >= controlPointCount)
							{
								continue;
							}
							Float weight = static_cast<Float>(influenceWeights[influence]);
							Size slotBase = static_cast<Size>(controlPoint) * 4;
							Size smallestSlot = 0;
							for (Size slot = 1; slot < 4; slot++)
							{
								if (controlPointWeights[slotBase + slot] < controlPointWeights[slotBase + smallestSlot])
								{
									smallestSlot = slot;
								}
							}
							if (weight > controlPointWeights[slotBase + smallestSlot])
							{
								controlPointWeights[slotBase + smallestSlot] = weight;
								controlPointJoints[slotBase + smallestSlot] = boneIndex;
							}
						}
					}
				}

				auto resolvePolygonSurface = [&](Int polygonIndex) -> Uint32
				{
					Int nodeMaterialIndex = 0;
					if (materialElement)
					{
						if (materialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
						{
							nodeMaterialIndex = materialElement->GetIndexArray().GetAt(polygonIndex);
						}
						else if (materialElement->GetIndexArray().GetCount() > 0)
						{
							nodeMaterialIndex = materialElement->GetIndexArray().GetAt(0);
						}
					}
					FbxSurfaceMaterial* polygonMaterial = (meshNode && nodeMaterialIndex >= 0 && nodeMaterialIndex < meshNode->GetMaterialCount()) ? meshNode->GetMaterial(nodeMaterialIndex) : nullptr;
					if (polygonMaterial)
					{
						for (Size surfaceIndex = 0; surfaceIndex < source.fbx_.materialTable_.size(); surfaceIndex++)
						{
							if (source.fbx_.materialTable_[surfaceIndex] == polygonMaterial)
							{
								return static_cast<Uint32>(surfaceIndex);
							}
						}
					}
					if (defaultSurfaceIndex < 0)
					{
						defaultSurfaceIndex = static_cast<Int>(crister.material_.surfaces_.size());
						crister.material_.surfaces_.emplace_back();
					}
					return static_cast<Uint32>(defaultSurfaceIndex);
				};

				Int polygonCount = fbxMesh->GetPolygonCount();

				DynamicArray<Uint32> usedSurfaces;
				for (Int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
				{
					if (fbxMesh->GetPolygonSize(polygonIndex) != 3)
					{
						continue;
					}
					Uint32 polygonSurface = resolvePolygonSurface(polygonIndex);
					Bool surfaceKnown = false;
					for (Uint32 existingSurface : usedSurfaces)
					{
						if (existingSurface == polygonSurface)
						{
							surfaceKnown = true;
							break;
						}
					}
					if (!surfaceKnown)
					{
						usedSurfaces.push_back(polygonSurface);
					}
				}

				DynamicArray<FbxBlendShapeChannel*> blendShapeChannels;
				DynamicArray<FbxVector4*> blendShapeTargetPoints;
				for (Int blendShapeIndex = 0; blendShapeIndex < fbxMesh->GetDeformerCount(FbxDeformer::eBlendShape); blendShapeIndex++)
				{
					FbxBlendShape* blendShape = static_cast<FbxBlendShape*>(fbxMesh->GetDeformer(blendShapeIndex, FbxDeformer::eBlendShape));
					for (Int channelIndex = 0; channelIndex < blendShape->GetBlendShapeChannelCount(); channelIndex++)
					{
						FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(channelIndex);
						Int targetShapeCount = channel->GetTargetShapeCount();
						if (targetShapeCount <= 0)
						{
							continue;
						}
						FbxShape* targetShape = channel->GetTargetShape(targetShapeCount - 1);
						blendShapeChannels.push_back(channel);
						blendShapeTargetPoints.push_back(targetShape->GetControlPoints());
					}
				}

				for (Uint32 surfaceIndex : usedSurfaces)
				{
					SubMesh& subMesh = crister.geometry_.subMeshes_.emplace_back();
					subMesh.meshIndex_ = static_cast<Int>(meshTableIndex);
					subMesh.surfaceIndex_ = surfaceIndex;
					subMesh.skinIndex_ = meshSkinIndex;

					Uint32 vertexOffset = static_cast<Uint32>(crister.geometry_.vertices_.size());
					subMesh.vertexOffset_ = vertexOffset;

					for (FbxBlendShapeChannel* channel : blendShapeChannels)
					{
						Morph& morph = subMesh.morphs_.emplace_back();
						morph.name_ = channel->GetName();
					}

					for (Int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
					{
						if (fbxMesh->GetPolygonSize(polygonIndex) != 3 || resolvePolygonSurface(polygonIndex) != surfaceIndex)
						{
							continue;
						}
						for (Int corner = 0; corner < 3; corner++)
						{
							Int controlPoint = fbxMesh->GetPolygonVertex(polygonIndex, corner);
							Vertex& vertex = crister.geometry_.vertices_.emplace_back();

							FbxVector4 position = controlPoints[controlPoint];
							vertex.position_ = Vector3(static_cast<Float>(position[0]), static_cast<Float>(position[1]), static_cast<Float>(position[2])) * source.fbx_.unitScale_;

							FbxVector4 normal;
							fbxMesh->GetPolygonVertexNormal(polygonIndex, corner, normal);
							vertex.normal_ = Vector3(static_cast<Float>(normal[0]), static_cast<Float>(normal[1]), static_cast<Float>(normal[2]));

							if (uvSetName)
							{
								FbxVector2 uv;
								Bool unmapped = false;
								fbxMesh->GetPolygonVertexUV(polygonIndex, corner, uvSetName, uv, unmapped);
								vertex.texcoord_ = Vector2(static_cast<Float>(uv[0]), 1.0f - static_cast<Float>(uv[1]));
							}

							vertex.tangent_ = Vector4(1.0f, 0.0f, 0.0f, 1.0f);

							Size slotBase = static_cast<Size>(controlPoint) * 4;
							vertex.joints_ = { controlPointJoints[slotBase + 0], controlPointJoints[slotBase + 1], controlPointJoints[slotBase + 2], controlPointJoints[slotBase + 3] };
							vertex.weights_ = Vector4(controlPointWeights[slotBase + 0], controlPointWeights[slotBase + 1], controlPointWeights[slotBase + 2], controlPointWeights[slotBase + 3]);
							if (vertex.weights_.x + vertex.weights_.y + vertex.weights_.z + vertex.weights_.w <= 0.0f)
							{
								vertex.weights_ = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
							}

							crister.geometry_.vertexMorphSource_.push_back(static_cast<Uint32>(crister.geometry_.vertices_.size() - 1));

							for (Size channelIndex = 0; channelIndex < blendShapeChannels.size(); channelIndex++)
							{
								FbxVector4 targetPosition = blendShapeTargetPoints[channelIndex][controlPoint];
								subMesh.morphs_[channelIndex].positionDeltas_.push_back(Vector3(static_cast<Float>(targetPosition[0] - position[0]), static_cast<Float>(targetPosition[1] - position[1]), static_cast<Float>(targetPosition[2] - position[2])) * source.fbx_.unitScale_);
							}
						}
					}

					Uint32 vertexCount = static_cast<Uint32>(crister.geometry_.vertices_.size()) - vertexOffset;
					subMesh.vertexCount_ = vertexCount;

					subMesh.indexOffset_ = static_cast<Uint32>(crister.geometry_.vertexIndices_.size());
					for (Uint32 localIndex = 0; localIndex < vertexCount; localIndex++)
					{
						crister.geometry_.vertexIndices_.push_back(vertexOffset + localIndex);
					}
					subMesh.indexCount_ = vertexCount;
				}
			}
			break;
		}
		}
	}

	void ModelLoader::FetchMaterials(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			for (const tinygltf::Material& gltfMaterial : model.materials)
			{
				Surface& material = crister.material_.surfaces_.emplace_back();

				Size materialIndex = crister.material_.surfaces_.size() - 1;
				material.name_ = gltfMaterial.name.empty() ? ("Material_" + std::to_string(materialIndex)) : gltfMaterial.name;

				const auto& pbr = gltfMaterial.pbrMetallicRoughness;
				material.baseColor_ = Color(static_cast<Float>(pbr.baseColorFactor[0]), static_cast<Float>(pbr.baseColorFactor[1]), static_cast<Float>(pbr.baseColorFactor[2]), static_cast<Float>(pbr.baseColorFactor[3]));
				material.metallic_ = static_cast<Float>(pbr.metallicFactor);
				material.roughness_ = static_cast<Float>(pbr.roughnessFactor);

				material.emissiveFactor_[0] = static_cast<Float>(gltfMaterial.emissiveFactor[0]);
				material.emissiveFactor_[1] = static_cast<Float>(gltfMaterial.emissiveFactor[1]);
				material.emissiveFactor_[2] = static_cast<Float>(gltfMaterial.emissiveFactor[2]);

				material.alphaMode_ = gltfMaterial.alphaMode == "OPAQUE" ? 0 : gltfMaterial.alphaMode == "MASK" ? 1 : 2;
				material.alphaCutoff_ = static_cast<Float>(gltfMaterial.alphaCutoff);
				material.doubleSided_ = gltfMaterial.doubleSided ? 1 : 0;

				if (pbr.baseColorTexture.index >= 0)
				{
					material.baseColorTextureIndex_ = model.textures[pbr.baseColorTexture.index].source;
				}
				if (gltfMaterial.normalTexture.index >= 0)
				{
					material.normalTextureIndex_ = model.textures[gltfMaterial.normalTexture.index].source;
				}
				if (pbr.metallicRoughnessTexture.index >= 0)
				{
					material.metallicRoughnessTextureIndex_ = model.textures[pbr.metallicRoughnessTexture.index].source;
				}
				if (gltfMaterial.occlusionTexture.index >= 0)
				{
					material.occlusionTextureIndex_ = model.textures[gltfMaterial.occlusionTexture.index].source;
				}
				if (gltfMaterial.emissiveTexture.index >= 0)
				{
					material.emissiveTextureIndex_ = model.textures[gltfMaterial.emissiveTexture.index].source;
				}

				auto extFloat = [](const tinygltf::Value& value, const Char* key, Float fallback) -> Float
				{
					return value.Has(key) ? static_cast<Float>(value.Get(key).GetNumberAsDouble()) : fallback;
				};
				auto extColor3 = [](const tinygltf::Value& value, const Char* key, Float* destination)
				{
					if (value.Has(key))
					{
						const tinygltf::Value& array = value.Get(key);
						if (array.IsArray() && array.ArrayLen() >= 3)
						{
							destination[0] = static_cast<Float>(array.Get(0).GetNumberAsDouble());
							destination[1] = static_cast<Float>(array.Get(1).GetNumberAsDouble());
							destination[2] = static_cast<Float>(array.Get(2).GetNumberAsDouble());
						}
					}
				};
				auto extTexture = [&model](const tinygltf::Value& value, const Char* key) -> Uint32
				{
					if (value.Has(key))
					{
						const tinygltf::Value& info = value.Get(key);
						if (info.Has("index"))
						{
							const Int textureIndex = static_cast<Int>(info.Get("index").GetNumberAsDouble());
							if (textureIndex >= 0)
							{
								return static_cast<Uint32>(model.textures[textureIndex].source);
							}
						}
					}
					return 0xFFFFFFFF;
				};

				const auto& extensions = gltfMaterial.extensions;

				if (auto it = extensions.find("KHR_materials_emissive_strength"); it != extensions.end())
				{
					material.khr_.emissiveStrength_.emissiveStrength_ = extFloat(it->second, "emissiveStrength", 1.0f);
				}

				if (auto it = extensions.find("KHR_materials_ior"); it != extensions.end())
				{
					material.khr_.ior_.ior_ = extFloat(it->second, "ior", 1.5f);
				}

				if (auto it = extensions.find("KHR_materials_specular"); it != extensions.end())
				{
					auto& specular = material.khr_.specular_;
					specular.specularFactor_ = extFloat(it->second, "specularFactor", 1.0f);
					extColor3(it->second, "specularColorFactor", specular.specularColorFactor_);
					specular.specularTextureIndex_ = extTexture(it->second, "specularTexture");
					specular.specularColorTextureIndex_ = extTexture(it->second, "specularColorTexture");
				}

				if (auto it = extensions.find("KHR_materials_clearcoat"); it != extensions.end())
				{
					auto& clearCoat = material.khr_.clearCoat_;
					clearCoat.clearCoatFactor_ = extFloat(it->second, "clearcoatFactor", 0.0f);
					clearCoat.clearCoatRoughnessFactor_ = extFloat(it->second, "clearcoatRoughnessFactor", 0.0f);
					clearCoat.clearCoatTextureIndex_ = extTexture(it->second, "clearcoatTexture");
					clearCoat.clearCoatRoughnessTextureIndex_ = extTexture(it->second, "clearcoatRoughnessTexture");
					clearCoat.clearCoatNormalTextureIndex_ = extTexture(it->second, "clearcoatNormalTexture");
				}

				if (auto it = extensions.find("KHR_materials_transmission"); it != extensions.end())
				{
					auto& transmission = material.khr_.transmission_;
					transmission.transmissionFactor_ = extFloat(it->second, "transmissionFactor", 0.0f);
					transmission.transmissionTextureIndex_ = extTexture(it->second, "transmissionTexture");

					if (transmission.transmissionFactor_ > 0.0f)
					{
						material.alphaMode_ = 0;
					}
				}

				if (auto it = extensions.find("KHR_materials_volume"); it != extensions.end())
				{
					auto& volume = material.khr_.volume_;
					volume.thicknessFactor_ = extFloat(it->second, "thicknessFactor", 0.0f);
					volume.attenuationDistance_ = extFloat(it->second, "attenuationDistance", FLT_MAX);
					extColor3(it->second, "attenuationColor", volume.attenuationColor_);
					volume.thicknessTextureIndex_ = extTexture(it->second, "thicknessTexture");
				}

				if (auto it = extensions.find("KHR_materials_sheen"); it != extensions.end())
				{
					auto& sheen = material.khr_.sheen_;
					extColor3(it->second, "sheenColorFactor", sheen.sheenColorFactor_);
					sheen.sheenRoughnessFactor_ = extFloat(it->second, "sheenRoughnessFactor", 0.0f);
					sheen.sheenColorTextureIndex_ = extTexture(it->second, "sheenColorTexture");
					sheen.sheenRoughnessTextureIndex_ = extTexture(it->second, "sheenRoughnessTexture");
				}

				if (auto it = extensions.find("KHR_materials_iridescence"); it != extensions.end())
				{
					auto& iridescence = material.khr_.iridescence_;
					iridescence.iridescenceFactor_ = extFloat(it->second, "iridescenceFactor", 0.0f);
					iridescence.iridescenceIor_ = extFloat(it->second, "iridescenceIor", 1.3f);
					iridescence.iridescenceThicknessMinimum_ = extFloat(it->second, "iridescenceThicknessMinimum", 100.0f);
					iridescence.iridescenceThicknessMaximum_ = extFloat(it->second, "iridescenceThicknessMaximum", 400.0f);
					iridescence.iridescenceTextureIndex_ = extTexture(it->second, "iridescenceTexture");
					iridescence.iridescenceThicknessTextureIndex_ = extTexture(it->second, "iridescenceThicknessTexture");
				}

				if (auto it = extensions.find("KHR_materials_anisotropy"); it != extensions.end())
				{
					auto& anisotropy = material.khr_.anisotropy_;
					anisotropy.anisotropyStrength_ = extFloat(it->second, "anisotropyStrength", 0.0f);
					anisotropy.anisotropyRotation_ = extFloat(it->second, "anisotropyRotation", 0.0f);
					anisotropy.anisotropyTextureIndex_ = extTexture(it->second, "anisotropyTexture");
				}

				if (extensions.count("KHR_materials_unlit"))
				{
					material.khr_.unlit_.unlit_ = 1;
				}
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			auto resolveTexture = [&source](FbxProperty property) -> Uint32
			{
				if (!property.IsValid())
				{
					return 0xFFFFFFFF;
				}
				FbxFileTexture* fileTexture = property.GetSrcObject<FbxFileTexture>(0);
				if (!fileTexture)
				{
					FbxLayeredTexture* layeredTexture = property.GetSrcObject<FbxLayeredTexture>(0);
					if (layeredTexture)
					{
						fileTexture = layeredTexture->GetSrcObject<FbxFileTexture>(0);
					}
				}
				if (!fileTexture)
				{
					return 0xFFFFFFFF;
				}
				for (Size textureIndex = 0; textureIndex < source.fbx_.textureTable_.size(); textureIndex++)
				{
					if (source.fbx_.textureTable_[textureIndex] == fileTexture)
					{
						return static_cast<Uint32>(textureIndex);
					}
				}
				source.fbx_.textureTable_.push_back(fileTexture);
				return static_cast<Uint32>(source.fbx_.textureTable_.size() - 1);
			};

			for (FbxNode* fbxNode : source.fbx_.nodeTable_)
			{
				for (Int nodeMaterialIndex = 0; nodeMaterialIndex < fbxNode->GetMaterialCount(); nodeMaterialIndex++)
				{
					FbxSurfaceMaterial* fbxMaterial = fbxNode->GetMaterial(nodeMaterialIndex);
					Bool materialKnown = false;
					for (FbxSurfaceMaterial* existingMaterial : source.fbx_.materialTable_)
					{
						if (existingMaterial == fbxMaterial)
						{
							materialKnown = true;
							break;
						}
					}
					if (materialKnown)
					{
						continue;
					}
					source.fbx_.materialTable_.push_back(fbxMaterial);

					Surface& material = crister.material_.surfaces_.emplace_back();
					std::string materialName = fbxMaterial->GetName();
					Size namespaceEnd = materialName.find_last_of(':');
					material.name_ = (namespaceEnd == std::string::npos) ? materialName : materialName.substr(namespaceEnd + 1);

					FbxProperty diffuseProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);
					if (diffuseProperty.IsValid())
					{
						FbxDouble3 diffuse = diffuseProperty.Get<FbxDouble3>();
						material.baseColor_ = Color(static_cast<Float>(diffuse[0]), static_cast<Float>(diffuse[1]), static_cast<Float>(diffuse[2]), 1.0f);
					}
					else
					{
						diffuseProperty = fbxMaterial->FindPropertyHierarchical("Maya|baseColor");
						if (diffuseProperty.IsValid())
						{
							FbxDouble3 baseColor = diffuseProperty.Get<FbxDouble3>();
							FbxProperty baseWeightProperty = fbxMaterial->FindPropertyHierarchical("Maya|base");
							Float baseWeight = baseWeightProperty.IsValid() ? static_cast<Float>(baseWeightProperty.Get<FbxDouble>()) : 1.0f;
							material.baseColor_ = Color(static_cast<Float>(baseColor[0]) * baseWeight, static_cast<Float>(baseColor[1]) * baseWeight, static_cast<Float>(baseColor[2]) * baseWeight, 1.0f);
						}
					}

					material.baseColorTextureIndex_ = resolveTexture(diffuseProperty);
					if (material.baseColorTextureIndex_ == 0xFFFFFFFF)
					{
						material.baseColorTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|baseColor"));
					}
					if (material.baseColorTextureIndex_ == 0xFFFFFFFF)
					{
						material.baseColorTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|TEX_color_map"));
					}
					if (material.baseColorTextureIndex_ != 0xFFFFFFFF)
					{
						material.baseColor_ = Color(1.0f, 1.0f, 1.0f, 1.0f);
					}

					FbxProperty emissiveProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sEmissive);
					if (emissiveProperty.IsValid())
					{
						FbxDouble3 emissive = emissiveProperty.Get<FbxDouble3>();
						material.emissiveFactor_[0] = static_cast<Float>(emissive[0]);
						material.emissiveFactor_[1] = static_cast<Float>(emissive[1]);
						material.emissiveFactor_[2] = static_cast<Float>(emissive[2]);
					}

					material.metallic_ = 0.0f;
					material.roughness_ = 1.0f;
					if (fbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
					{
						FbxSurfacePhong* phong = static_cast<FbxSurfacePhong*>(fbxMaterial);
						material.roughness_ = Clamp(1.0f - static_cast<Float>(phong->Shininess.Get()) / 100.0f, 0.04f, 1.0f);
					}

					FbxProperty transparentColorProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sTransparentColor);
					FbxProperty transparencyFactorProperty = fbxMaterial->FindProperty(FbxSurfaceMaterial::sTransparencyFactor);
					if (transparentColorProperty.IsValid())
					{
						FbxDouble3 transparentColor = transparentColorProperty.Get<FbxDouble3>();
						Double transparencyFactor = transparencyFactorProperty.IsValid() ? transparencyFactorProperty.Get<FbxDouble>() : 1.0;
						Double transparency = (transparentColor[0] + transparentColor[1] + transparentColor[2]) / 3.0 * transparencyFactor;
						Float alpha = 1.0f - Clamp(static_cast<Float>(transparency), 0.0f, 1.0f);
						material.baseColor_.a = alpha;
						if (alpha < 0.999f)
						{
							material.alphaMode_ = 2;
						}
					}

					material.normalTextureIndex_ = resolveTexture(fbxMaterial->FindProperty(FbxSurfaceMaterial::sNormalMap));
					if (material.normalTextureIndex_ == 0xFFFFFFFF)
					{
						material.normalTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|normalCamera"));
					}
					if (material.normalTextureIndex_ == 0xFFFFFFFF)
					{
						material.normalTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|TEX_normal_map"));
					}
					if (material.normalTextureIndex_ == 0xFFFFFFFF)
					{
						material.normalTextureIndex_ = resolveTexture(fbxMaterial->FindProperty(FbxSurfaceMaterial::sBump));
					}

					material.emissiveTextureIndex_ = resolveTexture(emissiveProperty);
					if (material.emissiveTextureIndex_ == 0xFFFFFFFF)
					{
						material.emissiveTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|emissionColor"));
					}
					if (material.emissiveTextureIndex_ == 0xFFFFFFFF)
					{
						material.emissiveTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|TEX_emissive_map"));
					}

					material.occlusionTextureIndex_ = resolveTexture(fbxMaterial->FindPropertyHierarchical("Maya|TEX_ao_map"));
				}
			}
			break;
		}
		}
	}

	void ModelLoader::FetchSkins(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			for (const tinygltf::Skin& gltfSkin : model.skins)
			{
				Skin& skin = crister.scene_.skins_.emplace_back();
				skin.joints_ = gltfSkin.joints;

				if (gltfSkin.inverseBindMatrices >= 0)
				{
					const tinygltf::Accessor& accessor = model.accessors.at(gltfSkin.inverseBindMatrices);
					const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
					skin.inverseBindMatrices_.resize(accessor.count);

					memcpy(skin.inverseBindMatrices_.data(), model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset, accessor.count * sizeof(Matrix));
				}
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			for (FbxMesh* fbxMesh : source.fbx_.meshTable_)
			{
				if (fbxMesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
				{
					continue;
				}
				FbxSkin* fbxSkin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::eSkin));

				Skin& skin = crister.scene_.skins_.emplace_back();
				for (Int clusterIndex = 0; clusterIndex < fbxSkin->GetClusterCount(); clusterIndex++)
				{
					FbxCluster* cluster = fbxSkin->GetCluster(clusterIndex);
					FbxNode* boneNode = cluster->GetLink();
					if (!boneNode)
					{
						continue;
					}

					Int boneIndex = -1;
					for (Size searchIndex = 0; searchIndex < source.fbx_.nodeTable_.size(); searchIndex++)
					{
						if (source.fbx_.nodeTable_[searchIndex] == boneNode)
						{
							boneIndex = static_cast<Int>(searchIndex);
							break;
						}
					}
					skin.joints_.push_back(boneIndex);

					FbxAMatrix meshBindMatrix;
					FbxAMatrix boneBindMatrix;
					cluster->GetTransformMatrix(meshBindMatrix);
					cluster->GetTransformLinkMatrix(boneBindMatrix);
					FbxAMatrix inverseBindMatrix = boneBindMatrix.Inverse() * meshBindMatrix;

					Matrix& engineMatrix = skin.inverseBindMatrices_.emplace_back();
					for (Int row = 0; row < 4; row++)
					{
						for (Int column = 0; column < 4; column++)
						{
							engineMatrix.m[row][column] = static_cast<Float>(inverseBindMatrix.Get(row, column));
						}
					}
					engineMatrix.m[3][0] *= source.fbx_.unitScale_;
					engineMatrix.m[3][1] *= source.fbx_.unitScale_;
					engineMatrix.m[3][2] *= source.fbx_.unitScale_;
				}
			}
			break;
		}
		}
	}

	void ModelLoader::ResolveSubMeshSkins(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			DynamicArray<Uint32> subMeshOffsets;
			subMeshOffsets.reserve(model.meshes.size());
			Uint32 offset = 0;
			for (const tinygltf::Mesh& gltfMesh : model.meshes)
			{
				subMeshOffsets.push_back(offset);
				offset += static_cast<Uint32>(gltfMesh.primitives.size());
			}

			for (const tinygltf::Node& gltfNode : model.nodes)
			{
				if (gltfNode.mesh < 0 || gltfNode.skin < 0)
				{
					continue;
				}

				Uint32 begin = subMeshOffsets[gltfNode.mesh];
				Uint32 count = static_cast<Uint32>(model.meshes[gltfNode.mesh].primitives.size());
				for (Uint32 index = 0; index < count; index++)
				{
					crister.geometry_.subMeshes_[begin + index].skinIndex_ = gltfNode.skin;
				}
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			break;
		}
		}
	}

	void ModelLoader::FetchTexture(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			for (const tinygltf::Image& gltfImage : model.images)
			{
				Bitmap& texture = crister.material_.bitmaps_.emplace_back();
				texture.name_ = gltfImage.name;
				texture.width_ = gltfImage.width;
				texture.height_ = gltfImage.height;
				texture.component_ = gltfImage.component;
				texture.bits_ = gltfImage.bits;
				texture.mimeType_ = gltfImage.mimeType;
				texture.cacheData_ = gltfImage.image;
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			for (FbxFileTexture* fileTexture : source.fbx_.textureTable_)
			{
				Bitmap& texture = crister.material_.bitmaps_.emplace_back();
				texture.name_ = fileTexture->GetName();
				texture.component_ = 4;
				texture.bits_ = 8;

				std::filesystem::path fbxDirectory(source.fbx_.directory_);
				std::string baseName = std::filesystem::path(fileTexture->GetFileName()).filename().string();
				if (baseName.empty())
				{
					baseName = std::filesystem::path(fileTexture->GetRelativeFileName()).filename().string();
				}

				DynamicArray<std::filesystem::path> candidatePaths;
				candidatePaths.push_back(fbxDirectory / (source.fbx_.stem_ + ".fbm") / baseName);
				candidatePaths.push_back(fbxDirectory / baseName);
				candidatePaths.push_back(fbxDirectory / fileTexture->GetRelativeFileName());
				candidatePaths.push_back(std::filesystem::path(fileTexture->GetFileName()));

				std::filesystem::path nonTexturePath("../Editor/Icon/Warning/NonTexture.icon");

				std::filesystem::path texturePath;
				for (const std::filesystem::path& candidatePath : candidatePaths)
				{
					if (std::filesystem::exists(candidatePath))
					{
						texturePath = candidatePath;
						break;
					}
				}
				if (texturePath.empty())
				{
					SC_LOG_WARNING("FBX のテクスチャ {} が見つかりません({})。NonTexture で代用します。", texture.name_, fileTexture->GetFileName());
					texturePath = nonTexturePath;
				}

				DynamicArray<std::filesystem::path> loadPaths = { texturePath };
				if (texturePath != nonTexturePath)
				{
					loadPaths.push_back(nonTexturePath);
				}

				DirectX::ScratchImage loadedImage;
				DirectX::ScratchImage rgbaImage;
				const DirectX::Image* image = nullptr;
				for (const std::filesystem::path& loadPath : loadPaths)
				{
					std::string extension = loadPath.extension().string();
					std::ranges::transform(extension, extension.begin(), [](Uchar character) { return static_cast<Char>(std::tolower(character)); });

					DirectX::TexMetadata loadedMetadata{};
					HRESULT loadResult{ S_OK };
					if (extension == ".icon")
					{
						loadResult = E_FAIL;
						BinaryInputArchive iconArchive;
						if (iconArchive.Read(String(loadPath.string())))
						{
							DynamicArray<Byte> iconData;
							iconArchive.TryField("data", iconData);
							if (!iconData.empty())
							{
								loadResult = DirectX::LoadFromDDSMemory(reinterpret_cast<const Uint8*>(iconData.data()), iconData.size(), DirectX::DDS_FLAGS_NONE, &loadedMetadata, loadedImage);
							}
						}
					}
					else if (extension == ".tga")
					{
						loadResult = DirectX::LoadFromTGAFile(loadPath.wstring().c_str(), &loadedMetadata, loadedImage);
					}
					else if (extension == ".dds")
					{
						loadResult = DirectX::LoadFromDDSFile(loadPath.wstring().c_str(), DirectX::DDS_FLAGS_NONE, &loadedMetadata, loadedImage);
					}
					else
					{
						loadResult = DirectX::LoadFromWICFile(loadPath.wstring().c_str(), DirectX::WIC_FLAGS_NONE, &loadedMetadata, loadedImage);
					}

					if (SUCCEEDED(loadResult))
					{
						if (loadedMetadata.format == DXGI_FORMAT_R8G8B8A8_UNORM)
						{
							image = loadedImage.GetImage(0, 0, 0);
						}
						else if (DirectX::IsCompressed(loadedMetadata.format))
						{
							if (SUCCEEDED(DirectX::Decompress(*loadedImage.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, rgbaImage)))
							{
								image = rgbaImage.GetImage(0, 0, 0);
							}
						}
						else if (SUCCEEDED(DirectX::Convert(*loadedImage.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgbaImage)))
						{
							image = rgbaImage.GetImage(0, 0, 0);
						}
					}

					if (image)
					{
						break;
					}
					SC_LOG_WARNING("FBX のテクスチャ {} を読み込めませんでした({})。", texture.name_, loadPath.string());
				}

				if (image)
				{
					texture.width_ = static_cast<Int>(image->width);
					texture.height_ = static_cast<Int>(image->height);
					texture.mimeType_ = "image/png";
					texture.cacheData_.resize(image->width * image->height * 4);
					for (Size row = 0; row < image->height; row++)
					{
						memcpy(texture.cacheData_.data() + row * image->width * 4, image->pixels + row * image->rowPitch, image->width * 4);
					}
				}
				else
				{
					texture.width_ = 1;
					texture.height_ = 1;
					texture.cacheData_.assign(4, static_cast<Uchar>(255));
				}
			}
			break;
		}
		}
	}

	void ModelLoader::ConvertSceneConvention(fbxsdk::FbxScene* scene, fbxsdk::FbxManager* manager)
	{
		FbxAxisSystem gltfAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		gltfAxisSystem.DeepConvertScene(scene);

		FbxGeometryConverter geometryConverter(manager);
		geometryConverter.Triangulate(scene, true);
	}

	void ModelLoader::ConvertFlatConvention(ModelSource& source)
	{
		DynamicArray<FbxNode*> nodeStack;
		FbxNode* fbxRoot = source.fbx_.scene_->GetRootNode();
		for (Int childIndex = fbxRoot->GetChildCount() - 1; childIndex >= 0; childIndex--)
		{
			nodeStack.push_back(fbxRoot->GetChild(childIndex));
		}

		while (!nodeStack.empty())
		{
			FbxNode* fbxNode = nodeStack.back();
			nodeStack.pop_back();
			source.fbx_.nodeTable_.push_back(fbxNode);

			FbxMesh* fbxMesh = fbxNode->GetMesh();
			if (fbxMesh)
			{
				Bool meshKnown = false;
				for (FbxMesh* existingMesh : source.fbx_.meshTable_)
				{
					if (existingMesh == fbxMesh)
					{
						meshKnown = true;
						break;
					}
				}
				if (!meshKnown)
				{
					source.fbx_.meshTable_.push_back(fbxMesh);
				}
			}

			for (Int childIndex = fbxNode->GetChildCount() - 1; childIndex >= 0; childIndex--)
			{
				nodeStack.push_back(fbxNode->GetChild(childIndex));
			}
		}
	}

	void ModelLoader::FetchLights(const ModelSource& source, Crister& crister)
	{
		switch (source.format_)
		{
		case ModelFormat::Gltf:
		{
			const tinygltf::Model& model = *source.gltf_.model_;

			for (const Node& node : crister.scene_.nodes_)
			{
				if (node.light_ < 0 || node.light_ >= static_cast<Int>(model.lights.size()))
				{
					continue;
				}

				const tinygltf::Light& gltfLight = model.lights[node.light_];

				if (gltfLight.type == "directional")
				{
					continue;
				}

				PunctualLight light{};
				light.type_ = (gltfLight.type == "spot") ? PunctualLight::Type::Spot : PunctualLight::Type::Point;

				const Matrix& worldTransform = node.globalTransform_;
				light.position_ = worldTransform.Translation();

				Vector3 worldForward(worldTransform._31, worldTransform._32, worldTransform._33);
				worldForward.Normalize();
				light.direction_ = -worldForward;

				if (gltfLight.color.size() == 3)
				{
					light.colorRGB_[0] = static_cast<Float>(gltfLight.color[0]);
					light.colorRGB_[1] = static_cast<Float>(gltfLight.color[1]);
					light.colorRGB_[2] = static_cast<Float>(gltfLight.color[2]);
				}

				light.intensity_ = static_cast<Float>(gltfLight.intensity);

				constexpr Float unboundedRangeFallback = 10.0f;
				Float range = static_cast<Float>(gltfLight.range);
				light.range_ = (range > 0.0f) ? range : unboundedRangeFallback;

				if (light.type_ == PunctualLight::Type::Spot)
				{
					light.innerConeAngle_ = static_cast<Float>(gltfLight.spot.innerConeAngle);
					light.outerConeAngle_ = static_cast<Float>(gltfLight.spot.outerConeAngle);
				}

				crister.scene_.lights_.push_back(light);
			}
			break;
		}
		case ModelFormat::Fbx:
		{
			for (Size nodeIndex = 0; nodeIndex < source.fbx_.nodeTable_.size(); nodeIndex++)
			{
				FbxLight* fbxLight = source.fbx_.nodeTable_[nodeIndex]->GetLight();
				if (!fbxLight)
				{
					continue;
				}
				FbxLight::EType lightType = fbxLight->LightType.Get();
				if (lightType != FbxLight::ePoint && lightType != FbxLight::eSpot)
				{
					continue;
				}

				PunctualLight light{};
				light.type_ = (lightType == FbxLight::eSpot) ? PunctualLight::Type::Spot : PunctualLight::Type::Point;

				const Matrix& worldTransform = crister.scene_.nodes_[nodeIndex].globalTransform_;
				light.position_ = worldTransform.Translation();

				Vector3 worldForward(worldTransform._31, worldTransform._32, worldTransform._33);
				worldForward.Normalize();
				light.direction_ = -worldForward;

				FbxDouble3 color = fbxLight->Color.Get();
				light.colorRGB_[0] = static_cast<Float>(color[0]);
				light.colorRGB_[1] = static_cast<Float>(color[1]);
				light.colorRGB_[2] = static_cast<Float>(color[2]);

				light.intensity_ = static_cast<Float>(fbxLight->Intensity.Get()) / 100.0f;

				constexpr Float unboundedRangeFallback = 10.0f;
				Float range = fbxLight->EnableFarAttenuation.Get() ? static_cast<Float>(fbxLight->FarAttenuationEnd.Get()) : 0.0f;
				light.range_ = (range > 0.0f) ? range : unboundedRangeFallback;

				if (light.type_ == PunctualLight::Type::Spot)
				{
					constexpr Float degreesToRadians = 3.14159265358979323846f / 180.0f;
					light.innerConeAngle_ = static_cast<Float>(fbxLight->InnerAngle.Get()) * 0.5f * degreesToRadians;
					light.outerConeAngle_ = static_cast<Float>(fbxLight->OuterAngle.Get()) * 0.5f * degreesToRadians;
				}

				crister.scene_.lights_.push_back(light);
			}
			break;
		}
		}
	}

}

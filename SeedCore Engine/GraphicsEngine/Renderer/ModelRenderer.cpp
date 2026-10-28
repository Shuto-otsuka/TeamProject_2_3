#include <GraphicsEngine/Renderer/ModelRenderer.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/Model/Crister.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12Check.h>
#include <GraphicsEngine/D3D12/Buffer/FrameBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/GeometryBuffer.h>
#include <GraphicsEngine/System/IndicesSystem.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <PhysicsEngine/Softbody/Softbody.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	ModelRenderer::ModelRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject) : modelShader_(rootSignature, pipelineStateObject)
	{
		/// No Code
	}

	void ModelRenderer::Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		device_ = device;
		bindlessHeap_ = bindlessHeap;
		constantIndicesSystem_ = &constantIndicesSystem;
		shaderResourceIndicesSystem_ = &shaderResourceIndicesSystem;
		maxInstanceCount_ = 65536;
		maxBoneCount_ = 65536;
		maxMorphWeightCount_ = 65536;

		modelShader_.Create(shaderCache, device);

		instanceBuffer_ = MakePtr<ReadOnlyStructuredBuffer<ModelStructuredBuffer>>(device, bindlessHeap, maxInstanceCount_);
		shaderResourceIndicesSystem.SetModelInstanceIndex(instanceBuffer_->Index());

		boneBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Matrix>>(device, bindlessHeap, maxBoneCount_);
		shaderResourceIndicesSystem.SetModelBoneMatrixIndex(boneBuffer_->Index());

		previousBoneBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Matrix>>(device, bindlessHeap, maxBoneCount_);
		shaderResourceIndicesSystem.SetModelPreviousBoneMatrixIndex(previousBoneBuffer_->Index());

		morphWeightBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Float>>(device, bindlessHeap, maxMorphWeightCount_);
		shaderResourceIndicesSystem.SetModelMorphWeightIndex(morphWeightBuffer_->Index());

		previousMorphWeightBuffer_ = MakePtr<ReadOnlyStructuredBuffer<Float>>(device, bindlessHeap, maxMorphWeightCount_);
		shaderResourceIndicesSystem.SetModelPreviousMorphWeightIndex(previousMorphWeightBuffer_->Index());

		modelFurConstantBuffer_ = MakePtr<StaticConstantBuffer<FurConstantBuffer>>(device, bindlessHeap);
		constantIndicesSystem.SetModelFurIndex(modelFurConstantBuffer_->Index());

		oitBuffer_.Create(device, bindlessHeap, constantIndicesSystem, unorderedAccessIndicesSystem, width, height);

		if (D3D12Check::GetLevel() != D3D12Level::D12_2)
		{
			modelCullingBuffer_.Create(device, bindlessHeap);
		}
	}

	void ModelRenderer::Resize(ID3D12Device* device, BindlessHeap* bindlessHeap, ConstantIndicesSystem& constantIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height)
	{
		oitBuffer_.Resize(device, bindlessHeap, constantIndicesSystem, unorderedAccessIndicesSystem, width, height);
	}

	void ModelRenderer::Gather(GraphicsResourceSystem& resources, World& world, const SceneConstantBuffer& scene, std::span<const Entity> selectedEntities)
	{
		ModelResource& modelResource = *resources.modelResource_;
		AnimationResource& animationResource = *resources.animationResource_;
		TextureResource& textureResource = *resources.textureResource_;

		streamingFrame_++;
		geometryStreamingRequests_.clear();

		opaqueInstances_.clear();
		transparentInstances_.clear();
		furInstances_.clear();
		boneMatrices_.clear();
		previousBoneMatrices_.clear();
		animatedBoneOffsets_.clear();
		previousAnimatedBonePalettes_.swap(animatedBonePalettes_);
		animatedBonePalettes_.clear();
		previousAnimatedMorphWeights_.swap(animatedMorphWeights_);
		animatedMorphWeights_.clear();
		previousAnimatorHistories_.swap(animatorHistories_);
		animatorHistories_.clear();
		morphWeights_.clear();
		previousMorphWeights_.clear();
		hasSkinnedOpaque_ = false;
		hasSkinnedTransparent_ = false;
		hasSelectedInstance_ = false;
		hasSelectedSkinned_ = false;
		uploaded_ = false;

		std::unordered_map<const Crister*, Uint> cristerBoneBase;

		const Surface emptySurface{};

		ComponentID boundsComponentID = ComponentRegistry::GetComponentID<Bounds>();

		Query<Read<Active>, Read<Mesh>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Mesh& mesh)
			{
				if (!active.active_)
				{
					return;
				}

				Handle<Crister> cristerHandle = modelResource.GetHandle(mesh.meshID_);
				if (cristerHandle.empty())
				{
					return;
				}

				Crister* crister = modelResource.Resolve(cristerHandle);
				if (!crister)
				{
					return;
				}

				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				if (boundsComponentID)
				{
					void* boundsRaw = actor.GetComponent(boundsComponentID);
					if (boundsRaw)
					{
						Bounds* bounds = static_cast<Bounds*>(boundsRaw);
						Vector3 positionExtent = crister->PlacedPositionExtent();
						bounds->center_ = crister->PlacedPositionMin() + positionExtent * 0.5f;
						bounds->extent_ = positionExtent * 0.5f;
					}
				}

				Softbody* softbody = actor.GetComponent<Softbody>();
				if (softbody && softbody->BodyID() != ScInvalidBodyID)
				{
					return;
				}

				Matrix worldMatrix = actor.WorldMatrix();

				Matrix previousWorldMatrix = worldMatrix;
				auto previousWorldIt = previousWorldMatrices_.find(entityID);
				if (previousWorldIt != previousWorldMatrices_.end())
				{
					previousWorldMatrix = previousWorldIt->second;
					previousWorldIt->second = worldMatrix;
				}
				else
				{
					previousWorldMatrices_.emplace(entityID, worldMatrix);
				}

				const auto& subMeshes = crister->SubMeshes();
				const auto& clusters = crister->Clusters();
				const auto& skins = crister->Skins();
				const auto& nodes = crister->Nodes();

				const Material* materialComponent = actor.GetComponent<Material>();
				const DynamicArray<Surface>& surfaces = (materialComponent && materialComponent->resolvedSurfaces_.size() == crister->Surfaces().size()) ? materialComponent->resolvedSurfaces_ : crister->Surfaces();

				const Skeleton* skeleton = actor.GetComponent<Skeleton>();
				const Bool hasPose = skeleton && skeleton->Animated() && skeleton->GlobalTransforms().size() == nodes.size();

				Animator* animator = actor.GetComponent<Animator>();
				Bool skinHistoryValid = true;
				if (animator)
				{
					AnimatorHistory& animatorHistory = animatorHistories_[entityID];
					animatorHistory.stateIndex_ = animator->CurrentStateIndex();
					animatorHistory.time_ = animator->CurrentTime();

					auto previousAnimatorHistoryIt = previousAnimatorHistories_.find(entityID);
					if (previousAnimatorHistoryIt != previousAnimatorHistories_.end())
					{
						const AnimatorHistory& previousAnimatorHistory = previousAnimatorHistoryIt->second;
						Bool poseJumped = previousAnimatorHistory.stateIndex_ != animatorHistory.stateIndex_ || previousAnimatorHistory.time_ > animatorHistory.time_;
						Bool crossFading = animator->Blending() && animator->blendDuration_ > 0.0f;
						if (poseJumped && !crossFading)
						{
							skinHistoryValid = false;
						}
					}
				}

				if (std::ranges::any_of(crister->SubMeshes(), [](const SubMesh& subMesh) { return !subMesh.morphs_.empty(); }))
				{
					if (animator)
					{
						Int stateIndex = animator->CurrentStateIndex();
						if (stateIndex >= 0 && static_cast<Size>(stateIndex) < animator->states_.size())
						{
							Uint32 animationAssetId = static_cast<Uint32>(animator->states_[stateIndex].animationID_);
							if (animationAssetId != 0)
							{
								Handle<Animation> animationHandle = animationResource.GetHandle(animationAssetId);
								Animation* animation = animationHandle.empty() ? nullptr : animationResource.Resolve(animationHandle);
								if (animation)
								{
									Float duration = animation->Duration();
									Float sampleTime = animator->CurrentTime();
									if (duration > 0.0f)
									{
										sampleTime = std::fmod(sampleTime, duration);
									}
									animator->UpdateCurrentClipDuration(duration);
									animation->SampleMorphWeights(sampleTime, animatedMorphWeights_[entityID]);
								}
							}
						}
					}
				}

				Uint boneBase = 0;
				Bool boneOverflow = false;
				if (!skins.empty())
				{
					if (hasPose)
					{
						Size totalJoints = 0;
						for (const Skin& skin : skins)
						{
							totalJoints += skin.joints_.size();
						}

						if (boneMatrices_.size() + totalJoints > maxBoneCount_)
						{
							boneOverflow = true;
						}
						else
						{
							boneBase = static_cast<Uint>(boneMatrices_.size());
							for (const Skin& skin : skins)
							{
								for (Size joint = 0; joint < skin.joints_.size(); joint++)
								{
									const Matrix& globalTransform = skeleton->GlobalTransforms()[skin.joints_[joint]];

									if (joint < skin.inverseBindMatrices_.size())
									{
										boneMatrices_.push_back(skin.inverseBindMatrices_[joint] * globalTransform);
									}
									else
									{
										boneMatrices_.push_back(globalTransform);
									}
								}
							}

							DynamicArray<Matrix>& bonePalette = animatedBonePalettes_[entityID];
							bonePalette.assign(boneMatrices_.begin() + boneBase, boneMatrices_.end());

							auto previousBonePaletteIt = previousAnimatedBonePalettes_.find(entityID);
							if (skinHistoryValid && previousBonePaletteIt != previousAnimatedBonePalettes_.end() && previousBonePaletteIt->second.size() == bonePalette.size())
							{
								previousBoneMatrices_.insert(previousBoneMatrices_.end(), previousBonePaletteIt->second.begin(), previousBonePaletteIt->second.end());
							}
							else
							{
								previousBoneMatrices_.insert(previousBoneMatrices_.end(), bonePalette.begin(), bonePalette.end());
							}
						}
					}
					else
					{
						auto found = cristerBoneBase.find(crister);
						if (found != cristerBoneBase.end())
						{
							boneBase = found->second;
						}
						else
						{
							Size totalJoints = 0;
							for (const Skin& skin : skins)
							{
								totalJoints += skin.joints_.size();
							}

							if (boneMatrices_.size() + totalJoints > maxBoneCount_)
							{
								boneOverflow = true;
							}
							else
							{
								boneBase = static_cast<Uint>(boneMatrices_.size());
								for (const Skin& skin : skins)
								{
									for (Size joint = 0; joint < skin.joints_.size(); joint++)
									{
										const Matrix& globalTransform = nodes[skin.joints_[joint]].globalTransform_;

										/// [EN] glTF allows omitting inverseBindMatrices (implies identity).
										/// [JP] glTF は inverseBindMatrices の省略を許容する（単位行列扱い）。
										if (joint < skin.inverseBindMatrices_.size())
										{
											boneMatrices_.push_back(skin.inverseBindMatrices_[joint] * globalTransform);
										}
										else
										{
											boneMatrices_.push_back(globalTransform);
										}
									}
								}
								previousBoneMatrices_.insert(previousBoneMatrices_.end(), boneMatrices_.begin() + boneBase, boneMatrices_.end());
								cristerBoneBase[crister] = boneBase;
							}
						}
					}

					if (hasPose && !boneOverflow)
					{
						animatedBoneOffsets_[entityID] = static_cast<Uint32>(boneBase);
					}
				}

				Bool selected = std::ranges::find(selectedEntities, ActorAccessor::GetEntity(actor)) != selectedEntities.end();

				for (Size subMeshIndex = 0; subMeshIndex < subMeshes.size(); subMeshIndex++)
				{
					const SubMesh& subMesh = subMeshes[subMeshIndex];

					const Surface& material = subMesh.surfaceIndex_ < surfaces.size() ? surfaces[subMesh.surfaceIndex_] : emptySurface;

					Bool skinned = subMesh.skinIndex_ >= 0 && subMesh.skinIndex_ < static_cast<Int>(skins.size()) && !boneOverflow;

					if (subMesh.clusterCount_ == 0)
					{
						continue;
					}

					Matrix lodWorldMatrix = crister->SubMeshPlacement(subMeshIndex).front() * worldMatrix;
					Float worldScale = Max(Max(Vector3(lodWorldMatrix._11, lodWorldMatrix._12, lodWorldMatrix._13).Length(), Vector3(lodWorldMatrix._21, lodWorldMatrix._22, lodWorldMatrix._23).Length()), Vector3(lodWorldMatrix._31, lodWorldMatrix._32, lodWorldMatrix._33).Length());
					Vector3 instancePosition(lodWorldMatrix._41, lodWorldMatrix._42, lodWorldMatrix._43);
					Float viewDistance = Max((instancePosition - Vector3(scene.cameraPosition_.x, scene.cameraPosition_.y, scene.cameraPosition_.z)).Length(), 0.0001f);
					Float pixelsPerUnit = scene.projection_._22 * scene.screenSize_.y * 0.5f / viewDistance;

					Vector3 boundsCenter = Vector3::Transform(crister->PositionMin() + crister->PositionExtent() * 0.5f, lodWorldMatrix);
					Float boundsRadius = crister->PositionExtent().Length() * 0.5f * worldScale;
					Float textureViewDistance = Max((boundsCenter - Vector3(scene.cameraPosition_.x, scene.cameraPosition_.y, scene.cameraPosition_.z)).Length() - boundsRadius, Max(scene.nearPlane_, 0.0001f));
					Float texturePixelsPerUnit = scene.projection_._22 * scene.screenSize_.y * 0.5f / textureViewDistance;

					auto requestTextureMip = [&](Uint32 materialTextureIndex)
					{
						if (materialTextureIndex == 0xFFFFFFFF)
						{
							return;
						}
						Uint32 desiredMip = crister->TextureDesiredMip(materialTextureIndex, subMesh.texcoordDensity_, worldScale, texturePixelsPerUnit);
						if (crister->TextureFinestMip(materialTextureIndex) > desiredMip)
						{
							textureStreamingRequests_.push_back({ crister, materialTextureIndex, desiredMip });
						}
						crister->TouchTexture(materialTextureIndex, streamingFrame_);
					};
					requestTextureMip(material.baseColorTextureAssetID_ != 0 ? SC_INVALID : material.baseColorTextureIndex_);
					requestTextureMip(material.normalTextureAssetID_ != 0 ? SC_INVALID : material.normalTextureIndex_);
					requestTextureMip(material.metallicRoughnessTextureAssetID_ != 0 ? SC_INVALID : material.metallicRoughnessTextureIndex_);
					requestTextureMip(material.emissiveTextureAssetID_ != 0 ? SC_INVALID : material.emissiveTextureIndex_);

					Uint32 selectedCluster = 0xFFFFFFFF;
					if (skinned)
					{
						/// [JP] スキンドは LOD 0 固定（ロード時にピン留め済み）。
						selectedCluster = subMesh.clusterOffset_;
					}
					else
					{
						Uint32 desired = 0;
						for (Uint32 c = 0; c < subMesh.clusterCount_; ++c)
						{
							if (clusters[subMesh.clusterOffset_ + c].lodError_ * worldScale * pixelsPerUnit <= 1.0f)
							{
								desired = c;
							}
						}
						if (!crister->ClusterResident(subMesh.clusterOffset_ + desired))
						{
							geometryStreamingRequests_.push_back({ crister, subMesh.clusterOffset_ + desired });
						}

						for (Uint32 c = 0; c < subMesh.clusterCount_; ++c)
						{
							Uint32 clusterIndex = subMesh.clusterOffset_ + c;
							if (!crister->ClusterResident(clusterIndex))
							{
								continue;
							}
							if (selectedCluster == 0xFFFFFFFF || clusters[clusterIndex].lodError_ * worldScale * pixelsPerUnit <= 1.0f)
							{
								selectedCluster = clusterIndex;
							}
						}
					}

					if (selectedCluster == 0xFFFFFFFF)
					{
						continue;
					}

					Uint32 morphDeltaBufferIndex = 0xFFFFFFFF;
					Uint32 vertexMorphSourceBufferIndex = 0xFFFFFFFF;
					Uint32 morphDeltaOffset = 0;
					Uint32 morphVertexOffset = 0;
					Uint32 morphVertexCount = 0;
					Uint32 morphTargetCount = 0;
					Uint32 morphWeightOffset = 0;

					if (!subMesh.morphs_.empty() && !crister->StandaloneVertices(selectedCluster))
					{
						Int ownerNodeIndex = -1;
						for (Size nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
						{
							if (nodes[nodeIndex].mesh_ == subMesh.meshIndex_)
							{
								ownerNodeIndex = static_cast<Int>(nodeIndex);
								break;
							}
						}

						auto entityWeightsIt = ownerNodeIndex >= 0 ? animatedMorphWeights_.find(entityID) : animatedMorphWeights_.end();
						if (entityWeightsIt != animatedMorphWeights_.end())
						{
							auto nodeWeightsIt = entityWeightsIt->second.find(ownerNodeIndex);
							if (nodeWeightsIt != entityWeightsIt->second.end() && !nodeWeightsIt->second.empty())
							{
								const DynamicArray<Float>& weights = nodeWeightsIt->second;
								if (morphWeights_.size() + weights.size() <= maxMorphWeightCount_)
								{
									morphWeightOffset = static_cast<Uint32>(morphWeights_.size());
									morphWeights_.insert(morphWeights_.end(), weights.begin(), weights.end());

									const DynamicArray<Float>* previousWeights = &weights;
									auto previousEntityWeightsIt = skinHistoryValid ? previousAnimatedMorphWeights_.find(entityID) : previousAnimatedMorphWeights_.end();
									if (previousEntityWeightsIt != previousAnimatedMorphWeights_.end())
									{
										auto previousNodeWeightsIt = previousEntityWeightsIt->second.find(ownerNodeIndex);
										if (previousNodeWeightsIt != previousEntityWeightsIt->second.end() && previousNodeWeightsIt->second.size() == weights.size())
										{
											previousWeights = &previousNodeWeightsIt->second;
										}
									}
									previousMorphWeights_.insert(previousMorphWeights_.end(), previousWeights->begin(), previousWeights->end());

									morphDeltaBufferIndex = crister->MorphDeltaBufferIndex();
									vertexMorphSourceBufferIndex = crister->VertexMorphSourceBufferIndex();
									morphDeltaOffset = subMesh.morphDeltaOffset_;
									morphVertexOffset = subMesh.vertexOffset_;
									morphVertexCount = subMesh.vertexCount_;
									morphTargetCount = static_cast<Uint32>(Min(weights.size(), subMesh.morphs_.size()));
								}
							}
						}
					}

					if (skinned)
					{
						crister->TouchCluster(subMesh.clusterOffset_, streamingFrame_);
					}
					else
					{
						for (Uint32 c = 0; c < subMesh.clusterCount_; ++c)
						{
							Uint32 clusterIndex = subMesh.clusterOffset_ + c;
							if (crister->ClusterResident(clusterIndex))
							{
								crister->TouchCluster(clusterIndex, streamingFrame_);
							}
						}
					}

					{
						Uint32 clusterIndex = selectedCluster;
						const Cluster& cluster = clusters[clusterIndex];

						Float lodErrorNext = FLT_MAX;

						constexpr Uint32 maxMeshletsPerDispatch = 32;

						if (cluster.meshletCount_ == 0)
						{
							continue;
						}

						ModelStructuredBuffer instanceData{};

						instanceData.texture_.baseColor_ = material.baseColor_;
						instanceData.texture_.metallic_ = material.metallic_;
						instanceData.texture_.roughness_ = material.roughness_;
						instanceData.texture_.alphaCutoff_ = material.alphaMode_ == 1 ? material.alphaCutoff_ : (material.alphaMode_ == 2 ? 0.01f : 0.0f);
						instanceData.texture_.emissive_ = Vector3(material.emissiveFactor_[0], material.emissiveFactor_[1], material.emissiveFactor_[2]);

						/// [EN] KHR material extensions consumed by the deferred G-Buffer.
						/// [JP] deferred G-Buffer が消費する KHR マテリアル拡張。
						instanceData.texture_.ior_ = material.khr_.ior_.ior_;
						instanceData.texture_.emissiveStrength_ = material.khr_.emissiveStrength_.emissiveStrength_;
						instanceData.extension_.specularFactor_ = material.khr_.specular_.specularFactor_;
						instanceData.extension_.specularColor_ = Vector3(material.khr_.specular_.specularColorFactor_[0], material.khr_.specular_.specularColorFactor_[1], material.khr_.specular_.specularColorFactor_[2]);
						instanceData.extension_.clearCoatFactor_ = material.khr_.clearCoat_.clearCoatFactor_;
						instanceData.extension_.clearCoatRoughness_ = material.khr_.clearCoat_.clearCoatRoughnessFactor_;
						instanceData.extension_.anisotropy_ = material.khr_.anisotropy_.anisotropyStrength_;
						instanceData.extension_.transmissionFactor_ = material.khr_.transmission_.transmissionFactor_;
						instanceData.extension_.volumeThicknessFactor_ = material.khr_.volume_.thicknessFactor_;
						instanceData.extension_.volumeAttenuationDistance_ = material.khr_.volume_.attenuationDistance_;
						instanceData.extension_.volumeAttenuationColor_ = Vector3(material.khr_.volume_.attenuationColor_[0], material.khr_.volume_.attenuationColor_[1], material.khr_.volume_.attenuationColor_[2]);
						instanceData.extension_.sheenColor_ = Vector3(material.khr_.sheen_.sheenColorFactor_[0], material.khr_.sheen_.sheenColorFactor_[1], material.khr_.sheen_.sheenColorFactor_[2]);
						instanceData.extension_.sheenRoughness_ = material.khr_.sheen_.sheenRoughnessFactor_;
						instanceData.extension_.iridescenceFactor_ = material.khr_.iridescence_.iridescenceFactor_;
						instanceData.extension_.iridescenceIor_ = material.khr_.iridescence_.iridescenceIor_;
						instanceData.extension_.iridescenceThickness_ = (material.khr_.iridescence_.iridescenceThicknessMinimum_ + material.khr_.iridescence_.iridescenceThicknessMaximum_) * 0.5f;
						instanceData.extension_.unlit_ = material.khr_.unlit_.unlit_ != 0 ? 1.0f : 0.0f;
						instanceData.shading_.shadingModel_ = material.khr_.unlit_.unlit_ != 0 ? static_cast<Uint>(ShadingModel::Unlit) : static_cast<Uint>(material.shadingModel_);

						/// [EN] Material stores glTF image indices — resolve to bindless heap indices.
						/// [JP] Material には glTF の image インデックスが入っているため、bindless ヒープインデックスに解決する。
						instanceData.texture_.baseColorTextureIndex_ = material.baseColorTextureAssetID_ != 0 ? textureResource.GetID(material.baseColorTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.baseColorTextureIndex_);
						instanceData.texture_.normalTextureIndex_ = material.normalTextureAssetID_ != 0 ? textureResource.GetID(material.normalTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.normalTextureIndex_);
						instanceData.texture_.metallicRoughnessTextureIndex_ = material.metallicRoughnessTextureAssetID_ != 0 ? textureResource.GetID(material.metallicRoughnessTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.metallicRoughnessTextureIndex_);
						instanceData.texture_.emissiveTextureIndex_ = material.emissiveTextureAssetID_ != 0 ? textureResource.GetID(material.emissiveTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.emissiveTextureIndex_);
						instanceData.texture_.occlusionTextureIndex_ = material.occlusionTextureAssetID_ != 0 ? textureResource.GetID(material.occlusionTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.occlusionTextureIndex_);
						instanceData.extension_.specularTextureIndex_ = crister->TextureBindlessIndex(material.khr_.specular_.specularTextureIndex_);
						instanceData.extension_.specularColorTextureIndex_ = crister->TextureBindlessIndex(material.khr_.specular_.specularColorTextureIndex_);
						instanceData.extension_.clearCoatTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatTextureIndex_);
						instanceData.extension_.clearCoatRoughnessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatRoughnessTextureIndex_);
						instanceData.extension_.clearCoatNormalTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatNormalTextureIndex_);
						instanceData.extension_.transmissionTextureIndex_ = crister->TextureBindlessIndex(material.khr_.transmission_.transmissionTextureIndex_);
						instanceData.extension_.thicknessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.volume_.thicknessTextureIndex_);
						instanceData.extension_.sheenColorTextureIndex_ = crister->TextureBindlessIndex(material.khr_.sheen_.sheenColorTextureIndex_);
						instanceData.extension_.sheenRoughnessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.sheen_.sheenRoughnessTextureIndex_);
						instanceData.extension_.iridescenceTextureIndex_ = crister->TextureBindlessIndex(material.khr_.iridescence_.iridescenceTextureIndex_);
						instanceData.extension_.iridescenceThicknessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.iridescence_.iridescenceThicknessTextureIndex_);
						instanceData.extension_.anisotropyTextureIndex_ = crister->TextureBindlessIndex(material.khr_.anisotropy_.anisotropyTextureIndex_);
						instanceData.extension_.anisotropyRotation_ = material.khr_.anisotropy_.anisotropyRotation_;
						instanceData.shading_.furLength_ = material.furLength_;
						instanceData.shading_.furDensity_ = material.furDensity_;
						instanceData.shading_.furShellCount_ = static_cast<Uint>(material.furShellCount_);

						instanceData.geometry_.vertexBufferIndex_ = crister->ClusterVertexBufferIndex(clusterIndex);
						instanceData.skining_.skinVertexBufferIndex_ = crister->SkinVertexBufferIndex();
						instanceData.streaming_.positionMin_ = crister->PositionMin();
						instanceData.streaming_.positionExtent_ = crister->PositionExtent();
						instanceData.streaming_.texcoordMinU_ = crister->TexcoordMin().x;
						instanceData.streaming_.texcoordMinV_ = crister->TexcoordMin().y;
						instanceData.streaming_.texcoordExtent_ = crister->TexcoordExtent();
						instanceData.geometry_.meshletBufferIndex_ = crister->ClusterMeshletBufferIndex(clusterIndex);
						instanceData.geometry_.meshletBoundBufferIndex_ = crister->ClusterMeshletBoundBufferIndex(clusterIndex);
						instanceData.geometry_.vertexIndicesBufferIndex_ = crister->ClusterVertexIndicesBufferIndex(clusterIndex);
						instanceData.geometry_.primitiveIndicesBufferIndex_ = crister->ClusterPrimitiveIndicesBufferIndex(clusterIndex);

						instanceData.morph_.morphDeltaBufferIndex_ = morphDeltaBufferIndex;
						instanceData.morph_.vertexMorphSourceBufferIndex_ = vertexMorphSourceBufferIndex;
						instanceData.morph_.morphDeltaOffset_ = morphDeltaOffset;
						instanceData.morph_.morphVertexOffset_ = morphVertexOffset;
						instanceData.morph_.morphVertexCount_ = morphVertexCount;
						instanceData.morph_.morphTargetCount_ = morphTargetCount;
						instanceData.morph_.morphWeightOffset_ = morphWeightOffset;

						instanceData.streaming_.lodError_ = 0.0f;
						instanceData.streaming_.lodErrorNext_ = lodErrorNext;

						if (skinned)
						{
							Uint skinBoneOffset = boneBase;
							for (Int skinIndex = 0; skinIndex < subMesh.skinIndex_; skinIndex++)
							{
								skinBoneOffset += static_cast<Uint>(skins[skinIndex].joints_.size());
							}
							instanceData.skining_.skinIndex_ = static_cast<Uint>(subMesh.skinIndex_);
							instanceData.skining_.boneOffset_ = skinBoneOffset;
						}
						else
						{
							instanceData.skining_.skinIndex_ = 0xFFFFFFFF;
							instanceData.skining_.boneOffset_ = 0;
						}

						instanceData.shading_.doubleSided_ = material.doubleSided_ ? 1 : 0;
						instanceData.shading_.blend_ = material.alphaMode_ == 2 ? 1 : 0;

						instanceData.shading_.selected_ = selected ? 1 : 0;
						if (selected)
						{
							hasSelectedInstance_ = true;
							hasSelectedSkinned_ = hasSelectedSkinned_ || skinned;
						}

						for (const Matrix& placement : crister->SubMeshPlacement(subMeshIndex))
						{
							Matrix placedWorldMatrix = placement * worldMatrix;
							instanceData.transform_.world_ = placedWorldMatrix;
							instanceData.transform_.inverseTransposeWorld_ = placedWorldMatrix.Invert().Transpose();
							instanceData.transform_.previousWorld_ = placement * previousWorldMatrix;

							Uint32 remaining = cluster.meshletCount_;
							Uint32 offset = cluster.meshletOffset_;

							while (remaining > 0)
							{
								Uint32 count = (remaining > maxMeshletsPerDispatch) ? maxMeshletsPerDispatch : remaining;

								instanceData.geometry_.meshletOffset_ = offset - cluster.meshletOffset_;
								instanceData.geometry_.meshletCount_ = count;

								if (material.alphaMode_ != 2)
								{
									opaqueInstances_.push_back(instanceData);
									hasSkinnedOpaque_ = hasSkinnedOpaque_ || instanceData.skining_.skinIndex_ != 0xFFFFFFFF;
									if (instanceData.shading_.shadingModel_ == static_cast<Uint>(ShadingModel::Fur))
									{
										furInstances_.push_back(instanceData);
									}
								}
								else
								{
									transparentInstances_.push_back(instanceData);
									hasSkinnedTransparent_ = hasSkinnedTransparent_ || instanceData.skining_.skinIndex_ != 0xFFFFFFFF;
								}

								offset += count;
								remaining -= count;
							}
						}
					}
				}
			});

		DynamicArray<EntityID> softbodyEntities;
		Query<Read<Softbody>>(world).ForEach([&](EntityID entityID, const Softbody&) { softbodyEntities.push_back(entityID); });
		for (EntityID entityID : softbodyEntities)
		{
			Actor actor = WorldAccessor::Make(world, entityID);
			if (!actor)
			{
				continue;
			}

			const Active* active = actor.GetComponent<Active>();
			if (active && !active->active_)
			{
				continue;
			}

			Softbody* softbody = actor.GetComponent<Softbody>();
			if (!softbody || softbody->BodyID() == ScInvalidBodyID)
			{
				continue;
			}

			const Mesh* mesh = actor.GetComponent<Mesh>();
			if (!mesh)
			{
				continue;
			}

			Handle<Crister> cristerHandle = modelResource.GetHandle(mesh->meshID_);
			if (cristerHandle.empty())
			{
				continue;
			}

			Crister* crister = modelResource.Resolve(cristerHandle);
			if (!crister)
			{
				continue;
			}

			ResourcePtr<SoftbodyMesh>& softbodyMesh = softbodyMeshes_[entityID];
			if (!softbodyMesh)
			{
				softbodyMesh = MakePtr<SoftbodyMesh>();
				if (!softbodyMesh->Create(device_, bindlessHeap_, *crister))
				{
					softbodyMeshes_.erase(entityID);
					continue;
				}
			}

			softbodyMesh->Update(softbody->BodyVertex());

			Matrix worldMatrix = actor.WorldMatrix();
			Matrix inverseTransposeWorld = worldMatrix.Invert().Transpose();

			auto previousWorldIt = previousWorldMatrices_.find(entityID);
			Matrix previousWorldMatrix = previousWorldIt != previousWorldMatrices_.end() ? previousWorldIt->second : worldMatrix;
			previousWorldMatrices_[entityID] = worldMatrix;

			const Material* materialComponent = actor.GetComponent<Material>();
			const DynamicArray<Surface>& surfaces = (materialComponent && materialComponent->resolvedSurfaces_.size() == crister->Surfaces().size()) ? materialComponent->resolvedSurfaces_ : crister->Surfaces();
			const Surface& material = !surfaces.empty() ? surfaces[0] : emptySurface;

			constexpr Uint32 maxMeshletsPerDispatch = 32;
			Uint32 remaining = softbodyMesh->MeshletCount();
			Uint32 offset = 0;

			while (remaining > 0)
			{
				Uint32 count = (remaining > maxMeshletsPerDispatch) ? maxMeshletsPerDispatch : remaining;

				ModelStructuredBuffer instanceData{};
				instanceData.transform_.world_ = worldMatrix;
				instanceData.transform_.inverseTransposeWorld_ = inverseTransposeWorld;
				instanceData.transform_.previousWorld_ = previousWorldMatrix;

				instanceData.texture_.baseColor_ = material.baseColor_;
				instanceData.texture_.metallic_ = material.metallic_;
				instanceData.texture_.roughness_ = material.roughness_;
				instanceData.texture_.alphaCutoff_ = material.alphaMode_ == 1 ? material.alphaCutoff_ : (material.alphaMode_ == 2 ? 0.01f : 0.0f);
				instanceData.texture_.emissive_ = Vector3(material.emissiveFactor_[0], material.emissiveFactor_[1], material.emissiveFactor_[2]);

				instanceData.texture_.ior_ = material.khr_.ior_.ior_;
				instanceData.texture_.emissiveStrength_ = material.khr_.emissiveStrength_.emissiveStrength_;
				instanceData.extension_.specularFactor_ = material.khr_.specular_.specularFactor_;
				instanceData.extension_.specularColor_ = Vector3(material.khr_.specular_.specularColorFactor_[0], material.khr_.specular_.specularColorFactor_[1], material.khr_.specular_.specularColorFactor_[2]);
				instanceData.extension_.clearCoatFactor_ = material.khr_.clearCoat_.clearCoatFactor_;
				instanceData.extension_.clearCoatRoughness_ = material.khr_.clearCoat_.clearCoatRoughnessFactor_;
				instanceData.extension_.anisotropy_ = material.khr_.anisotropy_.anisotropyStrength_;
				instanceData.extension_.transmissionFactor_ = material.khr_.transmission_.transmissionFactor_;
				instanceData.extension_.volumeThicknessFactor_ = material.khr_.volume_.thicknessFactor_;
				instanceData.extension_.volumeAttenuationDistance_ = material.khr_.volume_.attenuationDistance_;
				instanceData.extension_.volumeAttenuationColor_ = Vector3(material.khr_.volume_.attenuationColor_[0], material.khr_.volume_.attenuationColor_[1], material.khr_.volume_.attenuationColor_[2]);
				instanceData.extension_.sheenColor_ = Vector3(material.khr_.sheen_.sheenColorFactor_[0], material.khr_.sheen_.sheenColorFactor_[1], material.khr_.sheen_.sheenColorFactor_[2]);
				instanceData.extension_.sheenRoughness_ = material.khr_.sheen_.sheenRoughnessFactor_;
				instanceData.extension_.iridescenceFactor_ = material.khr_.iridescence_.iridescenceFactor_;
				instanceData.extension_.iridescenceIor_ = material.khr_.iridescence_.iridescenceIor_;
				instanceData.extension_.iridescenceThickness_ = (material.khr_.iridescence_.iridescenceThicknessMinimum_ + material.khr_.iridescence_.iridescenceThicknessMaximum_) * 0.5f;
				instanceData.extension_.unlit_ = material.khr_.unlit_.unlit_ != 0 ? 1.0f : 0.0f;
				instanceData.shading_.shadingModel_ = material.khr_.unlit_.unlit_ != 0 ? static_cast<Uint>(ShadingModel::Unlit) : static_cast<Uint>(material.shadingModel_);

				instanceData.texture_.baseColorTextureIndex_ = material.baseColorTextureAssetID_ != 0 ? textureResource.GetID(material.baseColorTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.baseColorTextureIndex_);
				instanceData.texture_.normalTextureIndex_ = material.normalTextureAssetID_ != 0 ? textureResource.GetID(material.normalTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.normalTextureIndex_);
				instanceData.texture_.metallicRoughnessTextureIndex_ = material.metallicRoughnessTextureAssetID_ != 0 ? textureResource.GetID(material.metallicRoughnessTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.metallicRoughnessTextureIndex_);
				instanceData.texture_.emissiveTextureIndex_ = material.emissiveTextureAssetID_ != 0 ? textureResource.GetID(material.emissiveTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.emissiveTextureIndex_);
				instanceData.texture_.occlusionTextureIndex_ = material.occlusionTextureAssetID_ != 0 ? textureResource.GetID(material.occlusionTextureAssetID_, streamingFrame_) : crister->TextureBindlessIndex(material.occlusionTextureIndex_);
				instanceData.extension_.specularTextureIndex_ = crister->TextureBindlessIndex(material.khr_.specular_.specularTextureIndex_);
				instanceData.extension_.specularColorTextureIndex_ = crister->TextureBindlessIndex(material.khr_.specular_.specularColorTextureIndex_);
				instanceData.extension_.clearCoatTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatTextureIndex_);
				instanceData.extension_.clearCoatRoughnessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatRoughnessTextureIndex_);
				instanceData.extension_.clearCoatNormalTextureIndex_ = crister->TextureBindlessIndex(material.khr_.clearCoat_.clearCoatNormalTextureIndex_);
				instanceData.extension_.transmissionTextureIndex_ = crister->TextureBindlessIndex(material.khr_.transmission_.transmissionTextureIndex_);
				instanceData.extension_.thicknessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.volume_.thicknessTextureIndex_);
				instanceData.extension_.sheenColorTextureIndex_ = crister->TextureBindlessIndex(material.khr_.sheen_.sheenColorTextureIndex_);
				instanceData.extension_.sheenRoughnessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.sheen_.sheenRoughnessTextureIndex_);
				instanceData.extension_.iridescenceTextureIndex_ = crister->TextureBindlessIndex(material.khr_.iridescence_.iridescenceTextureIndex_);
				instanceData.extension_.iridescenceThicknessTextureIndex_ = crister->TextureBindlessIndex(material.khr_.iridescence_.iridescenceThicknessTextureIndex_);
				instanceData.extension_.anisotropyTextureIndex_ = crister->TextureBindlessIndex(material.khr_.anisotropy_.anisotropyTextureIndex_);
				instanceData.extension_.anisotropyRotation_ = material.khr_.anisotropy_.anisotropyRotation_;
				instanceData.shading_.furLength_ = material.furLength_;
				instanceData.shading_.furDensity_ = material.furDensity_;
				instanceData.shading_.furShellCount_ = static_cast<Uint>(material.furShellCount_);

				/// [EN] SoftbodyMesh's own buffers, not Crister's — see
				///      SoftbodyMesh's class comment.
				/// [JP] Crister のではなく SoftbodyMesh 自身のバッファ —
				///      SoftbodyMesh のクラスコメント参照。
				instanceData.geometry_.vertexBufferIndex_ = softbodyMesh->VertexBufferIndex();
				instanceData.skining_.skinVertexBufferIndex_ = 0xFFFFFFFF;
				instanceData.streaming_.positionMin_ = softbodyMesh->PositionMin();
				instanceData.streaming_.positionExtent_ = softbodyMesh->PositionExtent();
				instanceData.streaming_.texcoordMinU_ = softbodyMesh->TexcoordMin().x;
				instanceData.streaming_.texcoordMinV_ = softbodyMesh->TexcoordMin().y;
				instanceData.streaming_.texcoordExtent_ = softbodyMesh->TexcoordExtent();
				instanceData.geometry_.meshletBufferIndex_ = softbodyMesh->MeshletBufferIndex();
				instanceData.geometry_.meshletBoundBufferIndex_ = softbodyMesh->MeshletBoundBufferIndex();
				instanceData.geometry_.vertexIndicesBufferIndex_ = softbodyMesh->VertexIndicesBufferIndex();
				instanceData.geometry_.primitiveIndicesBufferIndex_ = softbodyMesh->PrimitiveIndicesBufferIndex();

				instanceData.geometry_.meshletOffset_ = offset;
				instanceData.geometry_.meshletCount_ = count;

				instanceData.streaming_.lodError_ = 0.0f;
				instanceData.streaming_.lodErrorNext_ = FLT_MAX;

				instanceData.skining_.skinIndex_ = 0xFFFFFFFF;
				instanceData.skining_.boneOffset_ = 0;

				instanceData.shading_.doubleSided_ = material.doubleSided_ ? 1 : 0;
				instanceData.shading_.blend_ = material.alphaMode_ == 2 ? 1 : 0;

				instanceData.shading_.selected_ = std::ranges::find(selectedEntities, ActorAccessor::GetEntity(actor)) != selectedEntities.end() ? 1 : 0;
				if (instanceData.shading_.selected_)
				{
					hasSelectedInstance_ = true;
				}

				if (material.alphaMode_ != 2)
				{
					opaqueInstances_.push_back(instanceData);
					if (instanceData.shading_.shadingModel_ == static_cast<Uint>(ShadingModel::Fur))
					{
						furInstances_.push_back(instanceData);
					}
				}
				else
				{
					transparentInstances_.push_back(instanceData);
				}

				offset += count;
				remaining -= count;
			}
		}

		constexpr Uint maxUploadsPerFrame = 4;
		Uint uploads = 0;
		for (const auto& request : geometryStreamingRequests_)
		{
			if (uploads >= maxUploadsPerFrame)
			{
				break;
			}
			if (!request.first->ClusterResident(request.second))
			{
				request.first->MakeClusterResident(request.second);
				request.first->TouchCluster(request.second, streamingFrame_);
				uploads++;
			}
		}
		geometryStreamingRequests_.clear();

		constexpr Uint maxTextureUploadsPerFrame = 8;
		Uint textureUploads = 0;
		for (const TextureStreamingRequest& request : textureStreamingRequests_)
		{
			if (textureUploads >= maxTextureUploadsPerFrame)
			{
				break;
			}
			if (request.crister_->TextureFinestMip(request.textureIndex_) > request.desiredMip_)
			{
				request.crister_->MakeTextureMipResident(request.textureIndex_, request.desiredMip_);
				request.crister_->TouchTexture(request.textureIndex_, streamingFrame_);
				textureUploads++;
			}
		}
		textureStreamingRequests_.clear();

		Crister::EvictClusterBudget(streamingFrame_);
		Crister::EvictTextureBudget(streamingFrame_);
	}

	void ModelRenderer::Upload()
	{
		/// [EN] Upload is invoked once per view (editor / game) but the gathered
		///      data is view-independent — copy the tens of megabytes only once
		///      per Gather.
		/// [JP] Upload はビューごと（エディタ / ゲーム）に呼ばれるが、収集済み
		///      データはビュー非依存 — 数十 MB のコピーは Gather ごとに 1 回だけにする。
		if (uploaded_)
		{
			return;
		}
		uploaded_ = true;

		/// [EN] Frame-ring buffers: re-register the current frame's SRV indices.
		/// [JP] フレームリングバッファ: 現在フレームの SRV インデックスを再登録する。
		shaderResourceIndicesSystem_->SetModelInstanceIndex(instanceBuffer_->Index());
		shaderResourceIndicesSystem_->SetModelBoneMatrixIndex(boneBuffer_->Index());
		shaderResourceIndicesSystem_->SetModelMorphWeightIndex(morphWeightBuffer_->Index());
		constantIndicesSystem_->SetModelFurIndex(modelFurConstantBuffer_->Index());
		shaderResourceIndicesSystem_->SetModelPreviousBoneMatrixIndex(previousBoneBuffer_->Index());
		shaderResourceIndicesSystem_->SetModelPreviousMorphWeightIndex(previousMorphWeightBuffer_->Index());

		FurConstantBuffer furData{};
		furData.furInstanceOffset_ = static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size());
		furData.furInstanceCount_ = static_cast<Uint>(furInstances_.size());
		modelFurConstantBuffer_->Update(furData);

		if (!opaqueInstances_.empty() || !transparentInstances_.empty() || !furInstances_.empty())
		{
			DynamicArray<ModelStructuredBuffer> allInstances;
			allInstances.reserve(opaqueInstances_.size() + transparentInstances_.size() + furInstances_.size());
			allInstances.insert(allInstances.end(), opaqueInstances_.begin(), opaqueInstances_.end());
			allInstances.insert(allInstances.end(), transparentInstances_.begin(), transparentInstances_.end());
			allInstances.insert(allInstances.end(), furInstances_.begin(), furInstances_.end());

			instanceBuffer_->Update(allInstances.data(), static_cast<Uint>(allInstances.size()));
		}

		if (!boneMatrices_.empty())
		{
			boneBuffer_->Update(boneMatrices_.data(), static_cast<Uint>(boneMatrices_.size()));
			previousBoneBuffer_->Update(previousBoneMatrices_.data(), static_cast<Uint>(previousBoneMatrices_.size()));
		}

		if (!morphWeights_.empty())
		{
			morphWeightBuffer_->Update(morphWeights_.data(), static_cast<Uint>(morphWeights_.size()));
			previousMorphWeightBuffer_->Update(previousMorphWeights_.data(), static_cast<Uint>(previousMorphWeights_.size()));
		}

		if (D3D12Check::GetLevel() != D3D12Level::D12_2)
		{
			modelCullingBuffer_.Reserve(Max(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()) * 32, static_cast<Uint>(furInstances_.size()) * furShellMax_ * 32));
		}
	}

	Uint ModelRenderer::BoneMatrixBufferIndex()const
	{
		return boneBuffer_->Index();
	}

	Bool ModelRenderer::TryGetAnimatedBoneOffset(EntityID entityID, Uint32& outBoneOffset)const
	{
		auto found = animatedBoneOffsets_.find(entityID);
		if (found == animatedBoneOffsets_.end())
		{
			return false;
		}
		outBoneOffset = found->second;
		return true;
	}

	Bool ModelRenderer::TryGetAnimatedMorphWeights(EntityID entityID, Int nodeIndex, DynamicArray<Float>& outWeights)const
	{
		auto entityIt = animatedMorphWeights_.find(entityID);
		if (entityIt == animatedMorphWeights_.end())
		{
			return false;
		}
		auto nodeIt = entityIt->second.find(nodeIndex);
		if (nodeIt == entityIt->second.end())
		{
			return false;
		}
		outWeights = nodeIt->second;
		return true;
	}

	void ModelRenderer::DrawDepthPrepass(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (opaqueInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		/// [EN] All passes dispatch the full instance list; the AS entry skips
		///      instances that belong to the other pass (blend_ filter).
		/// [JP] 全パスが全インスタンスをディスパッチし、AS エントリが他方の
		///      パスに属するインスタンスをスキップする（blend_ フィルタ）。
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateDepthPrepass());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateModelCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateDepthPrepass());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateDepthPrepassDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			modelCullingBuffer_.End(cmd);
		}
	}

	void ModelRenderer::DrawOpaque(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (opaqueInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateStatic());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();

			/// [EN] Skinned instances are skipped by StaticModelMS and drawn here by
			///      the skeletal pipeline (SkeletalModelMS filters the inverse set).
			/// [JP] スキンインスタンスは StaticModelMS でスキップされ、ここでスケルタル
			///      パイプラインが描画する（SkeletalModelMS が逆の集合をフィルタする）。
			if (hasSkinnedOpaque_)
			{
				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletal());
				cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
				ProfilerStats::AddDrawCall();
			}
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateGeometryBufferCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateStatic());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateStaticDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedOpaque_)
			{
				cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletal());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
				ProfilerStats::AddDrawCall();

				cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletalDoubleSided());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
				ProfilerStats::AddDrawCall();
			}

			modelCullingBuffer_.End(cmd);
		}
	}

	void ModelRenderer::Compose(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		frameBuffer->Rebind(cmdList);

		auto* cmd = cmdList->Get();

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		cmd->SetPipelineState(modelShader_.GetPipelineStateComposite());
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(1, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(3, 1, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void ModelRenderer::DrawWireframe(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (opaqueInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		/// [JP] エディタフレームバッファの色 ＋ ジオメトリ深度（読み取りのみ）を bind。
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = frameBuffer->RenderTargetViewHandle();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilViewHandle = geometryBuffer->DepthStencilViewHandle();
		cmd->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, &depthStencilViewHandle);

		D3D12_VIEWPORT viewport = frameBuffer->GetViewport();
		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeStatic());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedOpaque_)
			{
				cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeSkeletal());
				cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
				ProfilerStats::AddDrawCall();
			}
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateGeometryBufferCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeStatic());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeStaticDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedOpaque_)
			{
				cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeSkeletal());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
				ProfilerStats::AddDrawCall();

				cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateWireframeSkeletalDoubleSided());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
				ProfilerStats::AddDrawCall();
			}

			modelCullingBuffer_.End(cmd);
		}
	}

	void ModelRenderer::DrawMeshlet(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (opaqueInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = frameBuffer->RenderTargetViewHandle();
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilViewHandle = geometryBuffer->DepthStencilViewHandle();
		cmd->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, &depthStencilViewHandle);

		D3D12_VIEWPORT viewport = frameBuffer->GetViewport();
		cmd->RSSetViewports(1, &viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		cmd->RSSetScissorRects(1, &scissorRect);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletStatic());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedOpaque_)
			{
				cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletSkeletal());
				cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
				ProfilerStats::AddDrawCall();
			}
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateGeometryBufferCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletStatic());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletStaticDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedOpaque_)
			{
				cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletSkeletal());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
				ProfilerStats::AddDrawCall();

				cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateMeshletSkeletalDoubleSided());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
				ProfilerStats::AddDrawCall();
			}

			modelCullingBuffer_.End(cmd);
		}
	}

	void ModelRenderer::DrawTransparent(D3D12CommandList* cmdList, FrameBuffer* frameBuffer, GeometryBuffer* geometryBuffer, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (transparentInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		oitBuffer_.Clear(cmd);
		oitBuffer_.Barrier(cmd);

		geometryBuffer->BeginDepthOnly(cmdList);

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateStaticTransparent());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedTransparent_)
			{
				oitBuffer_.Barrier(cmd);

				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletalTransparent());
				cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
				ProfilerStats::AddDrawCall();
			}
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateModelTransparentCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateStaticTransparent());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			oitBuffer_.Barrier(cmd);

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateStaticTransparentDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			if (hasSkinnedTransparent_)
			{
				oitBuffer_.Barrier(cmd);

				cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletalTransparent());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
				ProfilerStats::AddDrawCall();

				oitBuffer_.Barrier(cmd);

				cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSkeletalTransparentDoubleSided());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
				ProfilerStats::AddDrawCall();
			}

			modelCullingBuffer_.End(cmd);
		}

		oitBuffer_.Barrier(cmd);

		geometryBuffer->EndDepth(cmdList);

		frameBuffer->Rebind(cmdList);

		cmd->SetPipelineState(modelShader_.GetPipelineStateResolve());
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->DispatchMesh(1, 1, 1);
		}
		else
		{
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(3, 1, 0, 0);
		}
		ProfilerStats::AddDrawCall();
	}

	void ModelRenderer::DrawFurShell(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (furInstances_.empty())
		{
			return;
		}

		auto* cmd = cmdList->Get();

		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		/// [EN] Draws onto the already-bound lit HDR frame (blend), depth-test
		///      against the opaque depth without writing. Call after DrawTransparent.
		/// [JP] バインド済みのライティング済み HDR フレームへブレンド描画、
		///      不透明の深度に対してテストのみ(書き込みなし)。DrawTransparent の後に呼ぶ。
		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateFurShell());
			cmd->DispatchMesh(static_cast<Uint>(furInstances_.size()) * furShellMax_, 1, 1);
			ProfilerStats::AddDrawCall();
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateFurShellCulling());
			cmd->Dispatch(static_cast<Uint>(furInstances_.size()) * furShellMax_, 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateFurShell());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateFurShellDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			modelCullingBuffer_.End(cmd);
		}
	}

	void ModelRenderer::DrawSilhouette(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses)
	{
		if (!hasSelectedInstance_)
		{
			return;
		}

		auto* cmd = cmdList->Get();

		/// [JP] 深度なしで選択メッシュのシルエット全体を描く。手前の未選択オブジェクト
		///      に遮蔽されても穴を開けない（理由は Silhouette PSO のコメント参照）。
		///      マスクの Begin/Clear/End は呼び出し側（Renderer）が一括で行う
		///      （Model/Sprite/Billboard/Font が同じ 1 枚のマスクを共有するため）。
		ID3D12DescriptorHeap* heaps[] = { heap };
		cmd->SetDescriptorHeaps(_countof(heaps), heaps);
		cmd->SetGraphicsRootSignature(modelShader_.GetRootSignature());
		RootSignature::BindGraphics(cmd, addresses);

		if (D3D12Check::GetLevel() == D3D12Level::D12_2)
		{
			cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteStatic());
			cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
			ProfilerStats::AddDrawCall();

			if (hasSelectedSkinned_)
			{
				cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteSkeletal());
				cmd->DispatchMesh(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);
				ProfilerStats::AddDrawCall();
			}
		}
		else
		{
			modelCullingBuffer_.Begin(cmd);

			cmd->SetComputeRootSignature(modelShader_.GetRootSignature());
			RootSignature::BindCompute(cmd, addresses);
			Uint cullingIndex = modelCullingBuffer_.GetConstantBufferIndex();
			cmd->SetComputeRoot32BitConstants(3, 1, &cullingIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateModelSilhouetteCulling());
			cmd->Dispatch(static_cast<Uint>(opaqueInstances_.size() + transparentInstances_.size()), 1, 1);

			modelCullingBuffer_.Barrier(cmd);

			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			Uint singleSidedIndex = modelCullingBuffer_.GetSingleSidedShaderResourceViewIndex();
			Uint doubleSidedIndex = modelCullingBuffer_.GetDoubleSidedShaderResourceViewIndex();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteStatic());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
			ProfilerStats::AddDrawCall();

			cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
			cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteStaticDoubleSided());
			cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
			ProfilerStats::AddDrawCall();

			if (hasSelectedSkinned_)
			{
				cmd->SetGraphicsRoot32BitConstants(3, 1, &singleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteSkeletal());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), 0, nullptr, 0);
				ProfilerStats::AddDrawCall();

				cmd->SetGraphicsRoot32BitConstants(3, 1, &doubleSidedIndex, 0);
				cmd->SetPipelineState(modelShader_.GetPipelineStateSilhouetteSkeletalDoubleSided());
				cmd->ExecuteIndirect(modelCullingBuffer_.GetCommandSignature(), 1, modelCullingBuffer_.GetArgumentBuffer(), sizeof(D3D12_DRAW_ARGUMENTS), nullptr, 0);
				ProfilerStats::AddDrawCall();
			}

			modelCullingBuffer_.End(cmd);
		}
	}

}

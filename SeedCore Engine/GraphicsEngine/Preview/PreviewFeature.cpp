#include <GraphicsEngine/Preview/PreviewFeature.h>
#include <GraphicsEngine/Preview/PreviewStage.h>
#include <GraphicsEngine/Preview/PreviewRequest.h>
#include <GraphicsEngine/Avatar/AvatarMesh.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Animation/Animation.h>
#include <GraphicsEngine/Model/Animation/AnimationResource.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(PreviewFeature, DrawStage);

	void PreviewFeature::Create(RenderDevice& device)
	{
		meshPipeline_.Create(device);
		linePipeline_.Create(device);
	}

	void PreviewFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		instances_.clear();
		meshlets_.clear();
		bones_.clear();
		lines_.clear();
		lining_ = false;

		const PreviewRequest* request = scene.Find<PreviewRequest>();
		if (!request)
		{
			return;
		}
		Matrix world = request->world_;
		Matrix inverseTransposeWorld = world.Invert().Transpose();

		if (request->avatar_)
		{
			const AvatarMesh& mesh = *request->avatar_;
			if (!mesh.Created())
			{
				return;
			}
			for (Uint32 regionIndex = 0;regionIndex < mesh.RegionCount();++regionIndex)
			{
				const AvatarMesh::RegionMeshletRange& range = mesh.MeshletRangeForRegion(regionIndex);
				if (range.meshletCount_ == 0)
				{
					continue;
				}

				ModelStructuredBuffer instance{};
				instance.transform_.world_ = world;
				instance.transform_.inverseTransposeWorld_ = inverseTransposeWorld;
				instance.transform_.previousWorld_ = world;
				instance.geometry_.vertexBufferIndex_ = mesh.VertexBufferIndex();
				instance.geometry_.meshletBufferIndex_ = mesh.MeshletBufferIndex();
				instance.geometry_.meshletBoundBufferIndex_ = mesh.MeshletBoundBufferIndex();
				instance.geometry_.vertexIndicesBufferIndex_ = mesh.VertexIndicesBufferIndex();
				instance.geometry_.primitiveIndicesBufferIndex_ = mesh.PrimitiveIndicesBufferIndex();
				instance.geometry_.meshletOffset_ = range.meshletOffset_;
				instance.geometry_.meshletCount_ = range.meshletCount_;
				instance.streaming_.positionMin_ = mesh.PositionMin();
				instance.streaming_.positionExtent_ = mesh.PositionExtent();
				instance.streaming_.texcoordMinU_ = mesh.TexcoordMin().x;
				instance.streaming_.texcoordMinV_ = mesh.TexcoordMin().y;
				instance.streaming_.texcoordExtent_ = mesh.TexcoordExtent();
				instance.streaming_.lodLevel_ = 0;
				instance.shading_.doubleSided_ = 1;
				instance.texture_.baseColor_ = Color(0.78f, 0.76f, 0.74f, 1.0f);
				instance.texture_.baseColorTextureIndex_ = regionIndex < request->regionCount_ ? request->regionTextures_[regionIndex] : SC_INVALID;
				instance.texture_.normalTextureIndex_ = SC_INVALID;
				instance.texture_.metallicRoughnessTextureIndex_ = SC_INVALID;
				instance.texture_.emissiveTextureIndex_ = SC_INVALID;
				instance.texture_.occlusionTextureIndex_ = SC_INVALID;
				instance.skining_.skinIndex_ = SC_INVALID;
				instance.skining_.skinVertexBufferIndex_ = SC_INVALID;
				instance.morph_.morphDeltaBufferIndex_ = SC_INVALID;
				instance.morph_.vertexMorphSourceBufferIndex_ = SC_INVALID;

				Uint32 instanceIndex = static_cast<Uint32>(instances_.size());
				instances_.push_back(instance);
				for (Uint32 meshletIndex = 0;meshletIndex < range.meshletCount_;++meshletIndex)
				{
					meshlets_.push_back(instanceIndex);
					meshlets_.push_back(range.meshletOffset_ + meshletIndex);
				}
			}
			return;
		}

		ModelResource& modelResource = *scene.resource_->modelResource_;
		Handle<Crister> cristerHandle = modelResource.GetHandle(request->meshAssetID_);
		if (cristerHandle.empty())
		{
			return;
		}
		Crister* crister = modelResource.Resolve(cristerHandle);
		if (!crister)
		{
			return;
		}

		std::span<const Surface> surfaces = crister->Surfaces();
		DynamicArray<Surface> overrides;
		if (request->surfaceAssetID_ != 0)
		{
			MaterialResource& materialResource = *scene.resource_->materialResource_;
			const BakedSurface* baked = materialResource.Resolve(materialResource.GetHandle(request->surfaceAssetID_));
			if (baked)
			{
				overrides.assign(surfaces.size(), baked->surface_);
				surfaces = overrides;
			}
		}

		Bool reset = cristerHandle != crister_ || request->surfaceAssetID_ != surfaceAssetID_;
		crister_ = cristerHandle;
		surfaceAssetID_ = request->surfaceAssetID_;
		++frame_;
		materials_.Begin(frame_);
		if (surfaces.empty() || materials_.Write(0, reset, surfaces, static_cast<Uint32>(surfaces.size()), *crister, *scene.resource_->textureStreaming_) == SC_INVALID)
		{
			return;
		}
		std::span<const ModelMaterialStructuredBuffer> materials = materials_.Materials(0);

		std::span<const Node> nodes = crister->Nodes();
		std::span<const Skin> skins = crister->Skins();
		DynamicArray<Matrix> pose(nodes.size(), Matrix::Identity);
		for (Size nodeIndex = 0;nodeIndex < nodes.size();++nodeIndex)
		{
			pose[nodeIndex] = nodes[nodeIndex].globalTransform_;
		}

		if (request->animationAssetID_ != 0 && !skins.empty() && crister->DefaultStage() >= 0)
		{
			AnimationResource& animationResource = *scene.resource_->animationResource_;
			Handle<Animation> animationHandle = animationResource.GetHandle(request->animationAssetID_);
			Animation* animation = animationHandle.empty() ? nullptr : animationResource.Resolve(animationHandle);
			if (animation)
			{
				Float duration = animation->Duration();
				Float time = duration > 0.0f ? std::fmod(request->time_, duration) : request->time_;
				std::unordered_map<Int, Vector3> translations;
				std::unordered_map<Int, Quaternion> rotations;
				std::unordered_map<Int, Vector3> scales;
				animation->SamplePose(time, translations, rotations, scales);

				DynamicArray<std::pair<Int, Matrix>> pending;
				for (Int rootIndex : crister->Stages()[crister->DefaultStage()].nodes_)
				{
					pending.push_back({ rootIndex, Matrix::Identity });
				}
				while (!pending.empty())
				{
					std::pair<Int, Matrix> current = pending.back();
					pending.pop_back();
					const Node& node = nodes[current.first];

					auto translation = translations.find(current.first);
					auto rotation = rotations.find(current.first);
					auto scale = scales.find(current.first);
					Vector3 localScale = scale != scales.end() ? scale->second : node.scale_;
					Quaternion localRotation = rotation != rotations.end() ? rotation->second : node.rotation_;
					Vector3 localTranslation = translation != translations.end() ? translation->second : node.translation_;
					Matrix local = Matrix::CreateScale(localScale) * Matrix::CreateFromQuaternion(localRotation) * Matrix::CreateTranslation(localTranslation);

					pose[current.first] = local * current.second;
					for (Int childIndex : node.children_)
					{
						pending.push_back({ childIndex, pose[current.first] });
					}
				}
			}
		}

		for (const Skin& skin : skins)
		{
			for (Size joint = 0;joint < skin.joints_.size();++joint)
			{
				const Matrix& global = pose[skin.joints_[joint]];
				bones_.push_back(joint < skin.inverseBindMatrices_.size() ? skin.inverseBindMatrices_[joint] * global : global);
			}
		}

		std::span<const SubMesh> subMeshes = crister->SubMeshes();
		std::span<const Cluster> clusters = crister->Clusters();
		for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
		{
			const SubMesh& subMesh = subMeshes[subMeshIndex];
			if (subMesh.clusterCount_ == 0)
			{
				continue;
			}

			Bool skinned = subMesh.skinIndex_ >= 0 && subMesh.skinIndex_ < static_cast<Int>(skins.size());
			Uint32 clusterIndex = SC_INVALID;
			for (Uint32 clusterOffset = 0;clusterOffset < subMesh.clusterCount_;++clusterOffset)
			{
				if (crister->Residency().ClusterResident(subMesh.clusterOffset_ + clusterOffset))
				{
					clusterIndex = subMesh.clusterOffset_ + clusterOffset;
					break;
				}
				if (skinned)
				{
					break;
				}
			}
			if (clusterIndex == SC_INVALID || clusters[clusterIndex].meshletCount_ == 0)
			{
				continue;
			}
			crister->Residency().TouchCluster(clusterIndex, frame_);

			Uint32 boneOffset = 0;
			for (Int skinIndex = 0;skinIndex < subMesh.skinIndex_;++skinIndex)
			{
				boneOffset += static_cast<Uint32>(skins[skinIndex].joints_.size());
			}
			const ModelMaterialStructuredBuffer& material = materials[subMesh.surfaceIndex_ < materials.size() ? subMesh.surfaceIndex_ : 0];

			for (const Matrix& placement : crister->Residency().SubMeshPlacement(subMeshIndex))
			{
				Matrix placed = placement * world;

				ModelStructuredBuffer instance{};
				instance.transform_.world_ = placed;
				instance.transform_.inverseTransposeWorld_ = placed.Invert().Transpose();
				instance.transform_.previousWorld_ = placed;
				instance.geometry_.vertexBufferIndex_ = crister->Residency().ClusterVertexBufferIndex(clusterIndex);
				instance.geometry_.meshletBufferIndex_ = crister->Residency().ClusterMeshletBufferIndex(clusterIndex);
				instance.geometry_.meshletBoundBufferIndex_ = crister->Residency().ClusterMeshletBoundBufferIndex(clusterIndex);
				instance.geometry_.vertexIndicesBufferIndex_ = crister->Residency().ClusterVertexIndicesBufferIndex(clusterIndex);
				instance.geometry_.primitiveIndicesBufferIndex_ = crister->Residency().ClusterPrimitiveIndicesBufferIndex(clusterIndex);
				instance.geometry_.meshletCount_ = clusters[clusterIndex].meshletCount_;
				instance.streaming_.positionMin_ = crister->PositionMin();
				instance.streaming_.positionExtent_ = crister->PositionExtent();
				instance.streaming_.texcoordMinU_ = crister->TexcoordMin().x;
				instance.streaming_.texcoordMinV_ = crister->TexcoordMin().y;
				instance.streaming_.texcoordExtent_ = crister->TexcoordExtent();
				instance.streaming_.lodLevel_ = 0;
				instance.shading_ = material.shading_;
				instance.texture_ = material.texture_;
				instance.extension_ = material.extension_;
				instance.skining_.skinIndex_ = skinned ? static_cast<Uint>(subMesh.skinIndex_) : SC_INVALID;
				instance.skining_.boneOffset_ = skinned ? boneOffset : 0;
				instance.skining_.skinVertexBufferIndex_ = crister->Residency().SkinVertexBufferIndex();
				instance.morph_.morphDeltaBufferIndex_ = SC_INVALID;
				instance.morph_.vertexMorphSourceBufferIndex_ = SC_INVALID;

				Uint32 instanceIndex = static_cast<Uint32>(instances_.size());
				instances_.push_back(instance);
				for (Uint32 meshletIndex = 0;meshletIndex < clusters[clusterIndex].meshletCount_;++meshletIndex)
				{
					meshlets_.push_back(instanceIndex);
					meshlets_.push_back(meshletIndex);
				}
			}
		}

		if (request->bones_)
		{
			for (Size nodeIndex = 0;nodeIndex < nodes.size();++nodeIndex)
			{
				Int parentIndex = nodes[nodeIndex].parentIndex_;
				if (parentIndex < 0)
				{
					continue;
				}
				Bool highlighted = request->joint_ >= 0 && (request->joint_ == static_cast<Int>(nodeIndex) || request->joint_ == parentIndex);
				PreviewLineStructuredBuffer line;
				line.start_ = Vector3::Transform(pose[parentIndex].Translation(), world);
				line.end_ = Vector3::Transform(pose[nodeIndex].Translation(), world);
				line.width_ = highlighted ? 3.0f : 2.0f;
				line.color_ = highlighted ? Color(1.0f, 0.8f, 0.1f, 1.0f) : Color(0.55f, 0.75f, 1.0f, 1.0f);
				lines_.push_back(line);
			}
		}

		if (request->axes_)
		{
			Vector3 extent = crister->PositionExtent();
			Float length = Max(Max(Max(extent.x, extent.y), extent.z) * 0.75f, 0.1f);
			StaticArray<Vector3, 3> directions = { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) };
			StaticArray<Color, 3> colors = { Color(1.0f, 0.25f, 0.25f, 1.0f), Color(0.25f, 1.0f, 0.25f, 1.0f), Color(0.3f, 0.5f, 1.0f, 1.0f) };
			for (Size axis = 0;axis < directions.size();++axis)
			{
				PreviewLineStructuredBuffer line;
				line.start_ = Vector3(0.0f, 0.0f, 0.0f);
				line.end_ = directions[axis] * length;
				line.color_ = colors[axis];
				lines_.push_back(line);
			}
		}

		lining_ = !lines_.empty();
	}

	void PreviewFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!instances_.empty())
		{
			upload.Write(Instances, RenderViewKind::Game, std::span<const ModelStructuredBuffer>(instances_));
			upload.Write(Meshlets, RenderViewKind::Game, std::span<const Uint32>(meshlets_));
		}
		if (!bones_.empty())
		{
			upload.Write(Bones, RenderViewKind::Game, std::span<const Matrix>(bones_));
		}
		if (!lines_.empty())
		{
			upload.Write(Lines, RenderViewKind::Game, std::span<const PreviewLineStructuredBuffer>(lines_));
		}
	}
}

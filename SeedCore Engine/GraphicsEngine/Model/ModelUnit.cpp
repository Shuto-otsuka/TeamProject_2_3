#include <GraphicsEngine/Model/ModelUnit.h>
#include <GraphicsEngine/Model/Material/ModelMaterial.h>
#include <GraphicsEngine/Model/ModelMesh.h>
#include <GraphicsEngine/Model/Morph/ModelMorph.h>
#include <GraphicsEngine/Model/Skin/ModelSkin.h>
#include <GraphicsEngine/Model/Softbody/ModelSoftbody.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	ModelUnit::ModelUnit()
	{
		/// No Code
	}

	ModelUnit::~ModelUnit() = default;

	void ModelUnit::Gather(const RenderScene& scene, ModelMesh& meshes, ModelMaterial& materials, ModelSkin& skins, ModelMorph& morphs, ModelSoftbody& softbodies)
	{
		updates_.clear();
		World& world = *scene.world_;
		GraphicsResourceSystem& resources = *scene.resource_;
		ModelResource& modelResource = *resources.modelResource_;

		for (Uint32 index : selected_)
		{
			selectedFlags_[index] = 0;
		}
		selected_.clear();
		Query<Read<Selected>>(world).ForEach([&](EntityID entityID, const Selected&)
			{
				if (entityID.index_ >= capacity_)
				{
					return;
				}
				if (selectedFlags_.size() <= entityID.index_)
				{
					selectedFlags_.resize(static_cast<Size>(entityID.index_) + 1, 0);
				}
				selectedFlags_[entityID.index_] = 1;
				selected_.push_back(entityID.index_);
			});

		for (Slot& slot : slots_)
		{
			slot.seen_ = false;
		}

		ComponentID boundsComponentID = ComponentRegistry::GetComponentID<Bounds>();
		Query<Read<Active>, Read<Mesh>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Mesh& mesh)
			{
				if (!active.active_ || entityID.index_ >= capacity_)
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
				const ModelMeshRecord* record = meshes.Register(cristerHandle, *crister);
				if (!record)
				{
					return;
				}

				Uint32 index = entityID.index_;
				if (slots_.size() <= index)
				{
					slots_.resize(static_cast<Size>(index) + 1);
				}
				Slot& slot = slots_[index];
				Bool reset = !slot.used_ || slot.generation_ != entityID.generation_;
				Matrix worldMatrix = actor.WorldMatrix();
				Matrix previousWorldMatrix = reset ? worldMatrix : slot.previousWorld_;

				if ((reset || slot.crister_ != cristerHandle) && boundsComponentID)
				{
					void* boundsRaw = actor.GetComponent(boundsComponentID);
					if (boundsRaw)
					{
						Bounds* bounds = static_cast<Bounds*>(boundsRaw);
						Vector3 positionExtent = crister->Residency().PlacedPositionExtent();
						bounds->center_ = crister->Residency().PlacedPositionMin() + positionExtent * 0.5f;
						bounds->extent_ = positionExtent * 0.5f;
					}
				}

				slot.used_ = true;
				slot.seen_ = true;
				slot.generation_ = entityID.generation_;
				slot.crister_ = cristerHandle;
				slot.previousWorld_ = worldMatrix;

				const Material* materialComponent = actor.GetComponent<Material>();
				std::span<const Surface> surfaces = (materialComponent && materialComponent->resolvedSurfaces_.size() == crister->Surfaces().size()) ? std::span<const Surface>(materialComponent->resolvedSurfaces_) : std::span<const Surface>(crister->Surfaces());
				Uint32 materialOffset = materials.Write(index, reset, surfaces, record->surfaceCount_, *crister, *resources.textureStreaming_);
				Bool selected = index < selectedFlags_.size() && selectedFlags_[index] != 0;

				ModelUnitStructuredBuffer unit{};
				unit.world_ = worldMatrix;
				unit.previousWorld_ = previousWorldMatrix;
				unit.meshOffset_ = record->meshOffset_;
				unit.meshCount_ = record->meshCount_;
				unit.materialOffset_ = materialOffset;
				unit.boneOffset_ = SC_INVALID;
				unit.morphWeightOffset_ = SC_INVALID;
				if (materialOffset != SC_INVALID && !softbodies.Write(index, reset, actor, *crister, worldMatrix, previousWorldMatrix, materials.Materials(index)[0], selected))
				{
					Bool historyValid = skins.Advance(index, reset, actor.GetComponent<Animator>());
					unit.boneOffset_ = skins.Write(index, historyValid, actor, *crister, *record);
					unit.morphWeightOffset_ = morphs.Write(index, historyValid, actor, *crister, *record, *resources.animationResource_);
					unit.flags_ = 1u | (selected ? 2u : 0u);
				}

				if (!slot.sent_ || std::memcmp(&unit, &slot.unit_, sizeof(ModelUnitStructuredBuffer)) != 0)
				{
					ModelUnitUpdate update;
					update.index_ = index;
					update.data_ = unit;
					updates_.push_back(update);
					slot.unit_ = unit;
					slot.sent_ = true;
				}
			});

		count_ = 0;
		for (Uint32 index = 0;index < static_cast<Uint32>(slots_.size());++index)
		{
			Slot& slot = slots_[index];
			if (slot.used_ && !slot.seen_)
			{
				materials.Release(index);
				skins.Release(index);
				morphs.Release(index);
				softbodies.Release(index);
				slot.used_ = false;
				slot.crister_ = Handle<Crister>{};
				ModelUnitStructuredBuffer unit{};
				if (!slot.sent_ || std::memcmp(&unit, &slot.unit_, sizeof(ModelUnitStructuredBuffer)) != 0)
				{
					ModelUnitUpdate update;
					update.index_ = index;
					update.data_ = unit;
					updates_.push_back(update);
					slot.unit_ = unit;
					slot.sent_ = true;
				}
			}
			if (slot.used_)
			{
				count_ = index + 1;
			}
		}

		for (Uint32 index = initializedCount_;index < count_;++index)
		{
			Slot& slot = slots_[index];
			if (slot.sent_)
			{
				continue;
			}
			ModelUnitUpdate update;
			update.index_ = index;
			updates_.push_back(update);
			slot.unit_ = update.data_;
			slot.sent_ = true;
		}
		initializedCount_ = Max(initializedCount_, count_);
	}

	void ModelUnit::Finish(const RenderScene& scene, const ModelSoftbody& softbodies)
	{
		constants_ = ModelExpandConstantBuffer{};
		constants_.viewCount_ = static_cast<Uint>(Min(scene.views_.size(), static_cast<Size>(RenderViewCount)));
		for (Uint32 viewIndex = 0;viewIndex < constants_.viewCount_;++viewIndex)
		{
			const SceneConstantBuffer& sceneConstants = scene.views_[viewIndex].scene_;
			const Matrix& viewProjection = sceneConstants.nonJitterViewProjection_;
			StaticArray<Vector4, 6> planes =
			{
				Vector4(viewProjection._14 + viewProjection._11, viewProjection._24 + viewProjection._21, viewProjection._34 + viewProjection._31, viewProjection._44 + viewProjection._41),
				Vector4(viewProjection._14 - viewProjection._11, viewProjection._24 - viewProjection._21, viewProjection._34 - viewProjection._31, viewProjection._44 - viewProjection._41),
				Vector4(viewProjection._14 + viewProjection._12, viewProjection._24 + viewProjection._22, viewProjection._34 + viewProjection._32, viewProjection._44 + viewProjection._42),
				Vector4(viewProjection._14 - viewProjection._12, viewProjection._24 - viewProjection._22, viewProjection._34 - viewProjection._32, viewProjection._44 - viewProjection._42),
				Vector4(viewProjection._13, viewProjection._23, viewProjection._33, viewProjection._43),
				Vector4(viewProjection._14 - viewProjection._13, viewProjection._24 - viewProjection._23, viewProjection._34 - viewProjection._33, viewProjection._44 - viewProjection._43),
			};
			for (Size planeIndex = 0;planeIndex < planes.size();++planeIndex)
			{
				Vector4 plane = planes[planeIndex];
				Float length = Vector3(plane.x, plane.y, plane.z).Length();
				constants_.planes_[viewIndex * 6 + planeIndex] = length > 0.0f ? plane / length : Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
			constants_.cameras_[viewIndex] = Vector4(sceneConstants.cameraPosition_.x, sceneConstants.cameraPosition_.y, sceneConstants.cameraPosition_.z, sceneConstants.projection_._22 * sceneConstants.screenSize_.y * 0.5f);
		}
		constants_.unitCount_ = count_;
		constants_.transparentBase_ = GeometryFeature::transparentBase_;
		constants_.capacity_ = GeometryFeature::instanceCapacity_;
		constants_.softbodyOpaqueCount_ = softbodies.OpaqueCount();
		constants_.softbodyTransparentCount_ = softbodies.TransparentCount();
	}

	std::span<const ModelUnitUpdate> ModelUnit::Updates()const
	{
		return updates_;
	}

	const ModelExpandConstantBuffer& ModelUnit::Constants()const
	{
		return constants_;
	}

	Uint32 ModelUnit::Count()const
	{
		return count_;
	}
}

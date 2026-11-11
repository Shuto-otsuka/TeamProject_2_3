#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Light/LightStage.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/Model/ModelResource.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/WorldAccessor.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(LightFeature, ClusterAssignStage);

	void LightFeature::Create(RenderDevice& device)
	{
		clusterPipeline_.Create(device);
	}

	void LightFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		World& world = *scene.world_;
		ModelResource& modelResource = *scene.resource_->modelResource_;
		lights_.clear();
		constants_ = LightConstantBuffer{};

		Bool hasDirectional = false;
		const CelestialResult* celestial = scene.Find<CelestialResult>();
		if (celestial)
		{
			constants_.moonIntensity_ = celestial->moonIntensity_;
			constants_.moonColor_ = Vector3(celestial->moonColor_.r, celestial->moonColor_.g, celestial->moonColor_.b);
			constants_.moonPhase_ = celestial->moonPhase_;
			constants_.moonAngularRadius_ = celestial->moonAngularRadius_;
			constants_.nightFactor_ = celestial->nightFactor_;
		}

		Query<Read<Active>, Read<DirectionalLight>> directionalQuery(world);
		directionalQuery.ForEach([&](EntityID entityID, const Active& active, const DirectionalLight& light)
			{
				if (!active.active_ || hasDirectional)
				{
					return;
				}
				hasDirectional = true;

				Vector3 direction = light.direction_;
				Actor actor = WorldAccessor::Make(world, entityID);
				if (actor)
				{
					direction = Vector3::TransformNormal(direction, actor.WorldMatrix());
				}
				direction.Normalize();

				constants_.sunDirection_ = direction;
				constants_.sunIntensity_ = light.intensity_;
				constants_.sunColor_ = Vector3(light.color_.r, light.color_.g, light.color_.b);
			});

		Bool hasSky = false;
		Query<Read<Active>, Read<SkyLight>> skyQuery(world);
		skyQuery.ForEach([&](const Active& active, const SkyLight& light)
			{
				if (!active.active_ || hasSky)
				{
					return;
				}
				hasSky = true;
				constants_.skyIntensity_ = light.intensity_;
			});

		Query<Read<Active>, Read<PointLight>> pointQuery(world);
		pointQuery.ForEach([&](EntityID entityID, const Active& active, const PointLight& light)
			{
				if (!active.active_)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				LightStructuredBuffer entry{};
				entry.type_ = LightType::Point;
				entry.position_ = actor.WorldMatrix().Translation();
				entry.range_ = light.range_;
				entry.color_ = Vector3(light.color_.r, light.color_.g, light.color_.b);
				entry.intensity_ = light.intensity_;
				lights_.push_back(entry);
			});

		Query<Read<Active>, Read<SpotLight>> spotQuery(world);
		spotQuery.ForEach([&](EntityID entityID, const Active& active, const SpotLight& light)
			{
				if (!active.active_)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				Matrix worldMatrix = actor.WorldMatrix();
				Vector3 direction = Vector3::TransformNormal(light.direction_, worldMatrix);
				direction.Normalize();

				LightStructuredBuffer entry{};
				entry.type_ = LightType::Spot;
				entry.position_ = worldMatrix.Translation();
				entry.range_ = light.range_;
				entry.direction_ = direction;
				entry.cosHalfAngle_ = Cos(ToRadians(light.spotAngle_ * 0.5f));
				entry.color_ = Vector3(light.color_.r, light.color_.g, light.color_.b);
				entry.intensity_ = light.intensity_;
				entry.softness_ = light.softness_;
				lights_.push_back(entry);
			});

		Query<Read<Active>, Read<RectangleLight>> rectQuery(world);
		rectQuery.ForEach([&](EntityID entityID, const Active& active, const RectangleLight& light)
			{
				if (!active.active_)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				Matrix worldMatrix = actor.WorldMatrix();
				Vector3 normal = Vector3::TransformNormal(light.direction_, worldMatrix);
				normal.Normalize();
				Vector3 upHint = Vector3::TransformNormal(light.up_, worldMatrix);
				Vector3 right = upHint.Cross(normal);
				if (right.LengthSquared() < 1e-6f)
				{
					right = Vector3::Right;
				}
				right.Normalize();
				Vector3 up = normal.Cross(right);
				up.Normalize();

				LightStructuredBuffer entry{};
				entry.type_ = LightType::Rect;
				entry.position_ = worldMatrix.Translation();
				entry.range_ = light.range_;
				entry.direction_ = normal;
				entry.right_ = right;
				entry.halfWidth_ = light.width_ * 0.5f;
				entry.up_ = up;
				entry.halfHeight_ = light.height_ * 0.5f;
				entry.color_ = Vector3(light.color_.r, light.color_.g, light.color_.b);
				entry.intensity_ = light.intensity_;
				lights_.push_back(entry);
			});

		Query<Read<Active>, Read<Mesh>> modelQuery(world);
		modelQuery.ForEach([&](EntityID entityID, const Active& active, const Mesh& mesh)
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
				if (!crister || crister->Lights().empty())
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				Matrix worldMatrix = actor.WorldMatrix();
				for (const PunctualLight& light : crister->Lights())
				{
					Vector3 direction = Vector3::TransformNormal(light.direction_, worldMatrix);
					direction.Normalize();

					LightStructuredBuffer entry{};
					entry.position_ = Vector3::Transform(light.position_, worldMatrix);
					entry.range_ = light.range_;
					entry.color_ = Vector3(light.colorRGB_[0], light.colorRGB_[1], light.colorRGB_[2]);
					entry.intensity_ = light.intensity_;
					if (light.type_ == PunctualLight::Type::Point)
					{
						entry.type_ = LightType::Point;
					}
					else
					{
						entry.type_ = LightType::Spot;
						entry.direction_ = direction;
						entry.cosHalfAngle_ = Cos(light.outerConeAngle_);
						if (light.outerConeAngle_ > 0.0f)
						{
							entry.softness_ = Clamp((light.outerConeAngle_ - light.innerConeAngle_) / light.outerConeAngle_, 0.0f, 1.0f);
						}
					}
					lights_.push_back(entry);
				}
			});

		constants_.lightCount_ = static_cast<Uint>(lights_.size());
	}

	void LightFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!lights_.empty())
		{
			upload.Write(List, RenderViewKind::Game, std::span<const LightStructuredBuffer>(lights_));
		}
		upload.Write(Constants, RenderViewKind::Game, std::span<const LightConstantBuffer>(&constants_, 1));
	}
}

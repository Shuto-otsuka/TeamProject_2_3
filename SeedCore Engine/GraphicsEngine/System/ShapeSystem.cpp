#include <GraphicsEngine/System/ShapeSystem.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Shape/Primitive/BoxShape.h>
#include <GraphicsEngine/Shape/Primitive/SphereShape.h>
#include <GraphicsEngine/Shape/Primitive/CylinderShape.h>
#include <GraphicsEngine/Shape/Primitive/ConeShape.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/Interop/QueryInstance.h>
#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	void ShapeSystem::Update(World& world, Bool visible, Float aspectRatio)
	{
		shapes_.clear();

		const Color cameraColor(0.50f, 0.70f, 1.00f, 1.0f);
		const Color lightColor(1.00f, 0.82f, 0.35f, 1.0f);
		const Color audioColor(0.44f, 0.88f, 0.71f, 1.0f);

		DynamicArray<EntityID> cameraEntities;
		Query<Read<Camera>>(world).ForEach([&](EntityID entityID, const Camera&) { cameraEntities.push_back(entityID); });
		for (EntityID id : cameraEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const Camera* camera = actor.GetComponent<Camera>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			Vector3 eye = position ? Transform::Vector(*position) : Vector3(0.0f, 0.0f, 0.0f);
			Quaternion orientation = rotation ? Transform::Quat(*rotation) : Quaternion::Identity;

			Float length = Min(camera->farPlane_, frustumLength_);
			Float halfHeight = length * Tan(ToRadians(camera->fieldOfView_) * 0.5f);
			Float halfWidth = halfHeight * aspectRatio;

			Vector3 corners[4] =
			{
				eye + Vector3::Transform(Vector3(-halfWidth, -halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(halfWidth, -halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(halfWidth, halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(-halfWidth, halfHeight, -length), orientation),
			};

			for (Size cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
			{
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, eye, Quaternion::Identity, corners[cornerIndex] - eye, cameraColor });
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, corners[cornerIndex], Quaternion::Identity, corners[(cornerIndex + 1) % 4] - corners[cornerIndex], cameraColor });
			}
		}

		Float brainFieldOfView = 60.0f;
		for (EntityID id : cameraEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			const Camera* camera = actor ? actor.GetComponent<Camera>() : nullptr;
			if (camera && actor.Active() && camera->isActive_)
			{
				brainFieldOfView = camera->fieldOfView_;
				break;
			}
		}

		DynamicArray<EntityID> cameraBrainEntities;
		Query<Read<CameraBrain>>(world).ForEach([&](EntityID entityID, const CameraBrain&) { cameraBrainEntities.push_back(entityID); });
		for (EntityID id : cameraBrainEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active() || actor.GetComponent<Camera>())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			Vector3 eye = position ? Transform::Vector(*position) : Vector3(0.0f, 0.0f, 0.0f);
			Quaternion orientation = rotation ? Transform::Quat(*rotation) : Quaternion::Identity;

			Float length = frustumLength_;
			Float halfHeight = length * Tan(ToRadians(brainFieldOfView) * 0.5f);
			Float halfWidth = halfHeight * aspectRatio;

			Vector3 corners[4] =
			{
				eye + Vector3::Transform(Vector3(-halfWidth, -halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(halfWidth, -halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(halfWidth, halfHeight, -length), orientation),
				eye + Vector3::Transform(Vector3(-halfWidth, halfHeight, -length), orientation),
			};

			for (Size cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
			{
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, eye, Quaternion::Identity, corners[cornerIndex] - eye, cameraColor });
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, corners[cornerIndex], Quaternion::Identity, corners[(cornerIndex + 1) % 4] - corners[cornerIndex], cameraColor });
			}
		}

		DynamicArray<EntityID> pointLightEntities;
		Query<Read<PointLight>>(world).ForEach([&](EntityID entityID, const PointLight&) { pointLightEntities.push_back(entityID); });
		for (EntityID id : pointLightEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const PointLight* light = actor.GetComponent<PointLight>();

			shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actor.WorldMatrix().Translation(), Quaternion::Identity, Vector3(light->range_, 0.0f, 0.0f), lightColor });
		}

		DynamicArray<EntityID> spotLightEntities;
		Query<Read<SpotLight>>(world).ForEach([&](EntityID entityID, const SpotLight&) { spotLightEntities.push_back(entityID); });
		for (EntityID id : spotLightEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const SpotLight* light = actor.GetComponent<SpotLight>();

			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 apex = worldMatrix.Translation();
			Vector3 direction = Vector3::TransformDirection(light->direction_, worldMatrix);
			direction.Normalize();

			Quaternion coneRotation = Quaternion::FromToRotation(Vector3::Up, -direction);

			Float outerHalfAngle = ToRadians(light->spotAngle_ * 0.5f);
			Float outerHeight = light->range_ * Cos(outerHalfAngle);
			Float outerRadius = light->range_ * Sin(outerHalfAngle);
			shapes_.push_back({ ShapeKind::Cone, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, apex + direction * (outerHeight * 0.5f), coneRotation, Vector3(outerRadius, outerHeight * 0.5f, 0.0f), lightColor });

			if (light->softness_ > 0.0f)
			{
				Float outerCos = Cos(outerHalfAngle);
				Float innerHalfAngle = Acos(Clamp(outerCos + light->softness_ * (1.0f - outerCos), -1.0f, 1.0f));
				Float innerHeight = light->range_ * Cos(innerHalfAngle);
				Float innerRadius = light->range_ * Sin(innerHalfAngle);
				shapes_.push_back({ ShapeKind::Cone, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, apex + direction * (innerHeight * 0.5f), coneRotation, Vector3(innerRadius, innerHeight * 0.5f, 0.0f), lightColor });
			}
		}

		DynamicArray<EntityID> directionalLightEntities;
		Query<Read<DirectionalLight>>(world).ForEach([&](EntityID entityID, const DirectionalLight&) { directionalLightEntities.push_back(entityID); });
		for (EntityID id : directionalLightEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const DirectionalLight* light = actor.GetComponent<DirectionalLight>();

			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 origin = worldMatrix.Translation();
			Vector3 direction = Vector3::TransformDirection(light->direction_, worldMatrix);
			direction.Normalize();

			Vector3 side = (Abs(direction.y) > 0.99f ? Vector3::Right : Vector3::Up).Cross(direction);
			side.Normalize();
			Vector3 up = direction.Cross(side);

			shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, origin, Quaternion::FromToRotation(Vector3::UnitZ, direction), Vector3(directionalRadius_, 0.0f, 0.0f), lightColor });
			for (Uint rayIndex = 0; rayIndex < directionalRayCount_; ++rayIndex)
			{
				Float angle = static_cast<Float>(rayIndex) / static_cast<Float>(directionalRayCount_) * Pi<Float>::Two;
				Vector3 start = origin + (side * Cos(angle) + up * Sin(angle)) * directionalRadius_;
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, start, Quaternion::Identity, direction * directionalRayLength_, lightColor });
			}
		}

		DynamicArray<EntityID> rectangleLightEntities;
		Query<Read<RectangleLight>>(world).ForEach([&](EntityID entityID, const RectangleLight&) { rectangleLightEntities.push_back(entityID); });
		for (EntityID id : rectangleLightEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const RectangleLight* light = actor.GetComponent<RectangleLight>();

			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 center = worldMatrix.Translation();
			Vector3 normal = Vector3::TransformDirection(light->direction_, worldMatrix);
			normal.Normalize();
			Vector3 upHint = Vector3::TransformDirection(light->up_, worldMatrix);
			Vector3 right = upHint.Cross(normal);
			if (right.LengthSq() < 1e-6f)
			{
				right = Vector3::Right;
			}
			right.Normalize();
			Vector3 up = normal.Cross(right);
			up.Normalize();

			Vector3 halfRight = right * (light->width_ * 0.5f);
			Vector3 halfUp = up * (light->height_ * 0.5f);
			Vector3 corners[4] =
			{
				center - halfRight - halfUp,
				center + halfRight - halfUp,
				center + halfRight + halfUp,
				center - halfRight + halfUp,
			};

			for (Size cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
			{
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, corners[cornerIndex], Quaternion::Identity, corners[(cornerIndex + 1) % 4] - corners[cornerIndex], lightColor });
			}

			shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, center, Quaternion::Identity, normal * light->range_, lightColor, worldArrowHeadLength_ });
		}

		DynamicArray<EntityID> audioSourceEntities;
		Query<Read<AudioSource>>(world).ForEach([&](EntityID entityID, const AudioSource&) { audioSourceEntities.push_back(entityID); });
		for (EntityID id : audioSourceEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && !actor.HasComponent<Selected>())
			{
				continue;
			}

			const AudioSource* source = actor.GetComponent<AudioSource>();

			if (!source->spatial_)
			{
				continue;
			}

			Vector3 center = actor.WorldMatrix().Translation();
			shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, center, Quaternion::Identity, Vector3(source->minDistance_, 0.0f, 0.0f), audioColor });
			shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, center, Quaternion::Identity, Vector3(source->maxDistance_, 0.0f, 0.0f), audioColor });
		}

		auto gatherPrimitiveShapes = [&]<typename T>(std::type_identity<T>, ShapeKind kind, auto dimensions)
		{
			Query<Read<Active>, Read<T>> query(world);
			query.ForEach([&](EntityID entityID, const Active& active, const T& component)
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
					Vector3 translation, scale;
					Quaternion rotation;
					worldMatrix.Decompose(translation, rotation, scale);

					ShapeDesc shape{};
					shape.kind_ = kind;
					shape.style_ = ShapeStyle::Solid;
					shape.space_ = ShapeSpace::World;
					shape.scope_ = ShapeScope::Game;
					shape.position_ = translation;
					shape.rotation_ = rotation;
					shape.dimensions_ = dimensions(component, scale);
					shape.color_ = component.color_;
					shape.textureID_ = component.textureID_;
					shape.uvScale_ = component.uvScale_;
					shape.uvOffset_ = component.uvOffset_;
					shapes_.push_back(shape);
				});
		};

		gatherPrimitiveShapes(std::type_identity<BoxShape>{}, ShapeKind::Box, [](const BoxShape& box, const Vector3& scale)
			{
				return Vector3(box.size_.x * Abs(scale.x) * 0.5f, box.size_.y * Abs(scale.y) * 0.5f, box.size_.z * Abs(scale.z) * 0.5f);
			});

		gatherPrimitiveShapes(std::type_identity<SphereShape>{}, ShapeKind::Sphere, [](const SphereShape& sphere, const Vector3& scale)
			{
				return Vector3(sphere.radius_ * Max(Max(Abs(scale.x), Abs(scale.y)), Abs(scale.z)), 0.0f, 0.0f);
			});

		gatherPrimitiveShapes(std::type_identity<CylinderShape>{}, ShapeKind::Cylinder, [](const CylinderShape& cylinder, const Vector3& scale)
			{
				return Vector3(cylinder.radius_ * Max(Abs(scale.x), Abs(scale.z)), cylinder.height_ * Abs(scale.y) * 0.5f, 0.0f);
			});

		gatherPrimitiveShapes(std::type_identity<ConeShape>{}, ShapeKind::Cone, [](const ConeShape& cone, const Vector3& scale)
			{
				return Vector3(cone.radius_ * Max(Abs(scale.x), Abs(scale.z)), cone.height_ * Abs(scale.y) * 0.5f, 0.0f);
			});

#if SC_DEBUG || SC_DEVELOP
		const Color missColor(0.30f, 0.90f, 0.30f, 1.0f);
		const Color hitColor(1.00f, 0.30f, 0.30f, 1.0f);
		QueryInstance& queryInstance = *world.GetService<QueryInstance>();
		for (const QueryDesc& query : queryInstance.Queries())
		{
			const Color& color = query.hit_ ? hitColor : missColor;

			if (query.kind_ == QueryKind::Raycast)
			{
				Float length = query.hit_ ? query.hitDistance_ : query.distance_;
				shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_, Quaternion::Identity, query.direction_ * length, color, worldArrowHeadLength_ });
				if (query.hit_)
				{
					shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, Vector3(queryHitRadius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, query.hitNormal_ * queryNormalLength_, color, worldArrowHeadLength_ });
				}
			}
			else if (query.kind_ == QueryKind::Spherecast)
			{
				Float length = query.hit_ ? query.hitDistance_ : query.distance_;
				Quaternion sweepRotation = query.direction_.LengthSq() > 0.0f ? Quaternion::FromToRotation(Vector3::Up, query.direction_) : Quaternion::Identity;
				shapes_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_ + query.direction_ * (length * 0.5f), sweepRotation, Vector3(query.radius_, length * 0.5f, 0.0f), color });
				if (query.hit_)
				{
					shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, Vector3(queryHitRadius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, query.hitNormal_ * queryNormalLength_, color, worldArrowHeadLength_ });
				}
			}
			else if (query.kind_ == QueryKind::Boxcast)
			{
				Float length = query.hit_ ? query.hitDistance_ : query.distance_;
				Vector3 end = query.origin_ + query.direction_ * length;
				shapes_.push_back({ ShapeKind::Box, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_, query.rotation_, query.dimensions_, color });
				shapes_.push_back({ ShapeKind::Box, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, end, query.rotation_, query.dimensions_, color });
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_, Quaternion::Identity, end - query.origin_, color });
				if (query.hit_)
				{
					shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, Vector3(queryHitRadius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, query.hitNormal_ * queryNormalLength_, color, worldArrowHeadLength_ });
				}
			}
			else if (query.kind_ == QueryKind::Overlap)
			{
				shapes_.push_back({ static_cast<ShapeKind>(static_cast<Uint32>(query.shape_)), ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_, query.rotation_, query.dimensions_, color });
			}
			else
			{
				Vector3 origin(100000.0f + query.origin_.x, 100000.0f + (ScResolution::SC_CANVAS.Height - query.origin_.y), 100000.0f);
				Vector3 direction(query.direction_.x, -query.direction_.y, 0.0f);
				Vector3 hitPoint(100000.0f + query.hitPoint_.x, 100000.0f + (ScResolution::SC_CANVAS.Height - query.hitPoint_.y), 100000.0f);
				Vector3 hitNormal(query.hitNormal_.x, -query.hitNormal_.y, 0.0f);

				if (query.kind_ == QueryKind::Raycast2D)
				{
					Float length = query.hit_ ? query.hitDistance_ : query.distance_;
					shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, Quaternion::Identity, direction * length, color, canvasArrowHeadLength_ });
					if (query.hit_)
					{
						shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, Vector3(queryHitPixels_, 0.0f, 0.0f), color });
						shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, hitNormal * queryNormalPixels_, color, canvasArrowHeadLength_ });
					}
				}
				else if (query.kind_ == QueryKind::Circlecast2D)
				{
					Float length = query.hit_ ? query.hitDistance_ : query.distance_;
					Vector3 end = origin + direction * length;
					Vector3 edge = Vector3(-direction.y, direction.x, 0.0f) * query.radius_;
					shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, Quaternion::Identity, Vector3(query.radius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, end, Quaternion::Identity, Vector3(query.radius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin + edge, Quaternion::Identity, end - origin, color });
					shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin - edge, Quaternion::Identity, end - origin, color });
					if (query.hit_)
					{
						shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, Vector3(queryHitPixels_, 0.0f, 0.0f), color });
						shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, hitNormal * queryNormalPixels_, color, canvasArrowHeadLength_ });
					}
				}
				else if (query.kind_ == QueryKind::Boxcast2D)
				{
					Float length = query.hit_ ? query.hitDistance_ : query.distance_;
					Vector3 end = origin + direction * length;

					Quaternion rotation(-query.rotation_.x, -query.rotation_.y, -query.rotation_.z, query.rotation_.w);
					shapes_.push_back({ ShapeKind::Rect, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, rotation, query.dimensions_, color });
					shapes_.push_back({ ShapeKind::Rect, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, end, rotation, query.dimensions_, color });
					shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, Quaternion::Identity, end - origin, color });
					if (query.hit_)
					{
						shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, Vector3(queryHitPixels_, 0.0f, 0.0f), color });
						shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, hitPoint, Quaternion::Identity, hitNormal * queryNormalPixels_, color, canvasArrowHeadLength_ });
					}
				}
				else if (query.kind_ == QueryKind::Overlap2D)
				{
					Quaternion rotation(-query.rotation_.x, -query.rotation_.y, -query.rotation_.z, query.rotation_.w);
					shapes_.push_back({ static_cast<ShapeKind>(static_cast<Uint32>(query.shape_)), ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, rotation, query.dimensions_, color });
				}
			}
		}
		queryInstance.Clear();

		RenderInstance& renderInstance = *world.GetService<RenderInstance>();
		std::span<const ShapeDesc> requestedShapes = renderInstance.Shapes();
		shapes_.insert(shapes_.end(), requestedShapes.begin(), requestedShapes.end());
		renderInstance.Clear();
#endif
	}

	std::span<const ShapeDesc> ShapeSystem::Shapes()const
	{
		return shapes_;
	}
}

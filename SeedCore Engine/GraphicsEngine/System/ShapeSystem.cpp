#include <GraphicsEngine/System/ShapeSystem.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Shape/Primitive/BoxShape.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	/**
	* [EN]
	* Rebuilds the shape list from the World. The debug display covers
	* only the selected actors, or every actor while visible is on.
	* aspectRatio is the game view's width over height, for camera
	* frustums.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* World から形の一覧を作り直す。デバッグ表示は選択中のアクターだけ、
	* visible がオンなら全アクターが対象。aspectRatio はゲームビューの
	* 幅÷高さで、カメラの視錐台に使う。
	*/
	void ShapeSystem::Update(World& world, std::span<const Entity> selectedEntities, Bool visible, Float aspectRatio)
	{
		shapes_.clear();

		/// [EN] The same colors as the editor view's icons for each kind.
		/// [JP] 種類ごとに、エディタービューのアイコンと同じ色にする。
		const Color cameraColor(0.50f, 0.70f, 1.00f, 1.0f);
		const Color lightColor(1.00f, 0.82f, 0.35f, 1.0f);
		const Color audioColor(0.44f, 0.88f, 0.71f, 1.0f);

		for (EntityID id : world.GetComponents<Camera>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const Camera* camera = actor.GetComponent<Camera>();
			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			/// [EN] Same pose as CameraSystem: the actor's own Position and Rotation without its parents, looking down -Z.
			/// [JP] CameraSystem と同じ姿勢。親の変換を含まないアクター自身の Position と Rotation で、-Z を向く。
			Vector3 eye = position ? Transform::Vector(*position) : Vector3(0.0f, 0.0f, 0.0f);
			Quaternion orientation = rotation ? Transform::Quat(*rotation) : Quaternion::Identity;

			/// [EN] The far rectangle sits at the far plane or frustumLength_, whichever is nearer; the near plane is too close to see and is left out.
			/// [JP] 奥の四角は far 面と frustumLength_ の近い方に置く。near 面は近すぎて見えないので描かない。
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

			/// [EN] Four edges from the eye to the far corners, then the far rectangle.
			/// [JP] 視点から奥の4隅への4本と、奥の四角。
			for (Size cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
			{
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, eye, Quaternion::Identity, corners[cornerIndex] - eye, cameraColor });
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, corners[cornerIndex], Quaternion::Identity, corners[(cornerIndex + 1) % 4] - corners[cornerIndex], cameraColor });
			}
		}

		/// [EN] A brain renders with the lens of the first active Camera and the render aspect ratio (see CameraSystem::SyncCameraBrains), so its frustum is built from the same values here rather than from the brain's own state.
		/// [JP] ブレインは、最初に見つかったアクティブな Camera のレンズと描画の縦横比で描く（CameraSystem::SyncCameraBrains 参照）。そのため視錐台は、ブレイン自身の状態ではなく、ここで同じ値を求めて作る。
		Float brainFieldOfView = 60.0f;
		for (EntityID id : world.GetComponents<Camera>())
		{
			Actor actor = world.GetActor(id);
			const Camera* camera = actor ? actor.GetComponent<Camera>() : nullptr;
			if (camera && actor.Active() && camera->isActive_)
			{
				brainFieldOfView = camera->fieldOfView_;
				break;
			}
		}

		for (EntityID id : world.GetComponents<CameraBrain>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active() || actor.GetComponent<Camera>())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const Position* position = actor.GetComponent<Position>();
			const Rotation* rotation = actor.GetComponent<Rotation>();

			Vector3 eye = position ? Transform::Vector(*position) : Vector3(0.0f, 0.0f, 0.0f);
			Quaternion orientation = rotation ? Transform::Quat(*rotation) : Quaternion::Identity;

			/// [EN] A brain has no far plane of its own, so the frustum is drawn frustumLength_ deep.
			/// [JP] ブレインは自前の far 面を持たないので、視錐台は frustumLength_ の奥行きで描く。
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

		for (EntityID id : world.GetComponents<PointLight>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const PointLight* light = actor.GetComponent<PointLight>();

			/// [EN] Same position as LightSystem: the translation of the actor's world matrix.
			/// [JP] LightSystem と同じ位置。アクターのワールド行列の平行移動。
			shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, actor.WorldMatrix().Translation(), Quaternion::Identity, Vector3(light->range_, 0.0f, 0.0f), lightColor });
		}

		for (EntityID id : world.GetComponents<SpotLight>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const SpotLight* light = actor.GetComponent<SpotLight>();

			/// [EN] Same position and direction as LightSystem: direction_ is local and turned by the world matrix.
			/// [JP] LightSystem と同じ位置と向き。direction_ はローカルで、ワールド行列で回す。
			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 apex = worldMatrix.Translation();
			Vector3 direction = Vector3::TransformNormal(light->direction_, worldMatrix);
			direction.Normalize();

			/// [EN] The Cone shape runs along +Y with its apex on top, so +Y is turned to point back at the light.
			/// [JP] Cone の形は +Y 方向に伸び、頂点が上にあるので、+Y がライトの方へ戻る向きになるよう回す。
			Quaternion coneRotation = Quaternion::FromToRotation(Vector3::Up, -direction);

			/// [EN] spotAngle_ is the full angle; range_ is measured along the cone's side, so the cone's height and base radius follow the half angle.
			/// [JP] spotAngle_ は全角。range_ は円錐の側面に沿った長さなので、高さと底面の半径は半角から決まる。
			Float outerHalfAngle = ToRadians(light->spotAngle_ * 0.5f);
			Float outerHeight = light->range_ * Cos(outerHalfAngle);
			Float outerRadius = light->range_ * Sin(outerHalfAngle);
			shapes_.push_back({ ShapeKind::Cone, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, apex + direction * (outerHeight * 0.5f), coneRotation, Vector3(outerRadius, outerHeight * 0.5f, 0.0f), lightColor });

			/// [EN] Inner cone: where the spot fade reaches full strength, cos(angle) = cos(outer half angle) + softness * (1 - cos(outer half angle)), the same formula the lighting shaders use.
			/// [JP] 内側の円錐：スポットの減衰が最大になる角度。cos(角度) = cos(外側の半角) + softness × (1 - cos(外側の半角)) で、ライティングのシェーダーと同じ式。
			if (light->softness_ > 0.0f)
			{
				Float outerCos = Cos(outerHalfAngle);
				Float innerHalfAngle = Acos(Clamp(outerCos + light->softness_ * (1.0f - outerCos), -1.0f, 1.0f));
				Float innerHeight = light->range_ * Cos(innerHalfAngle);
				Float innerRadius = light->range_ * Sin(innerHalfAngle);
				shapes_.push_back({ ShapeKind::Cone, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, apex + direction * (innerHeight * 0.5f), coneRotation, Vector3(innerRadius, innerHeight * 0.5f, 0.0f), lightColor });
			}
		}

		for (EntityID id : world.GetComponents<DirectionalLight>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const DirectionalLight* light = actor.GetComponent<DirectionalLight>();

			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 origin = worldMatrix.Translation();
			Vector3 direction = Vector3::TransformNormal(light->direction_, worldMatrix);
			direction.Normalize();

			/// [EN] Two axes across the light direction; Y is crossed with it unless the light points nearly straight up or down, in which case X is used.
			/// [JP] 光の向きに垂直な2本の軸。光がほぼ真上か真下を向くとき以外は Y と外積を取り、そのときは X を使う。
			Vector3 side = (Abs(direction.y) > 0.99f ? Vector3::Right : Vector3::Up).Cross(direction);
			side.Normalize();
			Vector3 up = direction.Cross(side);

			/// [EN] A ring facing the light direction, with parallel rays running from evenly spaced points on it (the same look as Unity's directional light), so the light reads as parallel rays rather than one source. The Circle shape lies on its local XY plane, so its +Z is turned to the light direction.
			/// [JP] 光の向きに垂直な円と、その円周上に等間隔に並べた点から伸びる平行な光線（Unity の平行光源と同じ見た目）。1つの光源ではなく平行な光線だと分かるようにする。Circle の形はローカルの XY 平面にあるので、その +Z を光の向きに回す。
			shapes_.push_back({ ShapeKind::Circle, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, origin, Quaternion::FromToRotation(Vector3::UnitZ, direction), Vector3(directionalRadius_, 0.0f, 0.0f), lightColor });
			for (Uint rayIndex = 0; rayIndex < directionalRayCount_; ++rayIndex)
			{
				Float angle = static_cast<Float>(rayIndex) / static_cast<Float>(directionalRayCount_) * Pi<Float>::Two;
				Vector3 start = origin + (side * Cos(angle) + up * Sin(angle)) * directionalRadius_;
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, start, Quaternion::Identity, direction * directionalRayLength_, lightColor });
			}
		}

		for (EntityID id : world.GetComponents<RectangleLight>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const RectangleLight* light = actor.GetComponent<RectangleLight>();

			/// [EN] Same basis as LightSystem: normal is the facing direction, right and up run along the rectangle's edges, and right falls back to +X when up_ is nearly parallel to the normal.
			/// [JP] LightSystem と同じ基底。normal が正面、right と up が四角の辺の向き。up_ が normal とほぼ平行なら right は +X にする。
			Matrix worldMatrix = actor.WorldMatrix();
			Vector3 center = worldMatrix.Translation();
			Vector3 normal = Vector3::TransformNormal(light->direction_, worldMatrix);
			normal.Normalize();
			Vector3 upHint = Vector3::TransformNormal(light->up_, worldMatrix);
			Vector3 right = upHint.Cross(normal);
			if (right.LengthSquared() < 1e-6f)
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

			/// [EN] The emitting rectangle, and an arrow out of its face as long as the light's range.
			/// [JP] 発光面の四角と、面から出る、ライトの範囲の長さの矢印。
			for (Size cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
			{
				shapes_.push_back({ ShapeKind::Segment, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, corners[cornerIndex], Quaternion::Identity, corners[(cornerIndex + 1) % 4] - corners[cornerIndex], lightColor });
			}

			shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Editor, center, Quaternion::Identity, normal * light->range_, lightColor, worldArrowHeadLength_ });
		}

		for (EntityID id : world.GetComponents<AudioSource>())
		{
			Actor actor = world.GetActor(id);
			if (!actor || !actor.Active())
			{
				continue;
			}
			if (!visible && std::ranges::find(selectedEntities, actor.GetEntity()) == selectedEntities.end())
			{
				continue;
			}

			const AudioSource* source = actor.GetComponent<AudioSource>();

			/// [EN] The attenuation distances apply only while the source is spatial.
			/// [JP] 減衰の距離は、立体音響がオンのときだけ効く。
			if (!source->spatial_)
			{
				continue;
			}

			/// [EN] Where attenuation starts and where it ends.
			/// [JP] 減衰が始まる距離と、終わる距離。
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

					Actor actor = world.GetActor(entityID);
					if (!actor)
					{
						return;
					}

					Matrix worldMatrix = actor.WorldMatrix();
					Vector3 translation, scale;
					Quaternion rotation;
					worldMatrix.Decompose(scale, rotation, translation);

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

		/// [EN] The unit box spans -1 to 1, so the dimensions are half the scaled size.
		/// [JP] 単位の箱は -1 から 1 なので、大きさはスケール後のサイズの半分。
		gatherPrimitiveShapes(std::type_identity<BoxShape>{}, ShapeKind::Box, [](const BoxShape& box, const Vector3& scale)
			{
				return Vector3(box.size_.x * Abs(scale.x) * 0.5f, box.size_.y * Abs(scale.y) * 0.5f, box.size_.z * Abs(scale.z) * 0.5f);
			});

#ifdef _DEBUG
		/// [EN] Physics queries recorded since the last frame, in the colors of Unity's physics debugger: green when nothing was found, red when something was. They are read and then cleared, so each query is drawn once. 3D queries are drawn in the game view as well; 2D queries stay on the canvas.
		/// [JP] 前のフレームから記録された物理クエリ。Unity の物理デバッガーと同じく、何にも当たらなければ緑、当たれば赤で描く。読んだあと空にするので、各クエリは1回だけ描かれる。3D のクエリはゲームビューにも描き、2D のクエリは Canvas にだけ描く。
		const Color missColor(0.30f, 0.90f, 0.30f, 1.0f);
		const Color hitColor(1.00f, 0.30f, 0.30f, 1.0f);
		QueryInstance& queryInstance = world.GetQueryInstance();
		for (const QueryDesc& query : queryInstance.Queries())
		{
			const Color& color = query.hit_ ? hitColor : missColor;

			if (query.kind_ == QueryKind::Raycast)
			{
				/// [EN] The ray up to where it stopped, then a marker and the surface normal at the hit.
				/// [JP] 止まったところまでのレイと、当たった点の目印と面の法線。
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
				/// [EN] The volume the sphere swept up to where it stopped, as a capsule along the direction (the Capsule shape runs along +Y), then a marker and the surface normal at the hit.
				/// [JP] 球が止まったところまでになぞった範囲を、向きに沿ったカプセルで描く（Capsule の形は +Y 方向に伸びる）。当たった点の目印と面の法線も描く。
				Float length = query.hit_ ? query.hitDistance_ : query.distance_;
				Quaternion sweepRotation = query.direction_.LengthSquared() > 0.0f ? Quaternion::FromToRotation(Vector3::Up, query.direction_) : Quaternion::Identity;
				shapes_.push_back({ ShapeKind::Capsule, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_ + query.direction_ * (length * 0.5f), sweepRotation, Vector3(query.radius_, length * 0.5f, 0.0f), color });
				if (query.hit_)
				{
					shapes_.push_back({ ShapeKind::Sphere, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, Vector3(queryHitRadius_, 0.0f, 0.0f), color });
					shapes_.push_back({ ShapeKind::Arrow, ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.hitPoint_, Quaternion::Identity, query.hitNormal_ * queryNormalLength_, color, worldArrowHeadLength_ });
				}
			}
			else if (query.kind_ == QueryKind::Overlap)
			{
				/// [EN] The tested shape where it stood; ColliderKind and ShapeKind share their numbers, so the kind carries over as is.
				/// [JP] 調べた形をその場所に描く。ColliderKind と ShapeKind は番号が共通なので、種類はそのまま移せる。
				shapes_.push_back({ static_cast<ShapeKind>(static_cast<Uint32>(query.shape_)), ShapeStyle::Wireframe, ShapeSpace::World, ShapeScope::Game, query.origin_, query.rotation_, query.dimensions_, color });
			}
			else
			{
				/// [EN] 2D queries are in canvas pixels with Y down; the canvas view draws at the same placement as the 2D colliders (offset by 100000, Y flipped), so positions and directions are converted to it.
				/// [JP] 2D のクエリは Y 下向きの Canvas ピクセル。Canvas ビューは 2D のコライダーと同じ置き方（100000 ずらし、Y を反転）で描くので、位置と向きをそれに直す。
				Vector3 origin(100000.0f + query.origin_.x, 100000.0f + (ScResolution::SC_CANVAS.Height - query.origin_.y), 100000.0f);
				Vector3 direction(query.direction_.x, -query.direction_.y, 0.0f);
				Vector3 hitPoint(100000.0f + query.hitPoint_.x, 100000.0f + (ScResolution::SC_CANVAS.Height - query.hitPoint_.y), 100000.0f);
				Vector3 hitNormal(query.hitNormal_.x, -query.hitNormal_.y, 0.0f);

				if (query.kind_ == QueryKind::Raycast2D)
				{
					/// [EN] The ray up to where it stopped, then a marker and the surface normal at the hit.
					/// [JP] 止まったところまでのレイと、当たった点の目印と面の法線。
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
					/// [EN] The area the circle swept up to where it stopped: the circle at both ends joined by its two edges, then a marker and the surface normal at the hit.
					/// [JP] 円が止まったところまでになぞった範囲。両端の円と、それをつなぐ2本の縁。当たった点の目印と面の法線も描く。
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
				else if (query.kind_ == QueryKind::Overlap2D)
				{
					/// [EN] The tested shape where it stood. The recorded rotation turns about Z by the canvas angle with Y down, so with Y flipped it turns the other way, as the 2D colliders do.
					/// [JP] 調べた形をその場所に描く。記録された回転は Y 下向きでの Canvas の角度だけ Z 軸まわりに回るので、Y を反転した描画では、2D のコライダーと同じく逆向きに回す。
					Quaternion rotation(-query.rotation_.x, -query.rotation_.y, -query.rotation_.z, query.rotation_.w);
					shapes_.push_back({ static_cast<ShapeKind>(static_cast<Uint32>(query.shape_)), ShapeStyle::Wireframe, ShapeSpace::Canvas, ShapeScope::Editor, origin, rotation, query.dimensions_, color });
				}
			}
		}
		queryInstance.Clear();
#endif
	}

	/**
	* [EN]
	* Returns the shapes gathered by the last Update.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直前の Update で集めた形を返す。
	*/
	std::span<const ShapeDesc> ShapeSystem::Shapes()const
	{
		return shapes_;
	}
}

#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <AudioEngine/Audio/AudioListener.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <FoundationEngine/World/ECS/Component/Lifetime.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Spawner.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Constraint/AttachmentConstraint.h>
#include <GraphicsEngine/Constraint/IKConstraint.h>
#include <GraphicsEngine/Constraint/LookAtConstraint.h>
#include <GraphicsEngine/Constraint/ParentConstraint.h>
#include <GraphicsEngine/Constraint/PositionConstraint.h>
#include <GraphicsEngine/Constraint/RotationConstraint.h>
#include <GraphicsEngine/Effect/Zephyr/Effect.h>
#include <GraphicsEngine/Environment/Celestial/Celestial.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStar.h>
#include <GraphicsEngine/Environment/Weather.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/AmbientOcclusion.h>
#include <GraphicsEngine/Illumination/Caustics/Caustics.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/GlobalIllumination.h>
#include <GraphicsEngine/Illumination/Reflection/Reflection.h>
#include <GraphicsEngine/Illumination/Refraction/Refraction.h>
#include <GraphicsEngine/Illumination/Shadow/Shadow.h>
#include <GraphicsEngine/Illumination/SubsurfaceScattering/SubsurfaceScattering.h>
#include <GraphicsEngine/Illumination/VolumetricLight/VolumetricLight.h>
#include <GraphicsEngine/Light/DirectionalLight.h>
#include <GraphicsEngine/Light/PointLight.h>
#include <GraphicsEngine/Light/RectangleLight.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/Light/SpotLight.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/Shape/Primitive/BoxShape.h>
#include <GraphicsEngine/Shape/Primitive/CapsuleShape.h>
#include <GraphicsEngine/Shape/Primitive/ConeShape.h>
#include <GraphicsEngine/Shape/Primitive/CylinderShape.h>
#include <GraphicsEngine/Shape/Primitive/DiscShape.h>
#include <GraphicsEngine/Shape/Primitive/PlaneShape.h>
#include <GraphicsEngine/Shape/Primitive/RampShape.h>
#include <GraphicsEngine/Shape/Primitive/SegmentShape.h>
#include <GraphicsEngine/Shape/Primitive/SphereShape.h>
#include <GraphicsEngine/Shape/Primitive/TorusShape.h>
#include <GraphicsEngine/Texture/Image.h>
#include <PhysicsEngine/CharacterController/CharacterController.h>
#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Collider/CapsuleCollider.h>
#include <PhysicsEngine/Collider/CircleCollider.h>
#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Collider/MeshCollider.h>
#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Joint/FixedJoint.h>
#include <PhysicsEngine/Joint/HingeJoint.h>
#include <PhysicsEngine/Joint/SliderJoint.h>
#include <PhysicsEngine/Joint/SpringJoint.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <PhysicsEngine/Softbody/Softbody.h>

extern "C" int _force_reflection_AudioListener = 0;
extern "C" int _force_reflection_AudioSource = 0;
extern "C" int _force_reflection_Lifetime = 0;
extern "C" int _force_reflection_Name = 0;
extern "C" int _force_reflection_Position = 0;
extern "C" int _force_reflection_Rotation = 0;
extern "C" int _force_reflection_Scale = 0;
extern "C" int _force_reflection_Spawner = 0;
extern "C" int _force_reflection_LinearVelocity = 0;
extern "C" int _force_reflection_AngularVelocity = 0;
extern "C" int _force_reflection_Camera = 0;
extern "C" int _force_reflection_CameraBrain = 0;
extern "C" int _force_reflection_AttachmentConstraint = 0;
extern "C" int _force_reflection_Effector = 0;
extern "C" int _force_reflection_IKConstraint = 0;
extern "C" int _force_reflection_LookAtConstraint = 0;
extern "C" int _force_reflection_ParentConstraint = 0;
extern "C" int _force_reflection_PositionConstraint = 0;
extern "C" int _force_reflection_RotationConstraint = 0;
extern "C" int _force_reflection_Effect = 0;
extern "C" int _force_reflection_Rain = 0;
extern "C" int _force_reflection_Snow = 0;
extern "C" int _force_reflection_Weather = 0;
extern "C" int _force_reflection_SunSettings = 0;
extern "C" int _force_reflection_MoonSettings = 0;
extern "C" int _force_reflection_Celestial = 0;
extern "C" int _force_reflection_VolumetricCloudScapes = 0;
extern "C" int _force_reflection_VolumetricStar = 0;
extern "C" int _force_reflection_Text = 0;
extern "C" int _force_reflection_RaytracingAmbientOcclusionSettings = 0;
extern "C" int _force_reflection_GroundTruthAmbientOcclusionSettings = 0;
extern "C" int _force_reflection_ScreenSpaceAmbientOcclusionSettings = 0;
extern "C" int _force_reflection_AmbientOcclusion = 0;
extern "C" int _force_reflection_Caustics = 0;
extern "C" int _force_reflection_RaytracingGlobalIlluminationSettings = 0;
extern "C" int _force_reflection_DynamicDiffuseGlobalIlluminationSettings = 0;
extern "C" int _force_reflection_LightPropagationVolumesSettings = 0;
extern "C" int _force_reflection_ScreenSpaceGlobalIlluminationSettings = 0;
extern "C" int _force_reflection_GlobalIllumination = 0;
extern "C" int _force_reflection_RaytracingReflectionSettings = 0;
extern "C" int _force_reflection_SignedDistanceFieldReflectionSettings = 0;
extern "C" int _force_reflection_ScreenSpaceReflectionSettings = 0;
extern "C" int _force_reflection_Reflection = 0;
extern "C" int _force_reflection_RaytracingRefractionSettings = 0;
extern "C" int _force_reflection_Refraction = 0;
extern "C" int _force_reflection_RaytracingShadowSettings = 0;
extern "C" int _force_reflection_VirtualShadowMapSettings = 0;
extern "C" int _force_reflection_CascadedShadowMapSettings = 0;
extern "C" int _force_reflection_Shadow = 0;
extern "C" int _force_reflection_RaytracingSubsurfaceScatteringSettings = 0;
extern "C" int _force_reflection_SubsurfaceScattering = 0;
extern "C" int _force_reflection_RaytracingVolumetricLightSettings = 0;
extern "C" int _force_reflection_VolumetricLight = 0;
extern "C" int _force_reflection_DirectionalLight = 0;
extern "C" int _force_reflection_PointLight = 0;
extern "C" int _force_reflection_RectangleLight = 0;
extern "C" int _force_reflection_SkyLight = 0;
extern "C" int _force_reflection_SpotLight = 0;
extern "C" int _force_reflection_AnimationParameter = 0;
extern "C" int _force_reflection_AnimationCondition = 0;
extern "C" int _force_reflection_AnimationState = 0;
extern "C" int _force_reflection_AnimationTransition = 0;
extern "C" int _force_reflection_Animator = 0;
extern "C" int _force_reflection_Material = 0;
extern "C" int _force_reflection_Movie = 0;
extern "C" int _force_reflection_DepthOfFieldSettings = 0;
extern "C" int _force_reflection_BokehSettings = 0;
extern "C" int _force_reflection_LensDistortionSettings = 0;
extern "C" int _force_reflection_ChromaticAberrationSettings = 0;
extern "C" int _force_reflection_VignetteSettings = 0;
extern "C" int _force_reflection_BloomSettings = 0;
extern "C" int _force_reflection_AnamorphicFlareSettings = 0;
extern "C" int _force_reflection_LensFlareSettings = 0;
extern "C" int _force_reflection_ExposureSettings = 0;
extern "C" int _force_reflection_ColorGradingRangeSettings = 0;
extern "C" int _force_reflection_ColorGradingSettings = 0;
extern "C" int _force_reflection_ToneMappingSettings = 0;
extern "C" int _force_reflection_SharpnessSettings = 0;
extern "C" int _force_reflection_FilmGrainSettings = 0;
extern "C" int _force_reflection_PostProcess = 0;
extern "C" int _force_reflection_BoxShape = 0;
extern "C" int _force_reflection_CapsuleShape = 0;
extern "C" int _force_reflection_ConeShape = 0;
extern "C" int _force_reflection_CylinderShape = 0;
extern "C" int _force_reflection_DiscShape = 0;
extern "C" int _force_reflection_PlaneShape = 0;
extern "C" int _force_reflection_RampShape = 0;
extern "C" int _force_reflection_SegmentShape = 0;
extern "C" int _force_reflection_SphereShape = 0;
extern "C" int _force_reflection_TorusShape = 0;
extern "C" int _force_reflection_Image = 0;
extern "C" int _force_reflection_CharacterController = 0;
extern "C" int _force_reflection_BoxCollider = 0;
extern "C" int _force_reflection_CapsuleCollider = 0;
extern "C" int _force_reflection_CircleCollider = 0;
extern "C" int _force_reflection_CylinderCollider = 0;
extern "C" int _force_reflection_MeshCollider = 0;
extern "C" int _force_reflection_RectCollider = 0;
extern "C" int _force_reflection_SphereCollider = 0;
extern "C" int _force_reflection_FixedJoint = 0;
extern "C" int _force_reflection_HingeJoint = 0;
extern "C" int _force_reflection_SliderJoint = 0;
extern "C" int _force_reflection_SpringJoint = 0;
extern "C" int _force_reflection_Rigidbody = 0;
extern "C" int _force_reflection_Softbody = 0;

namespace SeedCore
{
	 namespace ScReflection
	 {
		// ---- AudioEngine/Audio/AudioListener.h ----
		struct Register_AudioListener
		{
			Register_AudioListener()
			{
				ReflectionRegistry::Register(String("AudioListener"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AudioListener& obj = *static_cast<AudioListener*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("ドップラー倍率");
						fi.offset_ = offsetof(AudioListener, dopplerMultiplier_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AudioListener global_AudioListener_register;

		// ---- AudioEngine/Audio/AudioSource.h ----
		struct Register_AudioSource
		{
			Register_AudioSource()
			{
				ReflectionRegistry::Register(String("AudioSource"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AudioSource& obj = *static_cast<AudioSource*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("cueName_");
						fi.offset_ = offsetof(AudioSource, cueName_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("自動再生"), offsetof(AudioSource, autoPlay_), AttributeType::Bool });
					outInfo.push_back({ String("ループ再生"), offsetof(AudioSource, loop_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("音量");
						fi.offset_ = offsetof(AudioSource, volume_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ピッチ(半音)");
						fi.offset_ = offsetof(AudioSource, pitch_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = -12.0f;
						fi.clampMax_ = 12.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("立体音響"), offsetof(AudioSource, spatial_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("減衰開始距離");
						fi.offset_ = offsetof(AudioSource, minDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AudioSource*>(p); return o.spatial_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減衰終了距離");
						fi.offset_ = offsetof(AudioSource, maxDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AudioSource*>(p); return o.spatial_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AudioSource global_AudioSource_register;

		// ---- FoundationEngine/World/ECS/Component/Lifetime.h ----
		struct Register_Lifetime
		{
			Register_Lifetime()
			{
				ReflectionRegistry::Register(String("Lifetime"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Lifetime& obj = *static_cast<Lifetime*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("生存時間(秒)");
						fi.offset_ = offsetof(Lifetime, duration_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.05f;
						fi.clampMax_ = 3600.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Lifetime global_Lifetime_register;

		// ---- FoundationEngine/World/ECS/Component/Name.h ----
		struct Register_Name
		{
			Register_Name()
			{
				ReflectionRegistry::Register(String("Name"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Name& obj = *static_cast<Name*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("name_");
						fi.offset_ = offsetof(Name, name_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Name global_Name_register;

		// ---- FoundationEngine/World/ECS/Component/Position.h ----
		struct Register_Position
		{
			Register_Position()
			{
				ReflectionRegistry::Register(String("Position"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Position& obj = *static_cast<Position*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("x_");
						fi.offset_ = offsetof(Position, x_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("y_");
						fi.offset_ = offsetof(Position, y_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("z_");
						fi.offset_ = offsetof(Position, z_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Position global_Position_register;

		// ---- FoundationEngine/World/ECS/Component/Rotation.h ----
		struct Register_Rotation
		{
			Register_Rotation()
			{
				ReflectionRegistry::Register(String("Rotation"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Rotation& obj = *static_cast<Rotation*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("x_");
						fi.offset_ = offsetof(Rotation, x_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("y_");
						fi.offset_ = offsetof(Rotation, y_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("z_");
						fi.offset_ = offsetof(Rotation, z_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("w_");
						fi.offset_ = offsetof(Rotation, w_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Rotation global_Rotation_register;

		// ---- FoundationEngine/World/ECS/Component/Scale.h ----
		struct Register_Scale
		{
			Register_Scale()
			{
				ReflectionRegistry::Register(String("Scale"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Scale& obj = *static_cast<Scale*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("x_");
						fi.offset_ = offsetof(Scale, x_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("y_");
						fi.offset_ = offsetof(Scale, y_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("z_");
						fi.offset_ = offsetof(Scale, z_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Scale global_Scale_register;

		// ---- FoundationEngine/World/ECS/Component/Spawner.h ----
		struct Register_Spawner
		{
			Register_Spawner()
			{
				ReflectionRegistry::Register(String("Spawner"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Spawner& obj = *static_cast<Spawner*>(ptr);
					outInfo.push_back({ String("開始時に自動生成"), offsetof(Spawner, autoStart_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("生成間隔");
						fi.offset_ = offsetof(Spawner, spawnInterval_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 3600.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大生成数");
						fi.offset_ = offsetof(Spawner, maxCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 1000;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("生存時間");
						fi.offset_ = offsetof(Spawner, lifeTime_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 3600.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("ランダムスポーン"), offsetof(Spawner, randomSpawn_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("ランダム範囲");
						fi.offset_ = offsetof(Spawner, randomRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Spawner*>(p); return o.randomSpawn_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Spawner global_Spawner_register;

		// ---- FoundationEngine/World/ECS/Component/Velocity.h ----
		struct Register_LinearVelocity
		{
			Register_LinearVelocity()
			{
				ReflectionRegistry::Register(String("LinearVelocity"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LinearVelocity& obj = *static_cast<LinearVelocity*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("x_");
						fi.offset_ = offsetof(LinearVelocity, x_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("y_");
						fi.offset_ = offsetof(LinearVelocity, y_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("z_");
						fi.offset_ = offsetof(LinearVelocity, z_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_LinearVelocity global_LinearVelocity_register;

		struct Register_AngularVelocity
		{
			Register_AngularVelocity()
			{
				ReflectionRegistry::Register(String("AngularVelocity"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AngularVelocity& obj = *static_cast<AngularVelocity*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("x_");
						fi.offset_ = offsetof(AngularVelocity, x_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("y_");
						fi.offset_ = offsetof(AngularVelocity, y_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("z_");
						fi.offset_ = offsetof(AngularVelocity, z_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AngularVelocity global_AngularVelocity_register;

		// ---- GraphicsEngine/Camera/Camera.h ----
		struct Register_Camera
		{
			Register_Camera()
			{
				ReflectionRegistry::Register(String("Camera"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Camera& obj = *static_cast<Camera*>(ptr);
					outInfo.push_back({ String("FOV"), offsetof(Camera, fieldOfView_), AttributeType::Float });
					outInfo.push_back({ String("近クリップ面"), offsetof(Camera, nearPlane_), AttributeType::Float });
					outInfo.push_back({ String("遠クリップ面"), offsetof(Camera, farPlane_), AttributeType::Float });
					outInfo.push_back({ String("アクティブカメラ"), offsetof(Camera, isActive_), AttributeType::Bool });
				});
			}
		};
		static Register_Camera global_Camera_register;

		// ---- GraphicsEngine/Camera/CameraBrain.h ----
		struct Register_CameraBrain
		{
			Register_CameraBrain()
			{
				ReflectionRegistry::Register(String("CameraBrain"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CameraBrain& obj = *static_cast<CameraBrain*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("モード");
						fi.offset_ = offsetof(CameraBrain, mode_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("CameraBrainMode");
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("カメラ向き"), offsetof(CameraBrain, direction_), AttributeType::Vector3 });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(CameraBrain, weight_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 500;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ブレンド時間");
						fi.offset_ = offsetof(CameraBrain, blendTime_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("距離");
						fi.offset_ = offsetof(CameraBrain, distance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CameraBrain*>(p); return o.mode_ == CameraBrainMode::Orbit || o.mode_ == CameraBrainMode::Follow || o.mode_ == CameraBrainMode::Lockon; };
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 1000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("遅れ");
						fi.offset_ = offsetof(CameraBrain, damping_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CameraBrain*>(p); return o.mode_ == CameraBrainMode::Follow || o.mode_ == CameraBrainMode::Lockon; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CameraBrain global_CameraBrain_register;

		// ---- GraphicsEngine/Constraint/AttachmentConstraint.h ----
		struct Register_AttachmentConstraint
		{
			Register_AttachmentConstraint()
			{
				ReflectionRegistry::Register(String("AttachmentConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AttachmentConstraint& obj = *static_cast<AttachmentConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(AttachmentConstraint, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(AttachmentConstraint, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("位置オフセット"), offsetof(AttachmentConstraint, positionOffset_), AttributeType::Vector3 });
					outInfo.push_back({ String("回転オフセット(オイラー角)"), offsetof(AttachmentConstraint, rotationOffset_), AttributeType::Vector3 });
					{
						FieldInfo fi;
						fi.name_ = String("boneName_");
						fi.offset_ = offsetof(AttachmentConstraint, boneName_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AttachmentConstraint global_AttachmentConstraint_register;

		// ---- GraphicsEngine/Constraint/IKConstraint.h ----
		struct Register_Effector
		{
			Register_Effector()
			{
				ReflectionRegistry::Register(String("Effector"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Effector& obj = *static_cast<Effector*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(Effector, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("effectorBoneName_");
						fi.offset_ = offsetof(Effector, effectorBoneName_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Effector global_Effector_register;

		struct Register_IKConstraint
		{
			Register_IKConstraint()
			{
				ReflectionRegistry::Register(String("IKConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					IKConstraint& obj = *static_cast<IKConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(IKConstraint, enabled_), AttributeType::Bool });
					{
						auto& arr = obj.entries_;
						FieldInfo header;
						header.name_ = String("エフェクタ");
						header.offset_ = 0;
						header.type_ = AttributeType::Struct;
						header.nestedTypeName_ = String("Effector");
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.entries_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.entries_.size()) obj.entries_.erase(obj.entries_.begin() + idx); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo;
							elementInfo.offset_ = 0;
							elementInfo.type_ = AttributeType::Struct;
							elementInfo.directPtr_ = &arr[i];
							elementInfo.nestedTypeName_ = String("Effector");
							outInfo.push_back(std::move(elementInfo));
						}
					}
				});
			}
		};
		static Register_IKConstraint global_IKConstraint_register;

		// ---- GraphicsEngine/Constraint/LookAtConstraint.h ----
		struct Register_LookAtConstraint
		{
			Register_LookAtConstraint()
			{
				ReflectionRegistry::Register(String("LookAtConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LookAtConstraint& obj = *static_cast<LookAtConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(LookAtConstraint, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(LookAtConstraint, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("上方向"), offsetof(LookAtConstraint, upVector_), AttributeType::Vector3 });
				});
			}
		};
		static Register_LookAtConstraint global_LookAtConstraint_register;

		// ---- GraphicsEngine/Constraint/ParentConstraint.h ----
		struct Register_ParentConstraint
		{
			Register_ParentConstraint()
			{
				ReflectionRegistry::Register(String("ParentConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ParentConstraint& obj = *static_cast<ParentConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(ParentConstraint, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(ParentConstraint, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("位置オフセット"), offsetof(ParentConstraint, positionOffset_), AttributeType::Vector3 });
					outInfo.push_back({ String("回転オフセット(オイラー角)"), offsetof(ParentConstraint, rotationOffset_), AttributeType::Vector3 });
				});
			}
		};
		static Register_ParentConstraint global_ParentConstraint_register;

		// ---- GraphicsEngine/Constraint/PositionConstraint.h ----
		struct Register_PositionConstraint
		{
			Register_PositionConstraint()
			{
				ReflectionRegistry::Register(String("PositionConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PositionConstraint& obj = *static_cast<PositionConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(PositionConstraint, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(PositionConstraint, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("オフセット"), offsetof(PositionConstraint, offset_), AttributeType::Vector3 });
				});
			}
		};
		static Register_PositionConstraint global_PositionConstraint_register;

		// ---- GraphicsEngine/Constraint/RotationConstraint.h ----
		struct Register_RotationConstraint
		{
			Register_RotationConstraint()
			{
				ReflectionRegistry::Register(String("RotationConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RotationConstraint& obj = *static_cast<RotationConstraint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(RotationConstraint, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("重み");
						fi.offset_ = offsetof(RotationConstraint, weight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("オフセット(オイラー角)"), offsetof(RotationConstraint, offset_), AttributeType::Vector3 });
				});
			}
		};
		static Register_RotationConstraint global_RotationConstraint_register;

		// ---- GraphicsEngine/Effect/Zephyr/Effect.h ----
		struct Register_Effect
		{
			Register_Effect()
			{
				ReflectionRegistry::Register(String("Effect"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Effect& obj = *static_cast<Effect*>(ptr);
					outInfo.push_back({ String("スポーンレート"), offsetof(Effect, spawnRate_), AttributeType::Float });
				});
			}
		};
		static Register_Effect global_Effect_register;

		// ---- GraphicsEngine/Environment/Weather.h ----
		struct Register_Rain
		{
			Register_Rain()
			{
				ReflectionRegistry::Register(String("Rain"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Rain& obj = *static_cast<Rain*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("密度");
						fi.offset_ = offsetof(Rain, density_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("落下速度");
						fi.offset_ = offsetof(Rain, fallSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 30.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(Rain, size_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("筋の長さ");
						fi.offset_ = offsetof(Rain, streakLength_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("明るさ");
						fi.offset_ = offsetof(Rain, brightness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 3.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スポーン範囲(半径)");
						fi.offset_ = offsetof(Rain, volumeRadius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 2.0f;
						fi.clampMax_ = 60.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スポーン範囲(高さ)");
						fi.offset_ = offsetof(Rain, volumeHeight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 2.0f;
						fi.clampMax_ = 40.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(Rain, color_), AttributeType::Color });
				});
			}
		};
		static Register_Rain global_Rain_register;

		struct Register_Snow
		{
			Register_Snow()
			{
				ReflectionRegistry::Register(String("Snow"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Snow& obj = *static_cast<Snow*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("密度");
						fi.offset_ = offsetof(Snow, density_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("落下速度");
						fi.offset_ = offsetof(Snow, fallSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(Snow, size_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.3f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("揺れ幅");
						fi.offset_ = offsetof(Snow, swayAmount_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("明るさ");
						fi.offset_ = offsetof(Snow, brightness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 3.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スポーン範囲(半径)");
						fi.offset_ = offsetof(Snow, volumeRadius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 2.0f;
						fi.clampMax_ = 60.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スポーン範囲(高さ)");
						fi.offset_ = offsetof(Snow, volumeHeight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 2.0f;
						fi.clampMax_ = 40.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(Snow, color_), AttributeType::Color });
				});
			}
		};
		static Register_Snow global_Snow_register;

		struct Register_Weather
		{
			Register_Weather()
			{
				ReflectionRegistry::Register(String("Weather"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Weather& obj = *static_cast<Weather*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("天候");
						fi.offset_ = offsetof(Weather, type_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("WeatherType");
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("自動サイクル"), offsetof(Weather, autoCycle_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("最短間隔(分)");
						fi.offset_ = offsetof(Weather, minIntervalMinutes_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 180.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最長間隔(分)");
						fi.offset_ = offsetof(Weather, maxIntervalMinutes_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 180.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("遷移時間(秒)");
						fi.offset_ = offsetof(Weather, transitionSeconds_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 300.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("雨を季節無視で許可"), offsetof(Weather, forceRain_), AttributeType::Bool });
					outInfo.push_back({ String("雪を季節無視で許可"), offsetof(Weather, forceSnow_), AttributeType::Bool });
					outInfo.push_back({ String("雷を強制的に許可"), offsetof(Weather, forceThunder_), AttributeType::Bool });
					outInfo.push_back({ String("暴風を強制的に許可"), offsetof(Weather, forceStorm_), AttributeType::Bool });
					outInfo.push_back({ String("雨パーティクル"), offsetof(Weather, rainEnabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("雨");
						fi.offset_ = offsetof(Weather, rain_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("Rain");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Weather*>(p); return o.rainEnabled_; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("雪パーティクル"), offsetof(Weather, snowEnabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("雪");
						fi.offset_ = offsetof(Weather, snow_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("Snow");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Weather*>(p); return o.snowEnabled_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Weather global_Weather_register;

		// ---- GraphicsEngine/Environment/Celestial/Celestial.h ----
		struct Register_SunSettings
		{
			Register_SunSettings()
			{
				ReflectionRegistry::Register(String("SunSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SunSettings& obj = *static_cast<SunSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(SunSettings, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("地平線の色");
						fi.offset_ = offsetof(SunSettings, horizonColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SunSettings*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("天頂の色");
						fi.offset_ = offsetof(SunSettings, zenithColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SunSettings*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大の強さ");
						fi.offset_ = offsetof(SunSettings, maxIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SunSettings*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("光が消える仰角（度）");
						fi.offset_ = offsetof(SunSettings, minElevationDegrees_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SunSettings*>(p); return o.enable_; };
						fi.clampMin_ = -20.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("角半径");
						fi.offset_ = offsetof(SunSettings, angularRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SunSettings*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SunSettings global_SunSettings_register;

		struct Register_MoonSettings
		{
			Register_MoonSettings()
			{
				ReflectionRegistry::Register(String("MoonSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					MoonSettings& obj = *static_cast<MoonSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(MoonSettings, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("色");
						fi.offset_ = offsetof(MoonSettings, color_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<MoonSettings*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大の強さ");
						fi.offset_ = offsetof(MoonSettings, maxIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<MoonSettings*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("角半径");
						fi.offset_ = offsetof(MoonSettings, angularRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<MoonSettings*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_MoonSettings global_MoonSettings_register;

		struct Register_Celestial
		{
			Register_Celestial()
			{
				ReflectionRegistry::Register(String("Celestial"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Celestial& obj = *static_cast<Celestial*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(Celestial, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("時刻");
						fi.offset_ = offsetof(Celestial, hourOfDay_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 24.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("日");
						fi.offset_ = offsetof(Celestial, dayOfMonth_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 365;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("1 か月の日数");
						fi.offset_ = offsetof(Celestial, daysPerMonth_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 365;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("月");
						fi.offset_ = offsetof(Celestial, monthOfYear_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 12;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("1 日の長さ（分）");
						fi.offset_ = offsetof(Celestial, dayLengthMinutes_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 1440.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("時間の倍率");
						fi.offset_ = offsetof(Celestial, timeScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 100.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("一時停止");
						fi.offset_ = offsetof(Celestial, paused_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("太陽");
						fi.offset_ = offsetof(Celestial, sun_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("SunSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("月");
						fi.offset_ = offsetof(Celestial, moon_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("MoonSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Celestial*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Celestial global_Celestial_register;

		// ---- GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h ----
		struct Register_VolumetricCloudScapes
		{
			Register_VolumetricCloudScapes()
			{
				ReflectionRegistry::Register(String("VolumetricCloudScapes"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					VolumetricCloudScapes& obj = *static_cast<VolumetricCloudScapes*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(VolumetricCloudScapes, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("雲層の下端");
						fi.offset_ = offsetof(VolumetricCloudScapes, cloudBottom_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("雲層の上端");
						fi.offset_ = offsetof(VolumetricCloudScapes, cloudTop_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 100.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("惑星の半径");
						fi.offset_ = offsetof(VolumetricCloudScapes, planetRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 100000.0f;
						fi.clampMax_ = 40000000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("雲量");
						fi.offset_ = offsetof(VolumetricCloudScapes, coverage_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("密度");
						fi.offset_ = offsetof(VolumetricCloudScapes, densityScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("雲の形（層雲〜積雲）");
						fi.offset_ = offsetof(VolumetricCloudScapes, cloudType_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("雨雲の度合い");
						fi.offset_ = offsetof(VolumetricCloudScapes, rain_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("形状ノイズのスケール");
						fi.offset_ = offsetof(VolumetricCloudScapes, noiseScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 0.01f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ディテールのスケール");
						fi.offset_ = offsetof(VolumetricCloudScapes, detailScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 16.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ディテールの強さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, detailStrength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("天候フィールドの周波数");
						fi.offset_ = offsetof(VolumetricCloudScapes, weatherScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("天候フィールドの影響");
						fi.offset_ = offsetof(VolumetricCloudScapes, weatherAmount_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("風の速さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, windSpeed_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ディテールの風の速さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, detailWindSpeed_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("雲の散乱色");
						fi.offset_ = offsetof(VolumetricCloudScapes, cloudAlbedo_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("前方散乱の異方性");
						fi.offset_ = offsetof(VolumetricCloudScapes, scatteringG_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = -0.9f;
						fi.clampMax_ = 0.9f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("後方散乱の異方性");
						fi.offset_ = offsetof(VolumetricCloudScapes, phaseBackward_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.9f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("前方・後方のブレンド");
						fi.offset_ = offsetof(VolumetricCloudScapes, phaseBlend_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("パウダー効果の強さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, powderStrength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("環境光の強さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, ambientStrength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("IBL の強さ");
						fi.offset_ = offsetof(VolumetricCloudScapes, iblIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("多重散乱のオクターブ数");
						fi.offset_ = offsetof(VolumetricCloudScapes, multiScatterOctaves_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 8;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("多重散乱の消散の減衰");
						fi.offset_ = offsetof(VolumetricCloudScapes, multiScatterAttenuation_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 0.95f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("多重散乱の寄与の減衰");
						fi.offset_ = offsetof(VolumetricCloudScapes, multiScatterContribution_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 0.95f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("多重散乱の偏心の減衰");
						fi.offset_ = offsetof(VolumetricCloudScapes, multiScatterEccentricity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 0.95f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("視線のステップ数");
						fi.offset_ = offsetof(VolumetricCloudScapes, stepCount_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 8;
						fi.clampMax_ = 256;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ライトマーチのステップ数");
						fi.offset_ = offsetof(VolumetricCloudScapes, lightStepCount_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 16;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("マーチの最大距離");
						fi.offset_ = offsetof(VolumetricCloudScapes, maxMarchDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 20000.0f;
						fi.clampMax_ = 400000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("LOD の開始距離");
						fi.offset_ = offsetof(VolumetricCloudScapes, lodDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 2000.0f;
						fi.clampMax_ = 100000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("時間方向のジッター");
						fi.offset_ = offsetof(VolumetricCloudScapes, temporalJitter_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("天頂の色");
						fi.offset_ = offsetof(VolumetricCloudScapes, skyZenithColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("地平線の色");
						fi.offset_ = offsetof(VolumetricCloudScapes, skyHorizonColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("空の明るさ");
						fi.offset_ = offsetof(VolumetricCloudScapes, skyBrightness_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("太陽の視半径");
						fi.offset_ = offsetof(VolumetricCloudScapes, sunSize_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("地面の色");
						fi.offset_ = offsetof(VolumetricCloudScapes, groundColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("空気遠近の密度");
						fi.offset_ = offsetof(VolumetricCloudScapes, aerialDensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricCloudScapes*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.0002f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_VolumetricCloudScapes global_VolumetricCloudScapes_register;

		// ---- GraphicsEngine/Environment/VolumetricStar/VolumetricStar.h ----
		struct Register_VolumetricStar
		{
			Register_VolumetricStar()
			{
				ReflectionRegistry::Register(String("VolumetricStar"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					VolumetricStar& obj = *static_cast<VolumetricStar*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(VolumetricStar, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("セルのサイズ");
						fi.offset_ = offsetof(VolumetricStar, cellSize_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.02f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("星の密度");
						fi.offset_ = offsetof(VolumetricStar, density_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("明るさ");
						fi.offset_ = offsetof(VolumetricStar, brightness_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("IBL の強さ");
						fi.offset_ = offsetof(VolumetricStar, iblIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("またたく速さ");
						fi.offset_ = offsetof(VolumetricStar, twinkleSpeed_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("星の色");
						fi.offset_ = offsetof(VolumetricStar, color_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最小サイズ");
						fi.offset_ = offsetof(VolumetricStar, sizeMin_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大サイズ");
						fi.offset_ = offsetof(VolumetricStar, sizeMax_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.005f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("グローの強さ");
						fi.offset_ = offsetof(VolumetricStar, glowIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 3.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("グローの減衰");
						fi.offset_ = offsetof(VolumetricStar, glowFalloff_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 10.0f;
						fi.clampMax_ = 400.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("流れ星の出現確率（毎秒）");
						fi.offset_ = offsetof(VolumetricStar, shootingStarChancePerSecond_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("流れ星の明るさ");
						fi.offset_ = offsetof(VolumetricStar, shootingStarBrightness_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("流れ星の幅");
						fi.offset_ = offsetof(VolumetricStar, shootingStarWidth_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricStar*>(p); return o.enable_; };
						fi.clampMin_ = 0.0002f;
						fi.clampMax_ = 0.02f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_VolumetricStar global_VolumetricStar_register;

		// ---- GraphicsEngine/Font/Text.h ----
		struct Register_Text
		{
			Register_Text()
			{
				ReflectionRegistry::Register(String("Text"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Text& obj = *static_cast<Text*>(ptr);
					outInfo.push_back({ String("テキスト"), offsetof(Text, text_), AttributeType::String });
					outInfo.push_back({ String("文字サイズ"), offsetof(Text, fontSize_), AttributeType::Float });
					outInfo.push_back({ String("文字間隔"), offsetof(Text, letterSpacing_), AttributeType::Float });
					outInfo.push_back({ String("行間"), offsetof(Text, lineSpacing_), AttributeType::Float });
					outInfo.push_back({ String("中心点"), offsetof(Text, pivot_), AttributeType::Vector2 });
					outInfo.push_back({ String("色"), offsetof(Text, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("アウトライン幅");
						fi.offset_ = offsetof(Text, outlineWidth_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 28.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("アウトライン色"), offsetof(Text, outlineColor_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("発光量");
						fi.offset_ = offsetof(Text, glowPower_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("発光色"), offsetof(Text, glowColor_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("表示形式");
						fi.offset_ = offsetof(Text, viewType_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("Text::ViewType");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("描画順");
						fi.offset_ = offsetof(Text, order_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Text*>(p); return o.viewType_ == Text::ViewType::Sprite; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カメラ正対");
						fi.offset_ = offsetof(Text, faceCamera_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Text*>(p); return o.viewType_ == Text::ViewType::Billboard; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("影"), offsetof(Text, shadowEnable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("影オフセット");
						fi.offset_ = offsetof(Text, shadowOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Text*>(p); return o.shadowEnable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("影色");
						fi.offset_ = offsetof(Text, shadowColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Text*>(p); return o.shadowEnable_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Text global_Text_register;

		// ---- GraphicsEngine/Illumination/AmbientOcclusion/AmbientOcclusion.h ----
		struct Register_RaytracingAmbientOcclusionSettings
		{
			Register_RaytracingAmbientOcclusionSettings()
			{
				ReflectionRegistry::Register(String("RaytracingAmbientOcclusionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingAmbientOcclusionSettings& obj = *static_cast<RaytracingAmbientOcclusionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの本数");
						fi.offset_ = offsetof(RaytracingAmbientOcclusionSettings, rayCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 32;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(RaytracingAmbientOcclusionSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingAmbientOcclusionSettings global_RaytracingAmbientOcclusionSettings_register;

		struct Register_GroundTruthAmbientOcclusionSettings
		{
			Register_GroundTruthAmbientOcclusionSettings()
			{
				ReflectionRegistry::Register(String("GroundTruthAmbientOcclusionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					GroundTruthAmbientOcclusionSettings& obj = *static_cast<GroundTruthAmbientOcclusionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("減衰の範囲");
						fi.offset_ = offsetof(GroundTruthAmbientOcclusionSettings, falloffRange_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スライス数");
						fi.offset_ = offsetof(GroundTruthAmbientOcclusionSettings, sliceCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 8;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スライスあたりのステップ数");
						fi.offset_ = offsetof(GroundTruthAmbientOcclusionSettings, stepsPerSlice_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 8;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_GroundTruthAmbientOcclusionSettings global_GroundTruthAmbientOcclusionSettings_register;

		struct Register_ScreenSpaceAmbientOcclusionSettings
		{
			Register_ScreenSpaceAmbientOcclusionSettings()
			{
				ReflectionRegistry::Register(String("ScreenSpaceAmbientOcclusionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ScreenSpaceAmbientOcclusionSettings& obj = *static_cast<ScreenSpaceAmbientOcclusionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("バイアス");
						fi.offset_ = offsetof(ScreenSpaceAmbientOcclusionSettings, bias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("強さ");
						fi.offset_ = offsetof(ScreenSpaceAmbientOcclusionSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サンプル数");
						fi.offset_ = offsetof(ScreenSpaceAmbientOcclusionSettings, sampleCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 4;
						fi.clampMax_ = 128;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ScreenSpaceAmbientOcclusionSettings global_ScreenSpaceAmbientOcclusionSettings_register;

		struct Register_AmbientOcclusion
		{
			Register_AmbientOcclusion()
			{
				ReflectionRegistry::Register(String("AmbientOcclusion"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AmbientOcclusion& obj = *static_cast<AmbientOcclusion*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(AmbientOcclusion, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(AmbientOcclusion, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("AmbientOcclusionTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("遮蔽の半径");
						fi.offset_ = offsetof(AmbientOcclusion, radius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_; };
						fi.clampMin_ = 0.05f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("コントラスト");
						fi.offset_ = offsetof(AmbientOcclusion, power_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_; };
						fi.clampMin_ = 0.25f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシングAO");
						fi.offset_ = offsetof(AmbientOcclusion, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingAmbientOcclusionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_&& o.technique_ == AmbientOcclusionTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("グラウンドトゥルースAO（SSAO）");
						fi.offset_ = offsetof(AmbientOcclusion, groundTruth_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("GroundTruthAmbientOcclusionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_&& o.technique_ == AmbientOcclusionTechnique::GroundTruth; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペースAO（SSAO）");
						fi.offset_ = offsetof(AmbientOcclusion, screenSpace_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ScreenSpaceAmbientOcclusionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AmbientOcclusion*>(p); return o.enable_&& o.technique_ == AmbientOcclusionTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AmbientOcclusion global_AmbientOcclusion_register;

		// ---- GraphicsEngine/Illumination/Caustics/Caustics.h ----
		struct Register_Caustics
		{
			Register_Caustics()
			{
				ReflectionRegistry::Register(String("Caustics"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Caustics& obj = *static_cast<Caustics*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(Caustics, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(Caustics, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("CausticsTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Caustics*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシングコースティクス");
						fi.offset_ = offsetof(Caustics, raytracing_);
						fi.type_ = AttributeType::Unknown;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Caustics*>(p); return o.enable_&& o.technique_ == CausticsTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペースコースティクス");
						fi.offset_ = offsetof(Caustics, screenSpace_);
						fi.type_ = AttributeType::Unknown;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Caustics*>(p); return o.enable_&& o.technique_ == CausticsTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Caustics global_Caustics_register;

		// ---- GraphicsEngine/Illumination/GlobalIllumination/GlobalIllumination.h ----
		struct Register_RaytracingGlobalIlluminationSettings
		{
			Register_RaytracingGlobalIlluminationSettings()
			{
				ReflectionRegistry::Register(String("RaytracingGlobalIlluminationSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingGlobalIlluminationSettings& obj = *static_cast<RaytracingGlobalIlluminationSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの本数");
						fi.offset_ = offsetof(RaytracingGlobalIlluminationSettings, rayCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 16;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("バウンス数");
						fi.offset_ = offsetof(RaytracingGlobalIlluminationSettings, bounceCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 4;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイの最大長");
						fi.offset_ = offsetof(RaytracingGlobalIlluminationSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 50.0f;
						fi.clampMax_ = 20000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(RaytracingGlobalIlluminationSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingGlobalIlluminationSettings global_RaytracingGlobalIlluminationSettings_register;

		struct Register_DynamicDiffuseGlobalIlluminationSettings
		{
			Register_DynamicDiffuseGlobalIlluminationSettings()
			{
				ReflectionRegistry::Register(String("DynamicDiffuseGlobalIlluminationSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DynamicDiffuseGlobalIlluminationSettings& obj = *static_cast<DynamicDiffuseGlobalIlluminationSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("プローブあたりのレイ数");
						fi.offset_ = offsetof(DynamicDiffuseGlobalIlluminationSettings, raysPerProbe_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 32;
						fi.clampMax_ = 512;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("プローブの間隔");
						fi.offset_ = offsetof(DynamicDiffuseGlobalIlluminationSettings, probeSpacing_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.25f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("履歴の重み");
						fi.offset_ = offsetof(DynamicDiffuseGlobalIlluminationSettings, hysteresis_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.5f;
						fi.clampMax_ = 0.999f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(DynamicDiffuseGlobalIlluminationSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_DynamicDiffuseGlobalIlluminationSettings global_DynamicDiffuseGlobalIlluminationSettings_register;

		struct Register_LightPropagationVolumesSettings
		{
			Register_LightPropagationVolumesSettings()
			{
				ReflectionRegistry::Register(String("LightPropagationVolumesSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LightPropagationVolumesSettings& obj = *static_cast<LightPropagationVolumesSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("格子の解像度");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, gridResolution_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 16;
						fi.clampMax_ = 64;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ボリュームの大きさ");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, volumeSize_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 8.0f;
						fi.clampMax_ = 512.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カスケードの数");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, cascadeCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 4;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("伝播の回数");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, propagationIterations_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 32;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("RSM（反射シャドウマップ）の解像度");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, reflectiveShadowMapResolution_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 128;
						fi.clampMax_ = 2048;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("注入のオフセット");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, injectionBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("遮蔽"), offsetof(LightPropagationVolumesSettings, occlusionEnabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("遮蔽の強さ");
						fi.offset_ = offsetof(LightPropagationVolumesSettings, occlusionStrength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LightPropagationVolumesSettings*>(p); return o.occlusionEnabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_LightPropagationVolumesSettings global_LightPropagationVolumesSettings_register;

		struct Register_ScreenSpaceGlobalIlluminationSettings
		{
			Register_ScreenSpaceGlobalIlluminationSettings()
			{
				ReflectionRegistry::Register(String("ScreenSpaceGlobalIlluminationSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ScreenSpaceGlobalIlluminationSettings& obj = *static_cast<ScreenSpaceGlobalIlluminationSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの長さ");
						fi.offset_ = offsetof(ScreenSpaceGlobalIlluminationSettings, rayLength_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("厚み");
						fi.offset_ = offsetof(ScreenSpaceGlobalIlluminationSettings, thickness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サンプル数");
						fi.offset_ = offsetof(ScreenSpaceGlobalIlluminationSettings, sampleCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 2;
						fi.clampMax_ = 64;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ScreenSpaceGlobalIlluminationSettings global_ScreenSpaceGlobalIlluminationSettings_register;

		struct Register_GlobalIllumination
		{
			Register_GlobalIllumination()
			{
				ReflectionRegistry::Register(String("GlobalIllumination"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					GlobalIllumination& obj = *static_cast<GlobalIllumination*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(GlobalIllumination, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(GlobalIllumination, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("GlobalIlluminationTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("間接光の強さ");
						fi.offset_ = offsetof(GlobalIllumination, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシングGI");
						fi.offset_ = offsetof(GlobalIllumination, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingGlobalIlluminationSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_&& o.technique_ == GlobalIlluminationTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("動的ディフューズGI（DDGI）");
						fi.offset_ = offsetof(GlobalIllumination, dynamicDiffuse_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("DynamicDiffuseGlobalIlluminationSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_&& o.technique_ == GlobalIlluminationTechnique::DynamicDiffuse; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ライト・プロパゲーション・ボリューム（LPV）");
						fi.offset_ = offsetof(GlobalIllumination, lightPropagationVolumes_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("LightPropagationVolumesSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_&& o.technique_ == GlobalIlluminationTechnique::LightPropagationVolumes; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペースGI（SSGI）");
						fi.offset_ = offsetof(GlobalIllumination, screenSpace_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ScreenSpaceGlobalIlluminationSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<GlobalIllumination*>(p); return o.enable_&& o.technique_ == GlobalIlluminationTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_GlobalIllumination global_GlobalIllumination_register;

		// ---- GraphicsEngine/Illumination/Reflection/Reflection.h ----
		struct Register_RaytracingReflectionSettings
		{
			Register_RaytracingReflectionSettings()
			{
				ReflectionRegistry::Register(String("RaytracingReflectionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingReflectionSettings& obj = *static_cast<RaytracingReflectionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの本数");
						fi.offset_ = offsetof(RaytracingReflectionSettings, rayCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 32;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("バウンス数");
						fi.offset_ = offsetof(RaytracingReflectionSettings, bounceCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 4;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("バウンス後の最大の粗さ");
						fi.offset_ = offsetof(RaytracingReflectionSettings, bounceRoughnessThreshold_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイの最大長");
						fi.offset_ = offsetof(RaytracingReflectionSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 5000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(RaytracingReflectionSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingReflectionSettings global_RaytracingReflectionSettings_register;

		struct Register_SignedDistanceFieldReflectionSettings
		{
			Register_SignedDistanceFieldReflectionSettings()
			{
				ReflectionRegistry::Register(String("SignedDistanceFieldReflectionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SignedDistanceFieldReflectionSettings& obj = *static_cast<SignedDistanceFieldReflectionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("最大距離");
						fi.offset_ = offsetof(SignedDistanceFieldReflectionSettings, maxDistance_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 5000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("コーンの角度");
						fi.offset_ = offsetof(SignedDistanceFieldReflectionSettings, coneAngle_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(SignedDistanceFieldReflectionSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SignedDistanceFieldReflectionSettings global_SignedDistanceFieldReflectionSettings_register;

		struct Register_ScreenSpaceReflectionSettings
		{
			Register_ScreenSpaceReflectionSettings()
			{
				ReflectionRegistry::Register(String("ScreenSpaceReflectionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ScreenSpaceReflectionSettings& obj = *static_cast<ScreenSpaceReflectionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("反射する最大の粗さ");
						fi.offset_ = offsetof(ScreenSpaceReflectionSettings, maxRoughness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("厚み");
						fi.offset_ = offsetof(ScreenSpaceReflectionSettings, thickness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大ステップ数");
						fi.offset_ = offsetof(ScreenSpaceReflectionSettings, maxStepCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 8;
						fi.clampMax_ = 256;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ScreenSpaceReflectionSettings global_ScreenSpaceReflectionSettings_register;

		struct Register_Reflection
		{
			Register_Reflection()
			{
				ReflectionRegistry::Register(String("Reflection"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Reflection& obj = *static_cast<Reflection*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(Reflection, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(Reflection, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("ReflectionTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Reflection*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("反射の強さ");
						fi.offset_ = offsetof(Reflection, strength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Reflection*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシング反射");
						fi.offset_ = offsetof(Reflection, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingReflectionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Reflection*>(p); return o.enable_&& o.technique_ == ReflectionTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("距離フィールド反射（SDFR）");
						fi.offset_ = offsetof(Reflection, signedDistanceField_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("SignedDistanceFieldReflectionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Reflection*>(p); return o.enable_&& o.technique_ == ReflectionTechnique::SignedDistanceField; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペース反射（SSR）");
						fi.offset_ = offsetof(Reflection, screenSpace_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ScreenSpaceReflectionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Reflection*>(p); return o.enable_&& o.technique_ == ReflectionTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Reflection global_Reflection_register;

		// ---- GraphicsEngine/Illumination/Refraction/Refraction.h ----
		struct Register_RaytracingRefractionSettings
		{
			Register_RaytracingRefractionSettings()
			{
				ReflectionRegistry::Register(String("RaytracingRefractionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingRefractionSettings& obj = *static_cast<RaytracingRefractionSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの最大長");
						fi.offset_ = offsetof(RaytracingRefractionSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 5000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(RaytracingRefractionSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingRefractionSettings global_RaytracingRefractionSettings_register;

		struct Register_Refraction
		{
			Register_Refraction()
			{
				ReflectionRegistry::Register(String("Refraction"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Refraction& obj = *static_cast<Refraction*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(Refraction, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(Refraction, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("RefractionTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Refraction*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("屈折の強さ");
						fi.offset_ = offsetof(Refraction, strength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Refraction*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシング屈折");
						fi.offset_ = offsetof(Refraction, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingRefractionSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Refraction*>(p); return o.enable_&& o.technique_ == RefractionTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペース屈折");
						fi.offset_ = offsetof(Refraction, screenSpace_);
						fi.type_ = AttributeType::Unknown;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Refraction*>(p); return o.enable_&& o.technique_ == RefractionTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Refraction global_Refraction_register;

		// ---- GraphicsEngine/Illumination/Shadow/Shadow.h ----
		struct Register_RaytracingShadowSettings
		{
			Register_RaytracingShadowSettings()
			{
				ReflectionRegistry::Register(String("RaytracingShadowSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingShadowSettings& obj = *static_cast<RaytracingShadowSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("レイの本数");
						fi.offset_ = offsetof(RaytracingShadowSettings, rayCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 32;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイの最大長");
						fi.offset_ = offsetof(RaytracingShadowSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 5000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("法線方向のオフセット");
						fi.offset_ = offsetof(RaytracingShadowSettings, normalBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("影の濃さ");
						fi.offset_ = offsetof(RaytracingShadowSettings, shadowStrength_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("太陽の角半径");
						fi.offset_ = offsetof(RaytracingShadowSettings, sunAngularRadius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("点光源の半径");
						fi.offset_ = offsetof(RaytracingShadowSettings, punctualLightRadius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingShadowSettings global_RaytracingShadowSettings_register;

		struct Register_VirtualShadowMapSettings
		{
			Register_VirtualShadowMapSettings()
			{
				ReflectionRegistry::Register(String("VirtualShadowMapSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					VirtualShadowMapSettings& obj = *static_cast<VirtualShadowMapSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("仮想解像度（2の累乗）");
						fi.offset_ = offsetof(VirtualShadowMapSettings, virtualResolutionLog2_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 11;
						fi.clampMax_ = 16;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ページプールのサイズ（MiB）");
						fi.offset_ = offsetof(VirtualShadowMapSettings, pagePoolSizeMib_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 16;
						fi.clampMax_ = 512;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("クリップマップの段数");
						fi.offset_ = offsetof(VirtualShadowMapSettings, clipmapLevelCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 12;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("柔らかさの半径");
						fi.offset_ = offsetof(VirtualShadowMapSettings, softnessRadius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("深度バイアス");
						fi.offset_ = offsetof(VirtualShadowMapSettings, depthBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 0.05f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_VirtualShadowMapSettings global_VirtualShadowMapSettings_register;

		struct Register_CascadedShadowMapSettings
		{
			Register_CascadedShadowMapSettings()
			{
				ReflectionRegistry::Register(String("CascadedShadowMapSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CascadedShadowMapSettings& obj = *static_cast<CascadedShadowMapSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("カスケードの数");
						fi.offset_ = offsetof(CascadedShadowMapSettings, cascadeCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 4;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("分割の比率");
						fi.offset_ = offsetof(CascadedShadowMapSettings, splitLambda_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("PCFのカーネルサイズ");
						fi.offset_ = offsetof(CascadedShadowMapSettings, pcfKernelSize_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 9;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カスケード境界のブレンド幅");
						fi.offset_ = offsetof(CascadedShadowMapSettings, cascadeBlendWidth_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("深度バイアス");
						fi.offset_ = offsetof(CascadedShadowMapSettings, depthBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 0.05f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("ちらつき防止"), offsetof(CascadedShadowMapSettings, stabilize_), AttributeType::Bool });
				});
			}
		};
		static Register_CascadedShadowMapSettings global_CascadedShadowMapSettings_register;

		struct Register_Shadow
		{
			Register_Shadow()
			{
				ReflectionRegistry::Register(String("Shadow"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Shadow& obj = *static_cast<Shadow*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(Shadow, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(Shadow, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("ShadowTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Shadow*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシング影");
						fi.offset_ = offsetof(Shadow, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingShadowSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Shadow*>(p); return o.enable_ && o.technique_ == ShadowTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("バーチャルシャドウマップ（VSM）");
						fi.offset_ = offsetof(Shadow, virtualShadowMap_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("VirtualShadowMapSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Shadow*>(p); return o.enable_ && o.technique_ == ShadowTechnique::VirtualShadowMap; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カスケードシャドウマップ（CSM）");
						fi.offset_ = offsetof(Shadow, cascadedShadowMap_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("CascadedShadowMapSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Shadow*>(p); return o.enable_ && o.technique_ == ShadowTechnique::CascadedShadowMap; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Shadow global_Shadow_register;

		// ---- GraphicsEngine/Illumination/SubsurfaceScattering/SubsurfaceScattering.h ----
		struct Register_RaytracingSubsurfaceScatteringSettings
		{
			Register_RaytracingSubsurfaceScatteringSettings()
			{
				ReflectionRegistry::Register(String("RaytracingSubsurfaceScatteringSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingSubsurfaceScatteringSettings& obj = *static_cast<RaytracingSubsurfaceScatteringSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("最大の厚み");
						fi.offset_ = offsetof(RaytracingSubsurfaceScatteringSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 50.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("厚み計算のオフセット");
						fi.offset_ = offsetof(RaytracingSubsurfaceScatteringSettings, thicknessBias_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RaytracingSubsurfaceScatteringSettings global_RaytracingSubsurfaceScatteringSettings_register;

		struct Register_SubsurfaceScattering
		{
			Register_SubsurfaceScattering()
			{
				ReflectionRegistry::Register(String("SubsurfaceScattering"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SubsurfaceScattering& obj = *static_cast<SubsurfaceScattering*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(SubsurfaceScattering, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(SubsurfaceScattering, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("SubsurfaceScatteringTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("散乱距離");
						fi.offset_ = offsetof(SubsurfaceScattering, scatteringDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_; };
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("透光の強さ");
						fi.offset_ = offsetof(SubsurfaceScattering, strength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("透けた光の色");
						fi.offset_ = offsetof(SubsurfaceScattering, subsurfaceColor_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシングサブサーフェススキャターリング");
						fi.offset_ = offsetof(SubsurfaceScattering, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingSubsurfaceScatteringSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_&& o.technique_ == SubsurfaceScatteringTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクリーンスペースサブサーフェススキャターリング");
						fi.offset_ = offsetof(SubsurfaceScattering, screenSpace_);
						fi.type_ = AttributeType::Unknown;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SubsurfaceScattering*>(p); return o.enable_&& o.technique_ == SubsurfaceScatteringTechnique::ScreenSpace; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SubsurfaceScattering global_SubsurfaceScattering_register;

		// ---- GraphicsEngine/Illumination/VolumetricLight/VolumetricLight.h ----
		struct Register_RaytracingVolumetricLightSettings
		{
			Register_RaytracingVolumetricLightSettings()
			{
				ReflectionRegistry::Register(String("RaytracingVolumetricLightSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RaytracingVolumetricLightSettings& obj = *static_cast<RaytracingVolumetricLightSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("散乱の異方性");
						fi.offset_ = offsetof(RaytracingVolumetricLightSettings, scatteringG_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.95f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("シャドウレイの最大長");
						fi.offset_ = offsetof(RaytracingVolumetricLightSettings, rayTMax_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 10.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ゴッドレイの強さ");
						fi.offset_ = offsetof(RaytracingVolumetricLightSettings, godrayStrength_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("雲の影"), offsetof(RaytracingVolumetricLightSettings, cloudShadowEnabled_), AttributeType::Bool });
				});
			}
		};
		static Register_RaytracingVolumetricLightSettings global_RaytracingVolumetricLightSettings_register;

		struct Register_VolumetricLight
		{
			Register_VolumetricLight()
			{
				ReflectionRegistry::Register(String("VolumetricLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					VolumetricLight& obj = *static_cast<VolumetricLight*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(VolumetricLight, enable_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("手法");
						fi.offset_ = offsetof(VolumetricLight, technique_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("VolumetricLightTechnique");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("フォグの密度");
						fi.offset_ = offsetof(VolumetricLight, density_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						fi.clampMin_ = 0.0001f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("吸収");
						fi.offset_ = offsetof(VolumetricLight, absorption_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.2f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("高さによる減衰");
						fi.offset_ = offsetof(VolumetricLight, heightFalloff_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("基準の高さ");
						fi.offset_ = offsetof(VolumetricLight, heightReference_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						fi.clampMin_ = -100.0f;
						fi.clampMax_ = 500.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("フォグの散乱色");
						fi.offset_ = offsetof(VolumetricLight, fogAlbedo_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レイトレーシングボリュームライト");
						fi.offset_ = offsetof(VolumetricLight, raytracing_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("RaytracingVolumetricLightSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_&& o.technique_ == VolumetricLightTechnique::Raytracing; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("高さフォグ");
						fi.offset_ = offsetof(VolumetricLight, heightFog_);
						fi.type_ = AttributeType::Unknown;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VolumetricLight*>(p); return o.enable_&& o.technique_ == VolumetricLightTechnique::HeightFog; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_VolumetricLight global_VolumetricLight_register;

		// ---- GraphicsEngine/Light/DirectionalLight.h ----
		struct Register_DirectionalLight
		{
			Register_DirectionalLight()
			{
				ReflectionRegistry::Register(String("DirectionalLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DirectionalLight& obj = *static_cast<DirectionalLight*>(ptr);
					outInfo.push_back({ String("色"), offsetof(DirectionalLight, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(DirectionalLight, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 200.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("向き");
						fi.offset_ = offsetof(DirectionalLight, direction_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_DirectionalLight global_DirectionalLight_register;

		// ---- GraphicsEngine/Light/PointLight.h ----
		struct Register_PointLight
		{
			Register_PointLight()
			{
				ReflectionRegistry::Register(String("PointLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PointLight& obj = *static_cast<PointLight*>(ptr);
					outInfo.push_back({ String("色"), offsetof(PointLight, color_), AttributeType::Color });
					outInfo.push_back({ String("範囲"), offsetof(PointLight, range_), AttributeType::Float });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(PointLight, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 200.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_PointLight global_PointLight_register;

		// ---- GraphicsEngine/Light/RectangleLight.h ----
		struct Register_RectangleLight
		{
			Register_RectangleLight()
			{
				ReflectionRegistry::Register(String("RectangleLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RectangleLight& obj = *static_cast<RectangleLight*>(ptr);
					outInfo.push_back({ String("色"), offsetof(RectangleLight, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("正面の向き");
						fi.offset_ = offsetof(RectangleLight, direction_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("上方向");
						fi.offset_ = offsetof(RectangleLight, up_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("幅");
						fi.offset_ = offsetof(RectangleLight, width_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 100.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("高さ");
						fi.offset_ = offsetof(RectangleLight, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 100.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("範囲"), offsetof(RectangleLight, range_), AttributeType::Float });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(RectangleLight, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 200.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RectangleLight global_RectangleLight_register;

		// ---- GraphicsEngine/Light/SkyLight.h ----
		struct Register_SkyLight
		{
			Register_SkyLight()
			{
				ReflectionRegistry::Register(String("SkyLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SkyLight& obj = *static_cast<SkyLight*>(ptr);
					outInfo.push_back({ String("スカイマップ使用"), offsetof(SkyLight, useSkymap_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(SkyLight, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SkyLight global_SkyLight_register;

		// ---- GraphicsEngine/Light/SpotLight.h ----
		struct Register_SpotLight
		{
			Register_SpotLight()
			{
				ReflectionRegistry::Register(String("SpotLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SpotLight& obj = *static_cast<SpotLight*>(ptr);
					outInfo.push_back({ String("色"), offsetof(SpotLight, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("向き");
						fi.offset_ = offsetof(SpotLight, direction_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("範囲"), offsetof(SpotLight, range_), AttributeType::Float });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(SpotLight, intensity_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 200.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スポット角度");
						fi.offset_ = offsetof(SpotLight, spotAngle_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 179.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ソフトネス");
						fi.offset_ = offsetof(SpotLight, softness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SpotLight global_SpotLight_register;

		// ---- GraphicsEngine/Model/Animation/Animator.h ----
		struct Register_AnimationParameter
		{
			Register_AnimationParameter()
			{
				ReflectionRegistry::Register(String("AnimationParameter"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AnimationParameter& obj = *static_cast<AnimationParameter*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("name_");
						fi.offset_ = offsetof(AnimationParameter, name_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("type_");
						fi.offset_ = offsetof(AnimationParameter, type_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("AnimationParameterType");
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("value_");
						fi.offset_ = offsetof(AnimationParameter, value_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AnimationParameter global_AnimationParameter_register;

		struct Register_AnimationCondition
		{
			Register_AnimationCondition()
			{
				ReflectionRegistry::Register(String("AnimationCondition"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AnimationCondition& obj = *static_cast<AnimationCondition*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("parameterName_");
						fi.offset_ = offsetof(AnimationCondition, parameterName_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("comparison_");
						fi.offset_ = offsetof(AnimationCondition, comparison_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("AnimationConditionComparison");
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("value_");
						fi.offset_ = offsetof(AnimationCondition, value_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("isOr_");
						fi.offset_ = offsetof(AnimationCondition, isOr_);
						fi.type_ = AttributeType::Bool;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AnimationCondition global_AnimationCondition_register;

		struct Register_AnimationState
		{
			Register_AnimationState()
			{
				ReflectionRegistry::Register(String("AnimationState"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AnimationState& obj = *static_cast<AnimationState*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("name_");
						fi.offset_ = offsetof(AnimationState, name_);
						fi.type_ = AttributeType::String;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("animationID_");
						fi.offset_ = offsetof(AnimationState, animationID_);
						fi.type_ = AttributeType::Int;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("nodePositionX_");
						fi.offset_ = offsetof(AnimationState, nodePositionX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("nodePositionY_");
						fi.offset_ = offsetof(AnimationState, nodePositionY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("useRootMotion_");
						fi.offset_ = offsetof(AnimationState, useRootMotion_);
						fi.type_ = AttributeType::Bool;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AnimationState global_AnimationState_register;

		struct Register_AnimationTransition
		{
			Register_AnimationTransition()
			{
				ReflectionRegistry::Register(String("AnimationTransition"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AnimationTransition& obj = *static_cast<AnimationTransition*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("fromState_");
						fi.offset_ = offsetof(AnimationTransition, fromState_);
						fi.type_ = AttributeType::Int;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("toState_");
						fi.offset_ = offsetof(AnimationTransition, toState_);
						fi.type_ = AttributeType::Int;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("duration_");
						fi.offset_ = offsetof(AnimationTransition, duration_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("hasExitTime_");
						fi.offset_ = offsetof(AnimationTransition, hasExitTime_);
						fi.type_ = AttributeType::Bool;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("exitTime_");
						fi.offset_ = offsetof(AnimationTransition, exitTime_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("fromOffsetX_");
						fi.offset_ = offsetof(AnimationTransition, fromOffsetX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("fromOffsetY_");
						fi.offset_ = offsetof(AnimationTransition, fromOffsetY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("toOffsetX_");
						fi.offset_ = offsetof(AnimationTransition, toOffsetX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("toOffsetY_");
						fi.offset_ = offsetof(AnimationTransition, toOffsetY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						auto& arr = obj.conditions_;
						FieldInfo header;
						header.name_ = String("conditions_");
						header.offset_ = 0;
						header.type_ = AttributeType::Struct;
						header.nestedTypeName_ = String("AnimationCondition");
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.conditions_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.conditions_.size()) obj.conditions_.erase(obj.conditions_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo;
							elementInfo.offset_ = 0;
							elementInfo.type_ = AttributeType::Struct;
							elementInfo.directPtr_ = &arr[i];
							elementInfo.nestedTypeName_ = String("AnimationCondition");
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
				});
			}
		};
		static Register_AnimationTransition global_AnimationTransition_register;

		struct Register_Animator
		{
			Register_Animator()
			{
				ReflectionRegistry::Register(String("Animator"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Animator& obj = *static_cast<Animator*>(ptr);
					outInfo.push_back({ String("アニメーション速度"), offsetof(Animator, animationSpeed_), AttributeType::Float });
					{
						auto& arr = obj.parameters_;
						FieldInfo header;
						header.name_ = String("parameters_");
						header.offset_ = 0;
						header.type_ = AttributeType::Struct;
						header.nestedTypeName_ = String("AnimationParameter");
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.parameters_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.parameters_.size()) obj.parameters_.erase(obj.parameters_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo;
							elementInfo.offset_ = 0;
							elementInfo.type_ = AttributeType::Struct;
							elementInfo.directPtr_ = &arr[i];
							elementInfo.nestedTypeName_ = String("AnimationParameter");
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
					{
						auto& arr = obj.states_;
						FieldInfo header;
						header.name_ = String("states_");
						header.offset_ = 0;
						header.type_ = AttributeType::Struct;
						header.nestedTypeName_ = String("AnimationState");
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.states_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.states_.size()) obj.states_.erase(obj.states_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo;
							elementInfo.offset_ = 0;
							elementInfo.type_ = AttributeType::Struct;
							elementInfo.directPtr_ = &arr[i];
							elementInfo.nestedTypeName_ = String("AnimationState");
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
					{
						auto& arr = obj.transitions_;
						FieldInfo header;
						header.name_ = String("transitions_");
						header.offset_ = 0;
						header.type_ = AttributeType::Struct;
						header.nestedTypeName_ = String("AnimationTransition");
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.transitions_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.transitions_.size()) obj.transitions_.erase(obj.transitions_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo;
							elementInfo.offset_ = 0;
							elementInfo.type_ = AttributeType::Struct;
							elementInfo.directPtr_ = &arr[i];
							elementInfo.nestedTypeName_ = String("AnimationTransition");
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
					{
						FieldInfo fi;
						fi.name_ = String("entryStateIndex_");
						fi.offset_ = offsetof(Animator, entryStateIndex_);
						fi.type_ = AttributeType::Int;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("entryNodePositionX_");
						fi.offset_ = offsetof(Animator, entryNodePositionX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("entryNodePositionY_");
						fi.offset_ = offsetof(Animator, entryNodePositionY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("exitNodePositionX_");
						fi.offset_ = offsetof(Animator, exitNodePositionX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("exitNodePositionY_");
						fi.offset_ = offsetof(Animator, exitNodePositionY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("anyNodePositionX_");
						fi.offset_ = offsetof(Animator, anyNodePositionX_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("anyNodePositionY_");
						fi.offset_ = offsetof(Animator, anyNodePositionY_);
						fi.type_ = AttributeType::Float;
						fi.editorVisible_ = false;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Animator global_Animator_register;

		// ---- GraphicsEngine/Model/Material/Material.h ----
		struct Register_Material
		{
			Register_Material()
			{
				ReflectionRegistry::Register(String("Material"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Material& obj = *static_cast<Material*>(ptr);
					{
						auto& arr = obj.slotMaterialIDs_;
						FieldInfo header;
						header.name_ = String("slotMaterialIDs_");
						header.offset_ = 0;
						header.type_ = AttributeType::Int;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.slotMaterialIDs_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.slotMaterialIDs_.size()) obj.slotMaterialIDs_.erase(obj.slotMaterialIDs_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo{ String("[" + std::to_string(i) + "]"), 0, AttributeType::Int, PayloadType::None, &arr[i] };
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
					{
						auto& arr = obj.slotNames_;
						FieldInfo header;
						header.name_ = String("slotNames_");
						header.offset_ = 0;
						header.type_ = AttributeType::String;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.slotNames_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.slotNames_.size()) obj.slotNames_.erase(obj.slotNames_.begin() + idx); };
						header.editorVisible_ = false;
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							FieldInfo elementInfo{ String("[" + std::to_string(i) + "]"), 0, AttributeType::String, PayloadType::None, &arr[i] };
							elementInfo.editorVisible_ = false;
							outInfo.push_back(std::move(elementInfo));
						}
					}
				});
			}
		};
		static Register_Material global_Material_register;

		// ---- GraphicsEngine/Movie/Movie.h ----
		struct Register_Movie
		{
			Register_Movie()
			{
				ReflectionRegistry::Register(String("Movie"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Movie& obj = *static_cast<Movie*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("表示形式");
						fi.offset_ = offsetof(Movie, displayMode_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("Movie::DisplayMode");
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("自動再生"), offsetof(Movie, autoPlay_), AttributeType::Bool });
					outInfo.push_back({ String("ループ再生"), offsetof(Movie, loop_), AttributeType::Bool });
					outInfo.push_back({ String("色"), offsetof(Movie, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(Movie, size_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Movie*>(p); return o.displayMode_ != Movie::DisplayMode::Fullscreen; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("中心点");
						fi.offset_ = offsetof(Movie, pivot_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Movie*>(p); return o.displayMode_ != Movie::DisplayMode::Fullscreen; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("描画順");
						fi.offset_ = offsetof(Movie, order_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Movie*>(p); return o.displayMode_ == Movie::DisplayMode::Sprite; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カメラ正対");
						fi.offset_ = offsetof(Movie, faceCamera_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Movie*>(p); return o.displayMode_ == Movie::DisplayMode::Billboard; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Movie global_Movie_register;

		// ---- GraphicsEngine/PostProcess/PostProcess.h ----
		struct Register_DepthOfFieldSettings
		{
			Register_DepthOfFieldSettings()
			{
				ReflectionRegistry::Register(String("DepthOfFieldSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DepthOfFieldSettings& obj = *static_cast<DepthOfFieldSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(DepthOfFieldSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("焦点距離");
						fi.offset_ = offsetof(DepthOfFieldSettings, focusDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<DepthOfFieldSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 100.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("焦点範囲");
						fi.offset_ = offsetof(DepthOfFieldSettings, focusRange_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<DepthOfFieldSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 50.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大ぼけ半径");
						fi.offset_ = offsetof(DepthOfFieldSettings, maxBlurRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<DepthOfFieldSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.05f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_DepthOfFieldSettings global_DepthOfFieldSettings_register;

		struct Register_BokehSettings
		{
			Register_BokehSettings()
			{
				ReflectionRegistry::Register(String("BokehSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BokehSettings& obj = *static_cast<BokehSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(BokehSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("しきい値");
						fi.offset_ = offsetof(BokehSettings, highlightThreshold_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BokehSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(BokehSettings, highlightIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BokehSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("角形の頂点数");
						fi.offset_ = offsetof(BokehSettings, bladeCount_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BokehSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 3;
						fi.clampMax_ = 8;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_BokehSettings global_BokehSettings_register;

		struct Register_LensDistortionSettings
		{
			Register_LensDistortionSettings()
			{
				ReflectionRegistry::Register(String("LensDistortionSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LensDistortionSettings& obj = *static_cast<LensDistortionSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(LensDistortionSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("歪曲 k1");
						fi.offset_ = offsetof(LensDistortionSettings, k1_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensDistortionSettings*>(p); return o.enabled_; };
						fi.clampMin_ = -0.5f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("歪曲 k2");
						fi.offset_ = offsetof(LensDistortionSettings, k2_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensDistortionSettings*>(p); return o.enabled_; };
						fi.clampMin_ = -0.5f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("歪曲 k3");
						fi.offset_ = offsetof(LensDistortionSettings, k3_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensDistortionSettings*>(p); return o.enabled_; };
						fi.clampMin_ = -0.5f;
						fi.clampMax_ = 0.5f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スケール");
						fi.offset_ = offsetof(LensDistortionSettings, scale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensDistortionSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.5f;
						fi.clampMax_ = 1.5f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_LensDistortionSettings global_LensDistortionSettings_register;

		struct Register_ChromaticAberrationSettings
		{
			Register_ChromaticAberrationSettings()
			{
				ReflectionRegistry::Register(String("ChromaticAberrationSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ChromaticAberrationSettings& obj = *static_cast<ChromaticAberrationSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(ChromaticAberrationSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(ChromaticAberrationSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ChromaticAberrationSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.05f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サンプル数");
						fi.offset_ = offsetof(ChromaticAberrationSettings, sampleCount_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ChromaticAberrationSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 3;
						fi.clampMax_ = 16;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ChromaticAberrationSettings global_ChromaticAberrationSettings_register;

		struct Register_VignetteSettings
		{
			Register_VignetteSettings()
			{
				ReflectionRegistry::Register(String("VignetteSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					VignetteSettings& obj = *static_cast<VignetteSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(VignetteSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(VignetteSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VignetteSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減光指数");
						fi.offset_ = offsetof(VignetteSettings, exponent_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VignetteSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("色");
						fi.offset_ = offsetof(VignetteSettings, color_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<VignetteSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_VignetteSettings global_VignetteSettings_register;

		struct Register_BloomSettings
		{
			Register_BloomSettings()
			{
				ReflectionRegistry::Register(String("BloomSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BloomSettings& obj = *static_cast<BloomSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(BloomSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("しきい値");
						fi.offset_ = offsetof(BloomSettings, threshold_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BloomSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ソフトニー");
						fi.offset_ = offsetof(BloomSettings, softKnee_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BloomSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(BloomSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BloomSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("フィルタ半径");
						fi.offset_ = offsetof(BloomSettings, filterRadius_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BloomSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 0.02f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_BloomSettings global_BloomSettings_register;

		struct Register_AnamorphicFlareSettings
		{
			Register_AnamorphicFlareSettings()
			{
				ReflectionRegistry::Register(String("AnamorphicFlareSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AnamorphicFlareSettings& obj = *static_cast<AnamorphicFlareSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(AnamorphicFlareSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("しきい値");
						fi.offset_ = offsetof(AnamorphicFlareSettings, threshold_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AnamorphicFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(AnamorphicFlareSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AnamorphicFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("筋の長さ");
						fi.offset_ = offsetof(AnamorphicFlareSettings, streakLength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AnamorphicFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減衰");
						fi.offset_ = offsetof(AnamorphicFlareSettings, attenuation_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AnamorphicFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.80f;
						fi.clampMax_ = 0.99f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("色");
						fi.offset_ = offsetof(AnamorphicFlareSettings, tint_);
						fi.type_ = AttributeType::Color;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<AnamorphicFlareSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_AnamorphicFlareSettings global_AnamorphicFlareSettings_register;

		struct Register_LensFlareSettings
		{
			Register_LensFlareSettings()
			{
				ReflectionRegistry::Register(String("LensFlareSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LensFlareSettings& obj = *static_cast<LensFlareSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(LensFlareSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("しきい値");
						fi.offset_ = offsetof(LensFlareSettings, threshold_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("全体強度");
						fi.offset_ = offsetof(LensFlareSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 5.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スパイク長");
						fi.offset_ = offsetof(LensFlareSettings, streakLength_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減衰");
						fi.offset_ = offsetof(LensFlareSettings, streakAttenuation_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.80f;
						fi.clampMax_ = 0.99f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("色収差");
						fi.offset_ = offsetof(LensFlareSettings, chromaticAberration_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.02f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("回転");
						fi.offset_ = offsetof(LensFlareSettings, angleOffset_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = -3.14159265f;
						fi.clampMax_ = 3.14159265f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ゴースト数");
						fi.offset_ = offsetof(LensFlareSettings, ghostCount_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 1;
						fi.clampMax_ = 8;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ゴースト間隔");
						fi.offset_ = offsetof(LensFlareSettings, ghostDispersal_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ゴースト強度");
						fi.offset_ = offsetof(LensFlareSettings, ghostIntensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ハロー半径");
						fi.offset_ = offsetof(LensFlareSettings, haloWidth_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("棘のばらつき");
						fi.offset_ = offsetof(LensFlareSettings, spikeVariation_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<LensFlareSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_LensFlareSettings global_LensFlareSettings_register;

		struct Register_ExposureSettings
		{
			Register_ExposureSettings()
			{
				ReflectionRegistry::Register(String("ExposureSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ExposureSettings& obj = *static_cast<ExposureSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("露出補正(EV)");
						fi.offset_ = offsetof(ExposureSettings, compensation_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = -8.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("自動露出(ヒストグラム)を使う"), offsetof(ExposureSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("最小log輝度");
						fi.offset_ = offsetof(ExposureSettings, minLogLuminance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ExposureSettings*>(p); return o.enabled_; };
						fi.clampMin_ = -16.0f;
						fi.clampMax_ = 0.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大log輝度");
						fi.offset_ = offsetof(ExposureSettings, maxLogLuminance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ExposureSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 16.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("目標輝度(キー値)");
						fi.offset_ = offsetof(ExposureSettings, keyValue_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ExposureSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("明順応速度");
						fi.offset_ = offsetof(ExposureSettings, adaptSpeedToBright_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ExposureSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("暗順応速度");
						fi.offset_ = offsetof(ExposureSettings, adaptSpeedToDark_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ExposureSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.01f;
						fi.clampMax_ = 20.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ExposureSettings global_ExposureSettings_register;

		struct Register_ColorGradingRangeSettings
		{
			Register_ColorGradingRangeSettings()
			{
				ReflectionRegistry::Register(String("ColorGradingRangeSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ColorGradingRangeSettings& obj = *static_cast<ColorGradingRangeSettings*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("色温度");
						fi.offset_ = offsetof(ColorGradingRangeSettings, temperature_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("彩度");
						fi.offset_ = offsetof(ColorGradingRangeSettings, saturation_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("コントラスト");
						fi.offset_ = offsetof(ColorGradingRangeSettings, contrast_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ガンマ");
						fi.offset_ = offsetof(ColorGradingRangeSettings, gamma_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.1f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ゲイン");
						fi.offset_ = offsetof(ColorGradingRangeSettings, gain_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 4.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("オフセット");
						fi.offset_ = offsetof(ColorGradingRangeSettings, offset_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ColorGradingRangeSettings global_ColorGradingRangeSettings_register;

		struct Register_ColorGradingSettings
		{
			Register_ColorGradingSettings()
			{
				ReflectionRegistry::Register(String("ColorGradingSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ColorGradingSettings& obj = *static_cast<ColorGradingSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(ColorGradingSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("全体");
						fi.offset_ = offsetof(ColorGradingSettings, global_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ColorGradingRangeSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("シャドウ");
						fi.offset_ = offsetof(ColorGradingSettings, shadows_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ColorGradingRangeSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("中間調");
						fi.offset_ = offsetof(ColorGradingSettings, midtones_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ColorGradingRangeSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ハイライト");
						fi.offset_ = offsetof(ColorGradingSettings, highlights_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ColorGradingRangeSettings");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("シャドウ上限");
						fi.offset_ = offsetof(ColorGradingSettings, shadowsMax_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ハイライト下限");
						fi.offset_ = offsetof(ColorGradingSettings, highlightsMin_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ColorGradingSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ColorGradingSettings global_ColorGradingSettings_register;

		struct Register_ToneMappingSettings
		{
			Register_ToneMappingSettings()
			{
				ReflectionRegistry::Register(String("ToneMappingSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ToneMappingSettings& obj = *static_cast<ToneMappingSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(ToneMappingSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("方式");
						fi.offset_ = offsetof(ToneMappingSettings, mode_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("ToneMappingSettings::ToneCurve");
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ToneMappingSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ToneMappingSettings global_ToneMappingSettings_register;

		struct Register_SharpnessSettings
		{
			Register_SharpnessSettings()
			{
				ReflectionRegistry::Register(String("SharpnessSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SharpnessSettings& obj = *static_cast<SharpnessSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(SharpnessSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("強さ");
						fi.offset_ = offsetof(SharpnessSettings, amount_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SharpnessSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 2.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SharpnessSettings global_SharpnessSettings_register;

		struct Register_FilmGrainSettings
		{
			Register_FilmGrainSettings()
			{
				ReflectionRegistry::Register(String("FilmGrainSettings"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FilmGrainSettings& obj = *static_cast<FilmGrainSettings*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(FilmGrainSettings, enabled_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("強度");
						fi.offset_ = offsetof(FilmGrainSettings, intensity_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<FilmGrainSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("粒の大きさ");
						fi.offset_ = offsetof(FilmGrainSettings, size_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<FilmGrainSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 8.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("階調応答");
						fi.offset_ = offsetof(FilmGrainSettings, luminanceResponse_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<FilmGrainSettings*>(p); return o.enabled_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("色付き");
						fi.offset_ = offsetof(FilmGrainSettings, colored_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<FilmGrainSettings*>(p); return o.enabled_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_FilmGrainSettings global_FilmGrainSettings_register;

		struct Register_PostProcess
		{
			Register_PostProcess()
			{
				ReflectionRegistry::Register(String("PostProcess"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PostProcess& obj = *static_cast<PostProcess*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("被写界深度");
						fi.offset_ = offsetof(PostProcess, depthOfField_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("DepthOfFieldSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ボケ");
						fi.offset_ = offsetof(PostProcess, bokeh_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("BokehSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レンズ歪曲");
						fi.offset_ = offsetof(PostProcess, lensDistortion_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("LensDistortionSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("色収差");
						fi.offset_ = offsetof(PostProcess, chromaticAberration_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ChromaticAberrationSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ビネット");
						fi.offset_ = offsetof(PostProcess, vignette_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("VignetteSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ブルーム");
						fi.offset_ = offsetof(PostProcess, bloom_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("BloomSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("アナモルフィックフレア");
						fi.offset_ = offsetof(PostProcess, anamorphicFlare_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("AnamorphicFlareSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("レンズフレア");
						fi.offset_ = offsetof(PostProcess, lensFlare_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("LensFlareSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("露出");
						fi.offset_ = offsetof(PostProcess, exposure_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ExposureSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カラーグレーディング");
						fi.offset_ = offsetof(PostProcess, colorGrading_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ColorGradingSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トーンマップ");
						fi.offset_ = offsetof(PostProcess, toneMapping_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("ToneMappingSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("シャープネス");
						fi.offset_ = offsetof(PostProcess, sharpness_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("SharpnessSettings");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("フィルムグレイン");
						fi.offset_ = offsetof(PostProcess, filmGrain_);
						fi.type_ = AttributeType::Struct;
						fi.nestedTypeName_ = String("FilmGrainSettings");
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_PostProcess global_PostProcess_register;

		// ---- GraphicsEngine/Shape/Primitive/BoxShape.h ----
		struct Register_BoxShape
		{
			Register_BoxShape()
			{
				ReflectionRegistry::Register(String("BoxShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BoxShape& obj = *static_cast<BoxShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(BoxShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BoxShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(BoxShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<BoxShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(BoxShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(BoxShape, size_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_BoxShape global_BoxShape_register;

		// ---- GraphicsEngine/Shape/Primitive/CapsuleShape.h ----
		struct Register_CapsuleShape
		{
			Register_CapsuleShape()
			{
				ReflectionRegistry::Register(String("CapsuleShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CapsuleShape& obj = *static_cast<CapsuleShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(CapsuleShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CapsuleShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(CapsuleShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CapsuleShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(CapsuleShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("円柱の高さ");
						fi.offset_ = offsetof(CapsuleShape, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CapsuleShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CapsuleShape global_CapsuleShape_register;

		// ---- GraphicsEngine/Shape/Primitive/ConeShape.h ----
		struct Register_ConeShape
		{
			Register_ConeShape()
			{
				ReflectionRegistry::Register(String("ConeShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ConeShape& obj = *static_cast<ConeShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(ConeShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ConeShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(ConeShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<ConeShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(ConeShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("高さ");
						fi.offset_ = offsetof(ConeShape, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(ConeShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_ConeShape global_ConeShape_register;

		// ---- GraphicsEngine/Shape/Primitive/CylinderShape.h ----
		struct Register_CylinderShape
		{
			Register_CylinderShape()
			{
				ReflectionRegistry::Register(String("CylinderShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CylinderShape& obj = *static_cast<CylinderShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(CylinderShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CylinderShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(CylinderShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CylinderShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(CylinderShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("高さ");
						fi.offset_ = offsetof(CylinderShape, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CylinderShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CylinderShape global_CylinderShape_register;

		// ---- GraphicsEngine/Shape/Primitive/DiscShape.h ----
		struct Register_DiscShape
		{
			Register_DiscShape()
			{
				ReflectionRegistry::Register(String("DiscShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DiscShape& obj = *static_cast<DiscShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(DiscShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<DiscShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(DiscShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<DiscShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(DiscShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(DiscShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("両面表示"), offsetof(DiscShape, doubleSided_), AttributeType::Bool });
				});
			}
		};
		static Register_DiscShape global_DiscShape_register;

		// ---- GraphicsEngine/Shape/Primitive/PlaneShape.h ----
		struct Register_PlaneShape
		{
			Register_PlaneShape()
			{
				ReflectionRegistry::Register(String("PlaneShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PlaneShape& obj = *static_cast<PlaneShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(PlaneShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<PlaneShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(PlaneShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<PlaneShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(PlaneShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(PlaneShape, size_);
						fi.type_ = AttributeType::Vector2;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("両面表示"), offsetof(PlaneShape, doubleSided_), AttributeType::Bool });
				});
			}
		};
		static Register_PlaneShape global_PlaneShape_register;

		// ---- GraphicsEngine/Shape/Primitive/RampShape.h ----
		struct Register_RampShape
		{
			Register_RampShape()
			{
				ReflectionRegistry::Register(String("RampShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RampShape& obj = *static_cast<RampShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(RampShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<RampShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(RampShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<RampShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(RampShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(RampShape, size_);
						fi.type_ = AttributeType::Vector3;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RampShape global_RampShape_register;

		// ---- GraphicsEngine/Shape/Primitive/SegmentShape.h ----
		struct Register_SegmentShape
		{
			Register_SegmentShape()
			{
				ReflectionRegistry::Register(String("SegmentShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SegmentShape& obj = *static_cast<SegmentShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(SegmentShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SegmentShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(SegmentShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SegmentShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(SegmentShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("長さ");
						fi.offset_ = offsetof(SegmentShape, length_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("幅");
						fi.offset_ = offsetof(SegmentShape, width_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SegmentShape global_SegmentShape_register;

		// ---- GraphicsEngine/Shape/Primitive/SphereShape.h ----
		struct Register_SphereShape
		{
			Register_SphereShape()
			{
				ReflectionRegistry::Register(String("SphereShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SphereShape& obj = *static_cast<SphereShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(SphereShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SphereShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(SphereShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SphereShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(SphereShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(SphereShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SphereShape global_SphereShape_register;

		// ---- GraphicsEngine/Shape/Primitive/TorusShape.h ----
		struct Register_TorusShape
		{
			Register_TorusShape()
			{
				ReflectionRegistry::Register(String("TorusShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					TorusShape& obj = *static_cast<TorusShape*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("UVの繰り返し");
						fi.offset_ = offsetof(TorusShape, uvScale_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<TorusShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("UVのずらし");
						fi.offset_ = offsetof(TorusShape, uvOffset_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<TorusShape*>(p); return o.textureID_ != 0; };
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("色"), offsetof(TorusShape, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("輪の半径");
						fi.offset_ = offsetof(TorusShape, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("太さの半径");
						fi.offset_ = offsetof(TorusShape, thickness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_TorusShape global_TorusShape_register;

		// ---- GraphicsEngine/Texture/Image.h ----
		struct Register_Image
		{
			Register_Image()
			{
				ReflectionRegistry::Register(String("Image"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Image& obj = *static_cast<Image*>(ptr);
					outInfo.push_back({ String("テクスチャサイズ"), offsetof(Image, textureSize_), AttributeType::Vector2 });
					outInfo.push_back({ String("テクスチャ位置"), offsetof(Image, texturePosition_), AttributeType::Vector2 });
					outInfo.push_back({ String("中心点"), offsetof(Image, pivot_), AttributeType::Vector2 });
					outInfo.push_back({ String("色"), offsetof(Image, color_), AttributeType::Color });
					{
						FieldInfo fi;
						fi.name_ = String("表示形式");
						fi.offset_ = offsetof(Image, viewType_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("Image::ViewType");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("描画順");
						fi.offset_ = offsetof(Image, order_);
						fi.type_ = AttributeType::Int;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Image*>(p); return o.viewType_ == Image::ViewType::Sprite; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("カメラ正対");
						fi.offset_ = offsetof(Image, faceCamera_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Image*>(p); return o.viewType_ == Image::ViewType::Billboard; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("テクスチャタイプ");
						fi.offset_ = offsetof(Image, motionType_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("Image::MotionType");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクロール速度");
						fi.offset_ = offsetof(Image, scrollSpeed_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Image*>(p); return o.motionType_ == Image::MotionType::Dynamic; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("スクロール向き");
						fi.offset_ = offsetof(Image, scrollDirection_);
						fi.type_ = AttributeType::Vector2;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Image*>(p); return o.motionType_ == Image::MotionType::Dynamic; };
						fi.clampMin_ = -1.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Image global_Image_register;

		// ---- PhysicsEngine/CharacterController/CharacterController.h ----
		struct Register_CharacterController
		{
			Register_CharacterController()
			{
				ReflectionRegistry::Register(String("CharacterController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CharacterController& obj = *static_cast<CharacterController*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CharacterController, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("高さ");
						fi.offset_ = offsetof(CharacterController, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("質量");
						fi.offset_ = offsetof(CharacterController, mass_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("押し出し力");
						fi.offset_ = offsetof(CharacterController, pushForce_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大移動速度");
						fi.offset_ = offsetof(CharacterController, maxMoveSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("加速度");
						fi.offset_ = offsetof(CharacterController, acceleration_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減速度");
						fi.offset_ = offsetof(CharacterController, deceleration_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("回転速度");
						fi.offset_ = offsetof(CharacterController, turnSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("空気抵抗");
						fi.offset_ = offsetof(CharacterController, airDrag_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大斜面角度");
						fi.offset_ = offsetof(CharacterController, maxSlopeAngle_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 89.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大許容段差高");
						fi.offset_ = offsetof(CharacterController, maxStepHeight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("重力倍率");
						fi.offset_ = offsetof(CharacterController, gravityScale_);
						fi.type_ = AttributeType::Float;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大落下速度");
						fi.offset_ = offsetof(CharacterController, maxFallSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ジャンプ力");
						fi.offset_ = offsetof(CharacterController, jumpPower_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("しゃがみ");
						fi.offset_ = offsetof(CharacterController, crouch_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("しゃがみ時の高さ");
						fi.offset_ = offsetof(CharacterController, crouchHeight_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CharacterController global_CharacterController_register;

		// ---- PhysicsEngine/Collider/BoxCollider.h ----
		struct Register_BoxCollider
		{
			Register_BoxCollider()
			{
				ReflectionRegistry::Register(String("BoxCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BoxCollider& obj = *static_cast<BoxCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(BoxCollider, size_);
						fi.type_ = AttributeType::Vector3;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("中心オフセット");
						fi.offset_ = offsetof(BoxCollider, center_);
						fi.type_ = AttributeType::Vector3;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(BoxCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_BoxCollider global_BoxCollider_register;

		// ---- PhysicsEngine/Collider/CapsuleCollider.h ----
		struct Register_CapsuleCollider
		{
			Register_CapsuleCollider()
			{
				ReflectionRegistry::Register(String("CapsuleCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CapsuleCollider& obj = *static_cast<CapsuleCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("円柱の高さ");
						fi.offset_ = offsetof(CapsuleCollider, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CapsuleCollider, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(CapsuleCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CapsuleCollider global_CapsuleCollider_register;

		// ---- PhysicsEngine/Collider/CircleCollider.h ----
		struct Register_CircleCollider
		{
			Register_CircleCollider()
			{
				ReflectionRegistry::Register(String("CircleCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CircleCollider& obj = *static_cast<CircleCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CircleCollider, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 1.0f;
						fi.clampMax_ = 10000.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("中心オフセット");
						fi.offset_ = offsetof(CircleCollider, center_);
						fi.type_ = AttributeType::Vector2;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(CircleCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CircleCollider global_CircleCollider_register;

		// ---- PhysicsEngine/Collider/CylinderCollider.h ----
		struct Register_CylinderCollider
		{
			Register_CylinderCollider()
			{
				ReflectionRegistry::Register(String("CylinderCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CylinderCollider& obj = *static_cast<CylinderCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("高さ");
						fi.offset_ = offsetof(CylinderCollider, height_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(CylinderCollider, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(CylinderCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_CylinderCollider global_CylinderCollider_register;

		// ---- PhysicsEngine/Collider/MeshCollider.h ----
		struct Register_MeshCollider
		{
			Register_MeshCollider()
			{
				ReflectionRegistry::Register(String("MeshCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					MeshCollider& obj = *static_cast<MeshCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("凸包にする");
						fi.offset_ = offsetof(MeshCollider, convex_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(MeshCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_MeshCollider global_MeshCollider_register;

		// ---- PhysicsEngine/Collider/RectCollider.h ----
		struct Register_RectCollider
		{
			Register_RectCollider()
			{
				ReflectionRegistry::Register(String("RectCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RectCollider& obj = *static_cast<RectCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("サイズ");
						fi.offset_ = offsetof(RectCollider, size_);
						fi.type_ = AttributeType::Vector2;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("中心オフセット");
						fi.offset_ = offsetof(RectCollider, center_);
						fi.type_ = AttributeType::Vector2;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(RectCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_RectCollider global_RectCollider_register;

		// ---- PhysicsEngine/Collider/SphereCollider.h ----
		struct Register_SphereCollider
		{
			Register_SphereCollider()
			{
				ReflectionRegistry::Register(String("SphereCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SphereCollider& obj = *static_cast<SphereCollider*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("半径");
						fi.offset_ = offsetof(SphereCollider, radius_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(SphereCollider, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SphereCollider global_SphereCollider_register;

		// ---- PhysicsEngine/Joint/FixedJoint.h ----
		struct Register_FixedJoint
		{
			Register_FixedJoint()
			{
				ReflectionRegistry::Register(String("FixedJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FixedJoint& obj = *static_cast<FixedJoint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(FixedJoint, enabled_), AttributeType::Bool });
				});
			}
		};
		static Register_FixedJoint global_FixedJoint_register;

		// ---- PhysicsEngine/Joint/HingeJoint.h ----
		struct Register_HingeJoint
		{
			Register_HingeJoint()
			{
				ReflectionRegistry::Register(String("HingeJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					HingeJoint& obj = *static_cast<HingeJoint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(HingeJoint, enabled_), AttributeType::Bool });
					outInfo.push_back({ String("アンカー(ローカル)"), offsetof(HingeJoint, anchor_), AttributeType::Vector3 });
					outInfo.push_back({ String("ヒンジ軸(ローカル)"), offsetof(HingeJoint, axis_), AttributeType::Vector3 });
					outInfo.push_back({ String("角度制限を使う"), offsetof(HingeJoint, useLimits_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("最小角(度)");
						fi.offset_ = offsetof(HingeJoint, minAngle_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<HingeJoint*>(p); return o.useLimits_; };
						fi.clampMin_ = -180.0f;
						fi.clampMax_ = 0.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大角(度)");
						fi.offset_ = offsetof(HingeJoint, maxAngle_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<HingeJoint*>(p); return o.useLimits_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 180.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_HingeJoint global_HingeJoint_register;

		// ---- PhysicsEngine/Joint/SliderJoint.h ----
		struct Register_SliderJoint
		{
			Register_SliderJoint()
			{
				ReflectionRegistry::Register(String("SliderJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SliderJoint& obj = *static_cast<SliderJoint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(SliderJoint, enabled_), AttributeType::Bool });
					outInfo.push_back({ String("スライド軸(ローカル)"), offsetof(SliderJoint, axis_), AttributeType::Vector3 });
					outInfo.push_back({ String("可動範囲制限を使う"), offsetof(SliderJoint, useLimits_), AttributeType::Bool });
					{
						FieldInfo fi;
						fi.name_ = String("最小距離");
						fi.offset_ = offsetof(SliderJoint, minDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SliderJoint*>(p); return o.useLimits_; };
						fi.clampMin_ = -1000.0f;
						fi.clampMax_ = 0.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大距離");
						fi.offset_ = offsetof(SliderJoint, maxDistance_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SliderJoint*>(p); return o.useLimits_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SliderJoint global_SliderJoint_register;

		// ---- PhysicsEngine/Joint/SpringJoint.h ----
		struct Register_SpringJoint
		{
			Register_SpringJoint()
			{
				ReflectionRegistry::Register(String("SpringJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SpringJoint& obj = *static_cast<SpringJoint*>(ptr);
					outInfo.push_back({ String("有効"), offsetof(SpringJoint, enabled_), AttributeType::Bool });
					outInfo.push_back({ String("アンカー(ローカル)"), offsetof(SpringJoint, anchor_), AttributeType::Vector3 });
					{
						FieldInfo fi;
						fi.name_ = String("最小距離");
						fi.offset_ = offsetof(SpringJoint, minDistance_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大距離");
						fi.offset_ = offsetof(SpringJoint, maxDistance_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("剛性(Hz)");
						fi.offset_ = offsetof(SpringJoint, frequency_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 30.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("減衰");
						fi.offset_ = offsetof(SpringJoint, damping_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SpringJoint global_SpringJoint_register;

		// ---- PhysicsEngine/Rigidbody/Rigidbody.h ----
		struct Register_Rigidbody
		{
			Register_Rigidbody()
			{
				ReflectionRegistry::Register(String("Rigidbody"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Rigidbody& obj = *static_cast<Rigidbody*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("動作モード");
						fi.offset_ = offsetof(Rigidbody, bodyType_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("Rigidbody::BodyType");
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("連続衝突判定");
						fi.offset_ = offsetof(Rigidbody, continuousCollision_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Rigidbody*>(p); return o.bodyType_ == Rigidbody::BodyType::Dynamic; };
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("質量");
						fi.offset_ = offsetof(Rigidbody, mass_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("空気抵抗");
						fi.offset_ = offsetof(Rigidbody, linearDrag_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("角抵抗");
						fi.offset_ = offsetof(Rigidbody, angularDrag_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("摩擦係数");
						fi.offset_ = offsetof(Rigidbody, friction_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("反発係数");
						fi.offset_ = offsetof(Rigidbody, restitution_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("重力");
						fi.offset_ = offsetof(Rigidbody, useGravity_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("重力倍率");
						fi.offset_ = offsetof(Rigidbody, gravityScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Rigidbody*>(p); return o.useGravity_; };
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("足場追従");
						fi.offset_ = offsetof(Rigidbody, followFoothold_);
						fi.type_ = AttributeType::Bool;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Rigidbody*>(p); return o.bodyType_ == Rigidbody::BodyType::Dynamic && !o.isTrigger_; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("足場の最大斜面角度");
						fi.offset_ = offsetof(Rigidbody, footholdSlopeAngle_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Rigidbody*>(p); return o.followFoothold_; };
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 89.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("位置X軸固定");
						fi.offset_ = offsetof(Rigidbody, freezePositionX_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("位置Y軸固定");
						fi.offset_ = offsetof(Rigidbody, freezePositionY_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("位置Z軸固定");
						fi.offset_ = offsetof(Rigidbody, freezePositionZ_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("回転X軸固定");
						fi.offset_ = offsetof(Rigidbody, freezeRotationX_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("回転Y軸固定");
						fi.offset_ = offsetof(Rigidbody, freezeRotationY_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("回転Z軸固定");
						fi.offset_ = offsetof(Rigidbody, freezeRotationZ_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("トリガー");
						fi.offset_ = offsetof(Rigidbody, isTrigger_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Rigidbody global_Rigidbody_register;

		// ---- PhysicsEngine/Softbody/Softbody.h ----
		struct Register_Softbody
		{
			Register_Softbody()
			{
				ReflectionRegistry::Register(String("Softbody"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Softbody& obj = *static_cast<Softbody*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("質量");
						fi.offset_ = offsetof(Softbody, mass_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.001f;
						fi.clampMax_ = 100.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("面積弾性力");
						fi.offset_ = offsetof(Softbody, areaStiffness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("体積弾性力");
						fi.offset_ = offsetof(Softbody, volumeStiffness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("抵抗力");
						fi.offset_ = offsetof(Softbody, damping_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("ポアソン比");
						fi.offset_ = offsetof(Softbody, poissonRatio_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 0.5f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("最大許容距離");
						fi.offset_ = offsetof(Softbody, maxDistance_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("辺補強係数");
						fi.offset_ = offsetof(Softbody, edgeStiffness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("曲耐性");
						fi.offset_ = offsetof(Softbody, bendStiffness_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("内部圧力");
						fi.offset_ = offsetof(Softbody, pressure_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = -10.0f;
						fi.clampMax_ = 10.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("摩擦係数");
						fi.offset_ = offsetof(Softbody, friction_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("反発係数");
						fi.offset_ = offsetof(Softbody, restitution_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 1.0f;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("重力");
						fi.offset_ = offsetof(Softbody, useGravity_);
						fi.type_ = AttributeType::Bool;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("重力倍率");
						fi.offset_ = offsetof(Softbody, gravityScale_);
						fi.type_ = AttributeType::Float;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<Softbody*>(p); return o.useGravity_; };
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サブステップ数");
						fi.offset_ = offsetof(Softbody, subSteps_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 20;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("反復回数");
						fi.offset_ = offsetof(Softbody, iterationCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 1;
						fi.clampMax_ = 10;
						fi.revision_ = true;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_Softbody global_Softbody_register;

		struct RegisterEnum_AmbientOcclusionTechnique
		{
			RegisterEnum_AmbientOcclusionTechnique()
			{
				EnumRegistry::Register(String("AmbientOcclusionTechnique"), {
					{ static_cast<Int>(AmbientOcclusionTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(AmbientOcclusionTechnique::GroundTruth), String("GroundTruth") },
					{ static_cast<Int>(AmbientOcclusionTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_AmbientOcclusionTechnique global_AmbientOcclusionTechnique_enum_register;

		struct RegisterEnum_AnimationConditionComparison
		{
			RegisterEnum_AnimationConditionComparison()
			{
				EnumRegistry::Register(String("AnimationConditionComparison"), {
					{ static_cast<Int>(AnimationConditionComparison::Equal), String("Equal") },
					{ static_cast<Int>(AnimationConditionComparison::NotEqual), String("NotEqual") },
					{ static_cast<Int>(AnimationConditionComparison::Greater), String("Greater") },
					{ static_cast<Int>(AnimationConditionComparison::Less), String("Less") },
					{ static_cast<Int>(AnimationConditionComparison::GreaterOrEqual), String("GreaterOrEqual") },
					{ static_cast<Int>(AnimationConditionComparison::LessOrEqual), String("LessOrEqual") },
				});
			}
		};
		static RegisterEnum_AnimationConditionComparison global_AnimationConditionComparison_enum_register;

		struct RegisterEnum_AnimationParameterType
		{
			RegisterEnum_AnimationParameterType()
			{
				EnumRegistry::Register(String("AnimationParameterType"), {
					{ static_cast<Int>(AnimationParameterType::Bool), String("Bool") },
					{ static_cast<Int>(AnimationParameterType::Float), String("Float") },
					{ static_cast<Int>(AnimationParameterType::Int), String("Int") },
					{ static_cast<Int>(AnimationParameterType::Trigger), String("Trigger") },
				});
			}
		};
		static RegisterEnum_AnimationParameterType global_AnimationParameterType_enum_register;

		struct RegisterEnum_CameraBrainMode
		{
			RegisterEnum_CameraBrainMode()
			{
				EnumRegistry::Register(String("CameraBrainMode"), {
					{ static_cast<Int>(CameraBrainMode::Free), String("Free") },
					{ static_cast<Int>(CameraBrainMode::Orbit), String("Orbit") },
					{ static_cast<Int>(CameraBrainMode::Follow), String("Follow") },
					{ static_cast<Int>(CameraBrainMode::Lookat), String("Lookat") },
					{ static_cast<Int>(CameraBrainMode::Lockon), String("Lockon") },
					{ static_cast<Int>(CameraBrainMode::Cinematic), String("Cinematic") },
				});
			}
		};
		static RegisterEnum_CameraBrainMode global_CameraBrainMode_enum_register;

		struct RegisterEnum_CausticsTechnique
		{
			RegisterEnum_CausticsTechnique()
			{
				EnumRegistry::Register(String("CausticsTechnique"), {
					{ static_cast<Int>(CausticsTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(CausticsTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_CausticsTechnique global_CausticsTechnique_enum_register;

		struct RegisterEnum_GlobalIlluminationTechnique
		{
			RegisterEnum_GlobalIlluminationTechnique()
			{
				EnumRegistry::Register(String("GlobalIlluminationTechnique"), {
					{ static_cast<Int>(GlobalIlluminationTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(GlobalIlluminationTechnique::DynamicDiffuse), String("DynamicDiffuse") },
					{ static_cast<Int>(GlobalIlluminationTechnique::LightPropagationVolumes), String("LightPropagationVolumes") },
					{ static_cast<Int>(GlobalIlluminationTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_GlobalIlluminationTechnique global_GlobalIlluminationTechnique_enum_register;

		struct RegisterEnum_Image_MotionType
		{
			RegisterEnum_Image_MotionType()
			{
				EnumRegistry::Register(String("Image::MotionType"), {
					{ static_cast<Int>(Image::MotionType::Static), String("Static") },
					{ static_cast<Int>(Image::MotionType::Dynamic), String("Dynamic") },
				});
			}
		};
		static RegisterEnum_Image_MotionType global_Image_MotionType_enum_register;

		struct RegisterEnum_Image_ViewType
		{
			RegisterEnum_Image_ViewType()
			{
				EnumRegistry::Register(String("Image::ViewType"), {
					{ static_cast<Int>(Image::ViewType::Sprite), String("Sprite") },
					{ static_cast<Int>(Image::ViewType::Billboard), String("Billboard") },
				});
			}
		};
		static RegisterEnum_Image_ViewType global_Image_ViewType_enum_register;

		struct RegisterEnum_Movie_DisplayMode
		{
			RegisterEnum_Movie_DisplayMode()
			{
				EnumRegistry::Register(String("Movie::DisplayMode"), {
					{ static_cast<Int>(Movie::DisplayMode::Fullscreen), String("Fullscreen") },
					{ static_cast<Int>(Movie::DisplayMode::Sprite), String("Sprite") },
					{ static_cast<Int>(Movie::DisplayMode::Billboard), String("Billboard") },
				});
			}
		};
		static RegisterEnum_Movie_DisplayMode global_Movie_DisplayMode_enum_register;

		struct RegisterEnum_ReflectionTechnique
		{
			RegisterEnum_ReflectionTechnique()
			{
				EnumRegistry::Register(String("ReflectionTechnique"), {
					{ static_cast<Int>(ReflectionTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(ReflectionTechnique::SignedDistanceField), String("SignedDistanceField") },
					{ static_cast<Int>(ReflectionTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_ReflectionTechnique global_ReflectionTechnique_enum_register;

		struct RegisterEnum_RefractionTechnique
		{
			RegisterEnum_RefractionTechnique()
			{
				EnumRegistry::Register(String("RefractionTechnique"), {
					{ static_cast<Int>(RefractionTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(RefractionTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_RefractionTechnique global_RefractionTechnique_enum_register;

		struct RegisterEnum_Rigidbody_BodyType
		{
			RegisterEnum_Rigidbody_BodyType()
			{
				EnumRegistry::Register(String("Rigidbody::BodyType"), {
					{ static_cast<Int>(Rigidbody::BodyType::Dynamic), String("Dynamic") },
					{ static_cast<Int>(Rigidbody::BodyType::Kinematic), String("Kinematic") },
					{ static_cast<Int>(Rigidbody::BodyType::Static), String("Static") },
				});
			}
		};
		static RegisterEnum_Rigidbody_BodyType global_Rigidbody_BodyType_enum_register;

		struct RegisterEnum_ShadowTechnique
		{
			RegisterEnum_ShadowTechnique()
			{
				EnumRegistry::Register(String("ShadowTechnique"), {
					{ static_cast<Int>(ShadowTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(ShadowTechnique::VirtualShadowMap), String("VirtualShadowMap") },
					{ static_cast<Int>(ShadowTechnique::CascadedShadowMap), String("CascadedShadowMap") },
				});
			}
		};
		static RegisterEnum_ShadowTechnique global_ShadowTechnique_enum_register;

		struct RegisterEnum_SubsurfaceScatteringTechnique
		{
			RegisterEnum_SubsurfaceScatteringTechnique()
			{
				EnumRegistry::Register(String("SubsurfaceScatteringTechnique"), {
					{ static_cast<Int>(SubsurfaceScatteringTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(SubsurfaceScatteringTechnique::ScreenSpace), String("ScreenSpace") },
				});
			}
		};
		static RegisterEnum_SubsurfaceScatteringTechnique global_SubsurfaceScatteringTechnique_enum_register;

		struct RegisterEnum_Text_ViewType
		{
			RegisterEnum_Text_ViewType()
			{
				EnumRegistry::Register(String("Text::ViewType"), {
					{ static_cast<Int>(Text::ViewType::Sprite), String("Sprite") },
					{ static_cast<Int>(Text::ViewType::Billboard), String("Billboard") },
				});
			}
		};
		static RegisterEnum_Text_ViewType global_Text_ViewType_enum_register;

		struct RegisterEnum_ToneMappingSettings_ToneCurve
		{
			RegisterEnum_ToneMappingSettings_ToneCurve()
			{
				EnumRegistry::Register(String("ToneMappingSettings::ToneCurve"), {
					{ static_cast<Int>(ToneMappingSettings::ToneCurve::None), String("None") },
					{ static_cast<Int>(ToneMappingSettings::ToneCurve::Reinhard), String("Reinhard") },
					{ static_cast<Int>(ToneMappingSettings::ToneCurve::AcesFilmic), String("AcesFilmic") },
					{ static_cast<Int>(ToneMappingSettings::ToneCurve::PbrNeutral), String("PbrNeutral") },
				});
			}
		};
		static RegisterEnum_ToneMappingSettings_ToneCurve global_ToneMappingSettings_ToneCurve_enum_register;

		struct RegisterEnum_VolumetricLightTechnique
		{
			RegisterEnum_VolumetricLightTechnique()
			{
				EnumRegistry::Register(String("VolumetricLightTechnique"), {
					{ static_cast<Int>(VolumetricLightTechnique::Raytracing), String("Raytracing") },
					{ static_cast<Int>(VolumetricLightTechnique::HeightFog), String("HeightFog") },
				});
			}
		};
		static RegisterEnum_VolumetricLightTechnique global_VolumetricLightTechnique_enum_register;

		struct RegisterEnum_WeatherType
		{
			RegisterEnum_WeatherType()
			{
				EnumRegistry::Register(String("WeatherType"), {
					{ static_cast<Int>(WeatherType::Clear), String("Clear") },
					{ static_cast<Int>(WeatherType::Cloudy), String("Cloudy") },
					{ static_cast<Int>(WeatherType::Overcast), String("Overcast") },
					{ static_cast<Int>(WeatherType::Rain), String("Rain") },
					{ static_cast<Int>(WeatherType::Snow), String("Snow") },
					{ static_cast<Int>(WeatherType::Storm), String("Storm") },
				});
			}
		};
		static RegisterEnum_WeatherType global_WeatherType_enum_register;

	}
}
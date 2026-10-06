#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <FoundationEngine/World/ECS/Component/Spawner.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Constraint/AttachmentConstraint.h>
#include <GraphicsEngine/Constraint/IKConstraint.h>
#include <GraphicsEngine/Constraint/LookAtConstraint.h>
#include <GraphicsEngine/Constraint/ParentConstraint.h>
#include <GraphicsEngine/Constraint/PositionConstraint.h>
#include <GraphicsEngine/Constraint/RotationConstraint.h>
#include <GraphicsEngine/Font/Text.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/Model/Animation/Animator.h>
#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Mesh.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>
#include <GraphicsEngine/Movie/Movie.h>
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
#include <PhysicsEngine/Collider/MeshCollider.h>
#include <PhysicsEngine/Joint/FixedJoint.h>
#include <PhysicsEngine/Joint/HingeJoint.h>
#include <PhysicsEngine/Joint/SliderJoint.h>
#include <PhysicsEngine/Joint/SpringJoint.h>

extern "C" int _force_payload_AudioSource = 0;
extern "C" int _force_payload_Spawner = 0;
extern "C" int _force_payload_CameraBrain = 0;
extern "C" int _force_payload_AttachmentConstraint = 0;
extern "C" int _force_payload_Effector = 0;
extern "C" int _force_payload_LookAtConstraint = 0;
extern "C" int _force_payload_ParentConstraint = 0;
extern "C" int _force_payload_PositionConstraint = 0;
extern "C" int _force_payload_RotationConstraint = 0;
extern "C" int _force_payload_Text = 0;
extern "C" int _force_payload_SkyLight = 0;
extern "C" int _force_payload_Mesh = 0;
extern "C" int _force_payload_Animator = 0;
extern "C" int _force_payload_Material = 0;
extern "C" int _force_payload_Skeleton = 0;
extern "C" int _force_payload_Movie = 0;
extern "C" int _force_payload_BoxShape = 0;
extern "C" int _force_payload_CapsuleShape = 0;
extern "C" int _force_payload_ConeShape = 0;
extern "C" int _force_payload_CylinderShape = 0;
extern "C" int _force_payload_DiscShape = 0;
extern "C" int _force_payload_PlaneShape = 0;
extern "C" int _force_payload_RampShape = 0;
extern "C" int _force_payload_SegmentShape = 0;
extern "C" int _force_payload_SphereShape = 0;
extern "C" int _force_payload_TorusShape = 0;
extern "C" int _force_payload_Image = 0;
extern "C" int _force_payload_MeshCollider = 0;
extern "C" int _force_payload_FixedJoint = 0;
extern "C" int _force_payload_HingeJoint = 0;
extern "C" int _force_payload_SliderJoint = 0;
extern "C" int _force_payload_SpringJoint = 0;

namespace SeedCore
{
	 namespace ScPayload
	 {
		// ---- AudioEngine/Audio/AudioSource.h ----
		struct Register_AudioSource
		{
			Register_AudioSource()
			{
				PayloadRegistry::Register(String("AudioSource"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AudioSource& obj = *static_cast<AudioSource*>(ptr);
					outInfo.push_back({ String("サウンド"), offsetof(AudioSource, soundID_), AttributeType::Int, PayloadType::Audio });
				});
			}
		};
		static Register_AudioSource global_AudioSource_register;

		// ---- FoundationEngine/World/ECS/Component/Spawner.h ----
		struct Register_Spawner
		{
			Register_Spawner()
			{
				PayloadRegistry::Register(String("Spawner"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Spawner& obj = *static_cast<Spawner*>(ptr);
					outInfo.push_back({ String("プレハブID"), offsetof(Spawner, prefabID_), AttributeType::Int, PayloadType::Prefab });
				});
			}
		};
		static Register_Spawner global_Spawner_register;

		// ---- GraphicsEngine/Camera/CameraBrain.h ----
		struct Register_CameraBrain
		{
			Register_CameraBrain()
			{
				PayloadRegistry::Register(String("CameraBrain"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CameraBrain& obj = *static_cast<CameraBrain*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("メインターゲット");
						fi.offset_ = offsetof(CameraBrain, mainTarget_);
						fi.type_ = AttributeType::Int;
						fi.assetType_ = PayloadType::Actor;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CameraBrain*>(p); return o.mode_ != CameraBrainMode::Free && o.mode_ != CameraBrainMode::Cinematic; };
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("サブターゲット");
						fi.offset_ = offsetof(CameraBrain, subTarget_);
						fi.type_ = AttributeType::Int;
						fi.assetType_ = PayloadType::Actor;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<CameraBrain*>(p); return o.mode_ == CameraBrainMode::Lockon; };
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
				PayloadRegistry::Register(String("AttachmentConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AttachmentConstraint& obj = *static_cast<AttachmentConstraint*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(AttachmentConstraint, target_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_AttachmentConstraint global_AttachmentConstraint_register;

		// ---- GraphicsEngine/Constraint/IKConstraint.h ----
		struct Register_Effector
		{
			Register_Effector()
			{
				PayloadRegistry::Register(String("Effector"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Effector& obj = *static_cast<Effector*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(Effector, target_), AttributeType::Int, PayloadType::Actor });
					outInfo.push_back({ String("ポール"), offsetof(Effector, pole_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_Effector global_Effector_register;

		// ---- GraphicsEngine/Constraint/LookAtConstraint.h ----
		struct Register_LookAtConstraint
		{
			Register_LookAtConstraint()
			{
				PayloadRegistry::Register(String("LookAtConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LookAtConstraint& obj = *static_cast<LookAtConstraint*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(LookAtConstraint, target_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_LookAtConstraint global_LookAtConstraint_register;

		// ---- GraphicsEngine/Constraint/ParentConstraint.h ----
		struct Register_ParentConstraint
		{
			Register_ParentConstraint()
			{
				PayloadRegistry::Register(String("ParentConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ParentConstraint& obj = *static_cast<ParentConstraint*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(ParentConstraint, target_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_ParentConstraint global_ParentConstraint_register;

		// ---- GraphicsEngine/Constraint/PositionConstraint.h ----
		struct Register_PositionConstraint
		{
			Register_PositionConstraint()
			{
				PayloadRegistry::Register(String("PositionConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PositionConstraint& obj = *static_cast<PositionConstraint*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(PositionConstraint, target_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_PositionConstraint global_PositionConstraint_register;

		// ---- GraphicsEngine/Constraint/RotationConstraint.h ----
		struct Register_RotationConstraint
		{
			Register_RotationConstraint()
			{
				PayloadRegistry::Register(String("RotationConstraint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RotationConstraint& obj = *static_cast<RotationConstraint*>(ptr);
					outInfo.push_back({ String("ターゲット"), offsetof(RotationConstraint, target_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_RotationConstraint global_RotationConstraint_register;

		// ---- GraphicsEngine/Font/Text.h ----
		struct Register_Text
		{
			Register_Text()
			{
				PayloadRegistry::Register(String("Text"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Text& obj = *static_cast<Text*>(ptr);
					outInfo.push_back({ String("フォントID"), offsetof(Text, fontID_), AttributeType::Int, PayloadType::Font });
				});
			}
		};
		static Register_Text global_Text_register;

		// ---- GraphicsEngine/Light/SkyLight.h ----
		struct Register_SkyLight
		{
			Register_SkyLight()
			{
				PayloadRegistry::Register(String("SkyLight"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SkyLight& obj = *static_cast<SkyLight*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("スカイマップID");
						fi.offset_ = offsetof(SkyLight, skymapID_);
						fi.type_ = AttributeType::Int;
						fi.assetType_ = PayloadType::Sky;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<SkyLight*>(p); return o.useSkymap_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_SkyLight global_SkyLight_register;

		// ---- GraphicsEngine/Model/Mesh.h ----
		struct Register_Mesh
		{
			Register_Mesh()
			{
				PayloadRegistry::Register(String("Mesh"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Mesh& obj = *static_cast<Mesh*>(ptr);
					outInfo.push_back({ String("メッシュID"), offsetof(Mesh, meshID_), AttributeType::Int, PayloadType::Model });
				});
			}
		};
		static Register_Mesh global_Mesh_register;

		// ---- GraphicsEngine/Model/Animation/Animator.h ----
		struct Register_Animator
		{
			Register_Animator()
			{
				PayloadRegistry::Register(String("Animator"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Animator& obj = *static_cast<Animator*>(ptr);
					{
						auto& arr = obj.animationIDs_;
						FieldInfo header;
						header.name_ = String("アニメーションID");
						header.offset_ = 0;
						header.type_ = AttributeType::Int;
						header.assetType_ = PayloadType::Animation;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.animationIDs_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.animationIDs_.size()) obj.animationIDs_.erase(obj.animationIDs_.begin() + idx); };
						header.array_.lastPtr_ = [&obj]() -> void* { return &obj.animationIDs_.back(); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							outInfo.push_back({ String("[" + std::to_string(i) + "]"), 0, AttributeType::Int, PayloadType::Animation, &arr[i] });
						}
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
				PayloadRegistry::Register(String("Material"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Material& obj = *static_cast<Material*>(ptr);
					{
						auto& arr = obj.materialIDs_;
						FieldInfo header;
						header.name_ = String("マテリアル");
						header.offset_ = 0;
						header.type_ = AttributeType::Int;
						header.assetType_ = PayloadType::Material;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.materialIDs_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.materialIDs_.size()) obj.materialIDs_.erase(obj.materialIDs_.begin() + idx); };
						header.array_.lastPtr_ = [&obj]() -> void* { return &obj.materialIDs_.back(); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							outInfo.push_back({ String("[" + std::to_string(i) + "]"), 0, AttributeType::Int, PayloadType::Material, &arr[i] });
						}
					}
				});
			}
		};
		static Register_Material global_Material_register;

		// ---- GraphicsEngine/Model/Skeleton/Skeleton.h ----
		struct Register_Skeleton
		{
			Register_Skeleton()
			{
				PayloadRegistry::Register(String("Skeleton"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Skeleton& obj = *static_cast<Skeleton*>(ptr);
					outInfo.push_back({ String("スケルトン"), offsetof(Skeleton, skeletonID_), AttributeType::Int, PayloadType::Skeleton });
				});
			}
		};
		static Register_Skeleton global_Skeleton_register;

		// ---- GraphicsEngine/Movie/Movie.h ----
		struct Register_Movie
		{
			Register_Movie()
			{
				PayloadRegistry::Register(String("Movie"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Movie& obj = *static_cast<Movie*>(ptr);
					outInfo.push_back({ String("動画ID"), offsetof(Movie, movieID_), AttributeType::Int, PayloadType::Movie });
				});
			}
		};
		static Register_Movie global_Movie_register;

		// ---- GraphicsEngine/Shape/Primitive/BoxShape.h ----
		struct Register_BoxShape
		{
			Register_BoxShape()
			{
				PayloadRegistry::Register(String("BoxShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BoxShape& obj = *static_cast<BoxShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(BoxShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_BoxShape global_BoxShape_register;

		// ---- GraphicsEngine/Shape/Primitive/CapsuleShape.h ----
		struct Register_CapsuleShape
		{
			Register_CapsuleShape()
			{
				PayloadRegistry::Register(String("CapsuleShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CapsuleShape& obj = *static_cast<CapsuleShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(CapsuleShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_CapsuleShape global_CapsuleShape_register;

		// ---- GraphicsEngine/Shape/Primitive/ConeShape.h ----
		struct Register_ConeShape
		{
			Register_ConeShape()
			{
				PayloadRegistry::Register(String("ConeShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ConeShape& obj = *static_cast<ConeShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(ConeShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_ConeShape global_ConeShape_register;

		// ---- GraphicsEngine/Shape/Primitive/CylinderShape.h ----
		struct Register_CylinderShape
		{
			Register_CylinderShape()
			{
				PayloadRegistry::Register(String("CylinderShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CylinderShape& obj = *static_cast<CylinderShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(CylinderShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_CylinderShape global_CylinderShape_register;

		// ---- GraphicsEngine/Shape/Primitive/DiscShape.h ----
		struct Register_DiscShape
		{
			Register_DiscShape()
			{
				PayloadRegistry::Register(String("DiscShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DiscShape& obj = *static_cast<DiscShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(DiscShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_DiscShape global_DiscShape_register;

		// ---- GraphicsEngine/Shape/Primitive/PlaneShape.h ----
		struct Register_PlaneShape
		{
			Register_PlaneShape()
			{
				PayloadRegistry::Register(String("PlaneShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PlaneShape& obj = *static_cast<PlaneShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(PlaneShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_PlaneShape global_PlaneShape_register;

		// ---- GraphicsEngine/Shape/Primitive/RampShape.h ----
		struct Register_RampShape
		{
			Register_RampShape()
			{
				PayloadRegistry::Register(String("RampShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					RampShape& obj = *static_cast<RampShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(RampShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_RampShape global_RampShape_register;

		// ---- GraphicsEngine/Shape/Primitive/SegmentShape.h ----
		struct Register_SegmentShape
		{
			Register_SegmentShape()
			{
				PayloadRegistry::Register(String("SegmentShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SegmentShape& obj = *static_cast<SegmentShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(SegmentShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_SegmentShape global_SegmentShape_register;

		// ---- GraphicsEngine/Shape/Primitive/SphereShape.h ----
		struct Register_SphereShape
		{
			Register_SphereShape()
			{
				PayloadRegistry::Register(String("SphereShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SphereShape& obj = *static_cast<SphereShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(SphereShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_SphereShape global_SphereShape_register;

		// ---- GraphicsEngine/Shape/Primitive/TorusShape.h ----
		struct Register_TorusShape
		{
			Register_TorusShape()
			{
				PayloadRegistry::Register(String("TorusShape"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					TorusShape& obj = *static_cast<TorusShape*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(TorusShape, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_TorusShape global_TorusShape_register;

		// ---- GraphicsEngine/Texture/Image.h ----
		struct Register_Image
		{
			Register_Image()
			{
				PayloadRegistry::Register(String("Image"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					Image& obj = *static_cast<Image*>(ptr);
					outInfo.push_back({ String("テクスチャID"), offsetof(Image, textureID_), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_Image global_Image_register;

		// ---- PhysicsEngine/Collider/MeshCollider.h ----
		struct Register_MeshCollider
		{
			Register_MeshCollider()
			{
				PayloadRegistry::Register(String("MeshCollider"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					MeshCollider& obj = *static_cast<MeshCollider*>(ptr);
					outInfo.push_back({ String("コリジョンメッシュ"), offsetof(MeshCollider, meshID_), AttributeType::Int, PayloadType::MeshCollision });
				});
			}
		};
		static Register_MeshCollider global_MeshCollider_register;

		// ---- PhysicsEngine/Joint/FixedJoint.h ----
		struct Register_FixedJoint
		{
			Register_FixedJoint()
			{
				PayloadRegistry::Register(String("FixedJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FixedJoint& obj = *static_cast<FixedJoint*>(ptr);
					outInfo.push_back({ String("接続先アクター"), offsetof(FixedJoint, connectedActor_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_FixedJoint global_FixedJoint_register;

		// ---- PhysicsEngine/Joint/HingeJoint.h ----
		struct Register_HingeJoint
		{
			Register_HingeJoint()
			{
				PayloadRegistry::Register(String("HingeJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					HingeJoint& obj = *static_cast<HingeJoint*>(ptr);
					outInfo.push_back({ String("接続先アクター"), offsetof(HingeJoint, connectedActor_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_HingeJoint global_HingeJoint_register;

		// ---- PhysicsEngine/Joint/SliderJoint.h ----
		struct Register_SliderJoint
		{
			Register_SliderJoint()
			{
				PayloadRegistry::Register(String("SliderJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SliderJoint& obj = *static_cast<SliderJoint*>(ptr);
					outInfo.push_back({ String("接続先アクター"), offsetof(SliderJoint, connectedActor_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_SliderJoint global_SliderJoint_register;

		// ---- PhysicsEngine/Joint/SpringJoint.h ----
		struct Register_SpringJoint
		{
			Register_SpringJoint()
			{
				PayloadRegistry::Register(String("SpringJoint"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SpringJoint& obj = *static_cast<SpringJoint*>(ptr);
					outInfo.push_back({ String("接続先アクター"), offsetof(SpringJoint, connectedActor_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_SpringJoint global_SpringJoint_register;

	}
}
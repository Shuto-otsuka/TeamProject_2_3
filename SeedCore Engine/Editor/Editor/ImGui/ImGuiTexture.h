#pragma once

namespace SeedCore
{
	struct EditorContext;

	enum class IconType : Uint
	{
		FolderInItem,
		FolderNoItem,

		Model,
		Effect,
		Audio,
		Font,
		Sky,
		Animation,
		MeshCollision,
		Material,
		Skeleton,
		Movie,
		Scene,

		Text,
		Cpp,
		CSharp,
		Header,
		Hlsl,
		Search,

		Play,
		Pause,
		Stop,

		Actor,
		ActorChild,
		Prefab,
		PrefabChild,

		Lock,
		Unlock,

		Add,
		Remove,

		SharedAsset,
		SharedOutdated,
		SharedModified,
		SharedConflict,

		Guizmo,
		NonSelected,
		Translate,
		Rotate,
		Scale,
		Rect,
		Pivot,
		ShowIcon,
		ShowShape,
		Camera,
		ViewMode,

		LogError,
		LogWarning,
		LogNotice,

		NonCameraWarning,

		ActorActive,
		ActorNonActive,

		ViewCamera,
		ViewPointLight,
		ViewDirectionalLight,
		ViewSpotLight,
		ViewRectangleLight,
		ViewSkyLight,
		ViewAudioSource,
		ViewAudioListener,
		ViewActor,

		ComponentTransform,
		ComponentCamera,
		ComponentCameraBrain,
		ComponentPointLight,
		ComponentDirectionalLight,
		ComponentSpotLight,
		ComponentRectangleLight,
		ComponentSkyLight,
		ComponentBoxCollider,
		ComponentSphereCollider,
		ComponentCapsuleCollider,
		ComponentCylinderCollider,
		ComponentRectCollider,
		ComponentCircleCollider,
		ComponentMeshCollider,
		ComponentRigidbody,
		ComponentSoftbody,
		ComponentCharacterController,
		ComponentHingeJoint,
		ComponentFixedJoint,
		ComponentSpringJoint,
		ComponentSliderJoint,
		ComponentAudioSource,
		ComponentAudioListener,
		ComponentImage,
		ComponentText,
		ComponentMovie,
		ComponentMesh,
		ComponentMaterial,
		ComponentSkeleton,
		ComponentAnimator,
		ComponentBoxShape,
		ComponentSphereShape,
		ComponentCapsuleShape,
		ComponentCylinderShape,
		ComponentConeShape,
		ComponentRampShape,
		ComponentDiscShape,
		ComponentTorusShape,
		ComponentPlaneShape,
		ComponentSegmentShape,
		ComponentPositionConstraint,
		ComponentRotationConstraint,
		ComponentLookAtConstraint,
		ComponentParentConstraint,
		ComponentAttachmentConstraint,
		ComponentIKConstraint,
		ComponentWeather,
		ComponentCelestial,
		ComponentVolumetricCloudScapes,
		ComponentVolumetricStar,
		ComponentShadow,
		ComponentGlobalIllumination,
		ComponentAmbientOcclusion,
		ComponentReflection,
		ComponentRefraction,
		ComponentCaustics,
		ComponentSubsurfaceScattering,
		ComponentVolumetricLight,
		ComponentEffect,
		ComponentPostProcess,
		ComponentSpawner,
		ComponentLifetime,
		ComponentCustom,

		Count
	};

	class ImGuiTexture
	{
	public:
		ImGuiTexture(EditorContext& context);
		~ImGuiTexture() = default;

		NODISCARD ImTextureID Icon(IconType type)const;

		NODISCARD static IconType ComponentIconType(const String& componentName);

	private:
		ImTextureID icons_[static_cast<Uint>(IconType::Count)] = {};

		DynamicArray<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
	};
}

#pragma once
#include <FoundationEngine/Prelude.h>

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

		Text,
		Cpp,
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
		Scene,

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
		ComponentPositionConstraint,
		ComponentRotationConstraint,
		ComponentLookAtConstraint,
		ComponentParentConstraint,
		ComponentAttachmentConstraint,
		ComponentIKConstraint,
		ComponentWeather,
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

		[[nodiscard]] ImTextureID Icon(IconType type)const;

		/// [EN] Maps a component's registered name to its Inspector/Add
		///      Component header icon (Unity-style). Falls back to
		///      IconType::ComponentCustom for anything not explicitly
		///      listed — covers UserProject scripts and any built-in
		///      component without a dedicated icon. Static (no instance
		///      state needed) so both InspectorPanel and AddComponentPanel
		///      can share one lookup table instead of each keeping their own.
		/// [JP] コンポーネントの登録名を Inspector/Add Component ヘッダー用
		///      アイコン（Unity 風）へ対応付ける。明示的に列挙されていない
		///      ものは IconType::ComponentCustom にフォールバックする —
		///      UserProject のスクリプトや、専用アイコンを持たない組み込み
		///      コンポーネントをカバーする。static（インスタンス状態不要）
		///      にすることで、InspectorPanel と AddComponentPanel が
		///      それぞれ別の対応表を持たず、1つを共有できる。
		[[nodiscard]] static IconType ComponentIconType(const String& componentName);

	private:
		ImTextureID icons_[static_cast<Uint>(IconType::Count)] = {};

		DynamicArray<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
	};
}

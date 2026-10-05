#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Graphics.h>
#include <FoundationEngine/Resource/LoaderSystem.h>

namespace SeedCore
{
	ImGuiTexture::ImGuiTexture(EditorContext& context)
	{
		D3D12Context& d3d12Context = context.graphics_.graphics_->GetContext();
		BindlessHeap* bindlessHeap = &context.graphics_.graphics_->GetBindlessHeap();

		auto load = [&](IconType type, const Char* folder, const Char* name)
		{
			Uint index = bindlessHeap->AllocateIndex();

			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			String filePath = String(std::string("Icon/") + folder + "/" + name + ".icon");
			TextureLoader::CreateTexturePath(d3d12Context.GetDevice(), d3d12Context.GetDirectQueue(), bindlessHeap->Heap(), filePath, resource, index);

			icons_[static_cast<Uint>(type)] = static_cast<ImTextureID>(bindlessHeap->GPUHandle(index).ptr);
			resources_.push_back(std::move(resource));
		};

		load(IconType::FolderInItem, "Folder", "FolderInItem");
		load(IconType::FolderNoItem, "Folder", "FolderNoItem");

		load(IconType::Model,         "Asset", "Model");
		load(IconType::Effect,        "Asset", "Effect");
		load(IconType::Audio,         "Asset", "Audio");
		load(IconType::Font,          "Asset", "Font");
		load(IconType::Sky,           "Asset", "Sky");
		load(IconType::Animation,     "Asset", "Animation");
		load(IconType::MeshCollision, "Asset", "MeshCollision");
		load(IconType::Material,      "Asset", "Material");
		load(IconType::Skeleton,      "Asset", "Skeleton");
		load(IconType::Movie,         "Asset", "Movie");

		load(IconType::Text,   "Asset", "Text");
		load(IconType::Cpp,    "Asset", "CPlusPlus");
		load(IconType::Header, "Asset", "Header");
		load(IconType::Hlsl,   "Asset", "Hlsl");

		load(IconType::Search, "Misc", "Search");

		load(IconType::Play,  "Toolbar", "Play");
		load(IconType::Pause, "Toolbar", "Pause");
		load(IconType::Stop,  "Toolbar", "Stop");

		load(IconType::Actor,       "Hierarchy", "Actor");
		load(IconType::ActorChild,  "Hierarchy", "ActorChild");
		load(IconType::Prefab,      "Hierarchy", "Prefab");
		load(IconType::PrefabChild, "Hierarchy", "PrefabChild");
		load(IconType::Scene,       "Asset", "Scene");

		load(IconType::Lock,   "Misc", "Lock");
		load(IconType::Unlock, "Misc", "Unlock");

		load(IconType::Add,    "Misc", "Plus");
		load(IconType::Remove, "Misc", "Cross");

		load(IconType::SharedAsset,    "Sharing", "Cloud");
		load(IconType::SharedOutdated, "Sharing", "CloudDownload");
		load(IconType::SharedModified, "Sharing", "CloudUpload");
		load(IconType::SharedConflict, "Sharing", "CloudWarning");

		load(IconType::Guizmo,      "Viewport", "Guizmo");
		load(IconType::NonSelected, "Viewport", "NonSelected");
		load(IconType::Translate,   "Viewport", "Translate");
		load(IconType::Rotate,      "Viewport", "Rotate");
		load(IconType::Scale,       "Viewport", "Scale");
		load(IconType::Rect,        "Viewport", "Rect");
		load(IconType::ShowIcon,    "Viewport", "ShowIcon");
		load(IconType::ShowShape,   "Viewport", "ShowShape");
		load(IconType::Camera,      "Viewport", "Camera");
		load(IconType::ViewMode,    "Viewport", "ViewMode");

		load(IconType::LogError,   "Log", "Error");
		load(IconType::LogWarning, "Log", "Warning");
		load(IconType::LogNotice,  "Log", "Notice");

		load(IconType::NonCameraWarning, "Warning", "NonCamera");

		load(IconType::ActorActive,    "Hierarchy", "ActorActive");
		load(IconType::ActorNonActive, "Hierarchy", "ActorNonActive");

		load(IconType::ViewCamera,           "View", "CameraView");
		load(IconType::ViewPointLight,       "View", "PointLightView");
		load(IconType::ViewDirectionalLight, "View", "DirectionalLightView");
		load(IconType::ViewSpotLight,        "View", "SpotLightView");
		load(IconType::ViewRectangleLight,   "View", "RectangleLightView");
		load(IconType::ViewSkyLight,         "View", "SkyLightView");
		load(IconType::ViewAudioSource,      "View", "AudioSourceView");
		load(IconType::ViewAudioListener,    "View", "AudioListenerView");
		load(IconType::ViewActor,            "View", "ActorView");

		load(IconType::ComponentTransform,            "Component", "Transform");
		load(IconType::ComponentCamera,               "Component", "Camera");
		load(IconType::ComponentCameraBrain,          "Component", "CameraBrain");
		load(IconType::ComponentPointLight,           "Component", "PointLight");
		load(IconType::ComponentDirectionalLight,     "Component", "DirectionalLight");
		load(IconType::ComponentSpotLight,            "Component", "SpotLight");
		load(IconType::ComponentRectangleLight,       "Component", "RectangleLight");
		load(IconType::ComponentSkyLight,             "Component", "SkyLight");
		load(IconType::ComponentBoxCollider,          "Component", "BoxCollider");
		load(IconType::ComponentSphereCollider,       "Component", "SphereCollider");
		load(IconType::ComponentCapsuleCollider,      "Component", "CapsuleCollider");
		load(IconType::ComponentCylinderCollider,     "Component", "CylinderCollider");
		load(IconType::ComponentRectCollider,         "Component", "RectCollider");
		load(IconType::ComponentCircleCollider,       "Component", "CircleCollider");
		load(IconType::ComponentMeshCollider,         "Component", "MeshCollider");
		load(IconType::ComponentRigidbody,            "Component", "Rigidbody");
		load(IconType::ComponentSoftbody,             "Component", "Softbody");
		load(IconType::ComponentCharacterController,  "Component", "CharacterController");
		load(IconType::ComponentHingeJoint,           "Component", "HingeJoint");
		load(IconType::ComponentFixedJoint,           "Component", "FixedJoint");
		load(IconType::ComponentSpringJoint,          "Component", "SpringJoint");
		load(IconType::ComponentSliderJoint,          "Component", "SliderJoint");
		load(IconType::ComponentAudioSource,          "Component", "AudioSource");
		load(IconType::ComponentAudioListener,        "Component", "AudioListener");
		load(IconType::ComponentImage,                "Component", "Image");
		load(IconType::ComponentText,                 "Component", "Text");
		load(IconType::ComponentMovie,                "Component", "Movie");
		load(IconType::ComponentMesh,                 "Component", "Mesh");
		load(IconType::ComponentMaterial,             "Component", "Material");
		load(IconType::ComponentSkeleton,             "Component", "Skeleton");
		load(IconType::ComponentAnimator,             "Component", "Animator");
		load(IconType::ComponentPositionConstraint,   "Component", "PositionConstraint");
		load(IconType::ComponentRotationConstraint,   "Component", "RotationConstraint");
		load(IconType::ComponentLookAtConstraint,     "Component", "LookAtConstraint");
		load(IconType::ComponentParentConstraint,     "Component", "ParentConstraint");
		load(IconType::ComponentAttachmentConstraint, "Component", "AttachmentConstraint");
		load(IconType::ComponentIKConstraint,         "Component", "IKConstraint");
		load(IconType::ComponentWeather,              "Component", "Weather");
		load(IconType::ComponentEffect,               "Component", "Effect");
		load(IconType::ComponentPostProcess,          "Component", "PostProcess");
		load(IconType::ComponentSpawner,              "Component", "Spawner");
		load(IconType::ComponentLifetime,             "Component", "Lifetime");
		load(IconType::ComponentCustom,               "Component", "Custom");
	}

	ImTextureID ImGuiTexture::Icon(IconType type)const
	{
		return icons_[static_cast<Uint>(type)];
	}

	IconType ImGuiTexture::ComponentIconType(const String& componentName)
	{
		static const std::unordered_map<std::string, IconType> table =
		{
			{ "Camera", IconType::ComponentCamera },
			{ "CameraBrain", IconType::ComponentCameraBrain },
			{ "PointLight", IconType::ComponentPointLight },
			{ "DirectionalLight", IconType::ComponentDirectionalLight },
			{ "SpotLight", IconType::ComponentSpotLight },
			{ "RectangleLight", IconType::ComponentRectangleLight },
			{ "SkyLight", IconType::ComponentSkyLight },
			{ "BoxCollider", IconType::ComponentBoxCollider },
			{ "SphereCollider", IconType::ComponentSphereCollider },
			{ "CapsuleCollider", IconType::ComponentCapsuleCollider },
			{ "CylinderCollider", IconType::ComponentCylinderCollider },
			{ "RectCollider", IconType::ComponentRectCollider },
			{ "CircleCollider", IconType::ComponentCircleCollider },
			{ "MeshCollider", IconType::ComponentMeshCollider },
			{ "Rigidbody", IconType::ComponentRigidbody },
			{ "Softbody", IconType::ComponentSoftbody },
			{ "CharacterController", IconType::ComponentCharacterController },
			{ "HingeJoint", IconType::ComponentHingeJoint },
			{ "FixedJoint", IconType::ComponentFixedJoint },
			{ "SpringJoint", IconType::ComponentSpringJoint },
			{ "SliderJoint", IconType::ComponentSliderJoint },
			{ "AudioSource", IconType::ComponentAudioSource },
			{ "AudioListener", IconType::ComponentAudioListener },
			{ "Image", IconType::ComponentImage },
			{ "Text", IconType::ComponentText },
			{ "Movie", IconType::ComponentMovie },
			{ "Mesh", IconType::ComponentMesh },
			{ "Material", IconType::ComponentMaterial },
			{ "Skeleton", IconType::ComponentSkeleton },
			{ "Animator", IconType::ComponentAnimator },
			{ "PositionConstraint", IconType::ComponentPositionConstraint },
			{ "RotationConstraint", IconType::ComponentRotationConstraint },
			{ "LookAtConstraint", IconType::ComponentLookAtConstraint },
			{ "ParentConstraint", IconType::ComponentParentConstraint },
			{ "AttachmentConstraint", IconType::ComponentAttachmentConstraint },
			{ "IKConstraint", IconType::ComponentIKConstraint },
			{ "Weather", IconType::ComponentWeather },
			{ "Effect", IconType::ComponentEffect },
			{ "PostProcess", IconType::ComponentPostProcess },
			{ "Spawner", IconType::ComponentSpawner },
			{ "Lifetime", IconType::ComponentLifetime },
		};

		auto found = table.find(componentName.str());
		if (found != table.end())
		{
			return found->second;
		}

		return IconType::ComponentCustom;
	}
}
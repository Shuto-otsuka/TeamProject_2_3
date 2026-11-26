#include <FoundationEngine/Reflection/ReflectionRegistry.h>

// [REFLECTION_AUTO_BEGIN]
#pragma comment(linker, "/include:_force_reflection_AudioListener")
#pragma comment(linker, "/include:_force_reflection_AudioSource")
#pragma comment(linker, "/include:_force_reflection_Lifetime")
#pragma comment(linker, "/include:_force_reflection_Name")
#pragma comment(linker, "/include:_force_reflection_Position")
#pragma comment(linker, "/include:_force_reflection_Rotation")
#pragma comment(linker, "/include:_force_reflection_Scale")
#pragma comment(linker, "/include:_force_reflection_Spawner")
#pragma comment(linker, "/include:_force_reflection_LinearVelocity")
#pragma comment(linker, "/include:_force_reflection_AngularVelocity")
#pragma comment(linker, "/include:_force_reflection_Camera")
#pragma comment(linker, "/include:_force_reflection_CameraBrain")
#pragma comment(linker, "/include:_force_reflection_AttachmentConstraint")
#pragma comment(linker, "/include:_force_reflection_Effector")
#pragma comment(linker, "/include:_force_reflection_IKConstraint")
#pragma comment(linker, "/include:_force_reflection_LookAtConstraint")
#pragma comment(linker, "/include:_force_reflection_ParentConstraint")
#pragma comment(linker, "/include:_force_reflection_PositionConstraint")
#pragma comment(linker, "/include:_force_reflection_RotationConstraint")
#pragma comment(linker, "/include:_force_reflection_SunSettings")
#pragma comment(linker, "/include:_force_reflection_MoonSettings")
#pragma comment(linker, "/include:_force_reflection_Celestial")
#pragma comment(linker, "/include:_force_reflection_VolumetricCloudShapeSettings")
#pragma comment(linker, "/include:_force_reflection_VolumetricCloudLightingSettings")
#pragma comment(linker, "/include:_force_reflection_VolumetricCloudAdvancedSettings")
#pragma comment(linker, "/include:_force_reflection_VolumetricCloudSkySettings")
#pragma comment(linker, "/include:_force_reflection_VolumetricCloudScapes")
#pragma comment(linker, "/include:_force_reflection_VolumetricStar")
#pragma comment(linker, "/include:_force_reflection_Rain")
#pragma comment(linker, "/include:_force_reflection_Snow")
#pragma comment(linker, "/include:_force_reflection_Weather")
#pragma comment(linker, "/include:_force_reflection_Text")
#pragma comment(linker, "/include:_force_reflection_RaytracingAmbientOcclusionSettings")
#pragma comment(linker, "/include:_force_reflection_GroundTruthAmbientOcclusionSettings")
#pragma comment(linker, "/include:_force_reflection_ScreenSpaceAmbientOcclusionSettings")
#pragma comment(linker, "/include:_force_reflection_AmbientOcclusion")
#pragma comment(linker, "/include:_force_reflection_Caustics")
#pragma comment(linker, "/include:_force_reflection_RaytracingGlobalIlluminationSettings")
#pragma comment(linker, "/include:_force_reflection_DynamicDiffuseGlobalIlluminationSettings")
#pragma comment(linker, "/include:_force_reflection_LightPropagationVolumesSettings")
#pragma comment(linker, "/include:_force_reflection_ScreenSpaceGlobalIlluminationSettings")
#pragma comment(linker, "/include:_force_reflection_GlobalIllumination")
#pragma comment(linker, "/include:_force_reflection_RaytracingReflectionSettings")
#pragma comment(linker, "/include:_force_reflection_SignedDistanceFieldReflectionSettings")
#pragma comment(linker, "/include:_force_reflection_ScreenSpaceReflectionSettings")
#pragma comment(linker, "/include:_force_reflection_Reflection")
#pragma comment(linker, "/include:_force_reflection_RaytracingRefractionSettings")
#pragma comment(linker, "/include:_force_reflection_Refraction")
#pragma comment(linker, "/include:_force_reflection_RaytracingShadowSettings")
#pragma comment(linker, "/include:_force_reflection_VirtualShadowMapSettings")
#pragma comment(linker, "/include:_force_reflection_CascadedShadowMapSettings")
#pragma comment(linker, "/include:_force_reflection_Shadow")
#pragma comment(linker, "/include:_force_reflection_RaytracingSubsurfaceScatteringSettings")
#pragma comment(linker, "/include:_force_reflection_SubsurfaceScattering")
#pragma comment(linker, "/include:_force_reflection_RaytracingVolumetricLightSettings")
#pragma comment(linker, "/include:_force_reflection_VolumetricLight")
#pragma comment(linker, "/include:_force_reflection_DirectionalLight")
#pragma comment(linker, "/include:_force_reflection_PointLight")
#pragma comment(linker, "/include:_force_reflection_RectangleLight")
#pragma comment(linker, "/include:_force_reflection_SkyLight")
#pragma comment(linker, "/include:_force_reflection_SpotLight")
#pragma comment(linker, "/include:_force_reflection_AnimationParameter")
#pragma comment(linker, "/include:_force_reflection_AnimationCondition")
#pragma comment(linker, "/include:_force_reflection_AnimationState")
#pragma comment(linker, "/include:_force_reflection_AnimationTransition")
#pragma comment(linker, "/include:_force_reflection_Animator")
#pragma comment(linker, "/include:_force_reflection_Material")
#pragma comment(linker, "/include:_force_reflection_Movie")
#pragma comment(linker, "/include:_force_reflection_DepthOfFieldSettings")
#pragma comment(linker, "/include:_force_reflection_BokehSettings")
#pragma comment(linker, "/include:_force_reflection_LensDistortionSettings")
#pragma comment(linker, "/include:_force_reflection_ChromaticAberrationSettings")
#pragma comment(linker, "/include:_force_reflection_VignetteSettings")
#pragma comment(linker, "/include:_force_reflection_BloomSettings")
#pragma comment(linker, "/include:_force_reflection_AnamorphicFlareSettings")
#pragma comment(linker, "/include:_force_reflection_LensFlareSettings")
#pragma comment(linker, "/include:_force_reflection_ExposureSettings")
#pragma comment(linker, "/include:_force_reflection_ColorGradingRangeSettings")
#pragma comment(linker, "/include:_force_reflection_ColorGradingSettings")
#pragma comment(linker, "/include:_force_reflection_ToneMappingSettings")
#pragma comment(linker, "/include:_force_reflection_SharpnessSettings")
#pragma comment(linker, "/include:_force_reflection_FilmGrainSettings")
#pragma comment(linker, "/include:_force_reflection_PostProcess")
#pragma comment(linker, "/include:_force_reflection_Outline")
#pragma comment(linker, "/include:_force_reflection_BoxShape")
#pragma comment(linker, "/include:_force_reflection_CapsuleShape")
#pragma comment(linker, "/include:_force_reflection_ConeShape")
#pragma comment(linker, "/include:_force_reflection_CylinderShape")
#pragma comment(linker, "/include:_force_reflection_DiscShape")
#pragma comment(linker, "/include:_force_reflection_PlaneShape")
#pragma comment(linker, "/include:_force_reflection_RampShape")
#pragma comment(linker, "/include:_force_reflection_SegmentShape")
#pragma comment(linker, "/include:_force_reflection_SphereShape")
#pragma comment(linker, "/include:_force_reflection_TorusShape")
#pragma comment(linker, "/include:_force_reflection_Image")
#pragma comment(linker, "/include:_force_reflection_CharacterController")
#pragma comment(linker, "/include:_force_reflection_BoxCollider")
#pragma comment(linker, "/include:_force_reflection_CapsuleCollider")
#pragma comment(linker, "/include:_force_reflection_CircleCollider")
#pragma comment(linker, "/include:_force_reflection_CylinderCollider")
#pragma comment(linker, "/include:_force_reflection_MeshCollider")
#pragma comment(linker, "/include:_force_reflection_RectCollider")
#pragma comment(linker, "/include:_force_reflection_SphereCollider")
#pragma comment(linker, "/include:_force_reflection_FixedJoint")
#pragma comment(linker, "/include:_force_reflection_HingeJoint")
#pragma comment(linker, "/include:_force_reflection_SliderJoint")
#pragma comment(linker, "/include:_force_reflection_SpringJoint")
#pragma comment(linker, "/include:_force_reflection_Rigidbody")
#pragma comment(linker, "/include:_force_reflection_Softbody")
// [REFLECTION_AUTO_END]

namespace SeedCore
{
	FlatMap<String, ReflectionRegistry::ReflectFunc>& ReflectionRegistry::GetRegistry()
	{
		static FlatMap<String, ReflectFunc> instance;
		return instance;
	}

	void ReflectionRegistry::Register(String name, ReflectFunc function)
	{
		GetRegistry()[name] = function;
	}

	FlatMap<String, DynamicArray<EnumEntry>>& EnumRegistry::GetRegistry()
	{
		static FlatMap<String, DynamicArray<EnumEntry>> instance;
		return instance;
	}

	void EnumRegistry::Register(String typeName, DynamicArray<EnumEntry> entries)
	{
		GetRegistry()[typeName] = std::move(entries);
	}

	const DynamicArray<EnumEntry>* EnumRegistry::GetEntries(const String& typeName)
	{
		auto& registry = GetRegistry();
		auto it = registry.find(typeName);
		if (it != registry.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	FlatMap<String, DynamicArray<FunctionInfo>>& FunctionRegistry::GetRegistry()
	{
		static FlatMap<String, DynamicArray<FunctionInfo>> instance;
		return instance;
	}

	void FunctionRegistry::Register(String componentName, FunctionInfo function)
	{
		DynamicArray<FunctionInfo>& functions = GetRegistry()[componentName];

		erase_if(functions, [&function](const FunctionInfo& registered) { return registered.name_ == function.name_; });
		functions.push_back(std::move(function));
	}
}

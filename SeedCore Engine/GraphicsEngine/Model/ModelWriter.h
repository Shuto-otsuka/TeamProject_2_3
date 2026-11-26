#pragma once
#include <GraphicsEngine/Model/ModelLoader.h>

namespace tinygltf
{
	class Model;
}

namespace SeedCore
{
	class Crister;

	enum class ExportAxis
	{
		GltfRightHandedYUp,
		MayaYUp,
		UnrealZUpLeftHanded,
		EngineNativeDirectX,
	};

	enum class ExportPreset
	{
		Gltf,
		Glb,
		FbxMaya,
		FbxUnreal,
		FbxUnity,
		FbxNative,
	};

	struct ExportProfile
	{
		ModelFormat format_ = ModelFormat::Gltf;
		Bool binary_ = false;
		ExportAxis axis_ = ExportAxis::GltfRightHandedYUp;
		Float unitScale_ = 1.0f;
	};

	class SEEDCORE_API ModelWriter
	{
	public:
		NODISCARD static ExportProfile Profile(ExportPreset preset);

		static Bool Write(const Crister& crister, const ExportProfile& profile, String filePath);

		static Bool Write(const tinygltf::Model& model, const ExportProfile& profile, String filePath);
	};
}

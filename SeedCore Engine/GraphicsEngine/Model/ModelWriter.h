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

	/**
	* [EN]
	* Writes models out in glTF/GLB/FBX, converted to a preset's axis
	* system and unit scale. Works on a Crister read for export or on a
	* tinygltf model built in memory; it owns nothing and reads no asset,
	* so ModelExporter (for assets) and the avatar converters (for models
	* they build) both write through it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルを glTF/GLB/FBX で、プリセットの軸系と単位の倍率に変換して
	* 書き出す。書き出し用に読み込んだ Crister か、メモリ上で組み立てた
	* tinygltf のモデルを扱う。何も所有せずアセットも読まないので、
	* ModelExporter（アセット用）とアバターのコンバーター（自分で組み立てた
	* モデル用）の両方がこれを通して書く。
	*/
	class SEEDCORE_API ModelWriter
	{
	public:
		/**
		* [EN]
		* The format, axis system and unit scale a preset stands for.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プリセットが表す形式・軸系・単位の倍率。
		*/
		NODISCARD static ExportProfile Profile(ExportPreset preset);

		/**
		* [EN]
		* Writes crister to filePath in the format, axis system and unit scale
		* of profile, creating the folder when needed. crister must hold
		* uncompressed, LOD-free geometry (as ModelLoader leaves it for
		* export). Returns false when the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* crister を profile の形式・軸系・単位の倍率で filePath へ書き出す。
		* 必要ならフォルダを作る。crister は非圧縮・LOD 無しのジオメトリを
		* 持っている必要がある（ModelLoader が書き出し用に残す形）。書き込みに
		* 失敗した場合は false を返す。
		*/
		static Bool Write(const Crister& crister, const ExportProfile& profile, String filePath);

		/**
		* [EN]
		* Writes a tinygltf model built in memory (such as a baked avatar) to
		* filePath in the format, axis system and unit scale of profile,
		* creating the folder when needed. Returns false when the write
		* failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メモリ上で組み立てた tinygltf のモデル（焼き込んだアバターなど）を、
		* profile の形式・軸系・単位の倍率で filePath へ書き出す。必要なら
		* フォルダを作る。書き込みに失敗した場合は false を返す。
		*/
		static Bool Write(const tinygltf::Model& model, const ExportProfile& profile, String filePath);
	};
}

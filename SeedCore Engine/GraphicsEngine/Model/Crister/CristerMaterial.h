#pragma once

namespace SeedCore
{
	struct KHR
	{
		struct EmissiveStrength
		{
			Float emissiveStrength_ = 1.0f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("emissive_strength", emissiveStrength_);
			}
		};

		EmissiveStrength emissiveStrength_;

		struct Ior
		{
			Float ior_ = 1.5f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("ior", ior_);
			}
		};

		Ior ior_;

		struct Specular
		{
			Float specularFactor_ = 1.0f;

			Float specularColorFactor_[3] = { 1,1,1 };

			Uint32 specularTextureIndex_ = SC_INVALID;

			Uint32 specularColorTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("specular_factor", specularFactor_);
				archive.TryField("specular_color_factor", specularColorFactor_);
				archive.TryField("specular_texture_index", specularTextureIndex_);
				archive.TryField("specular_color_texture_index", specularColorTextureIndex_);
			}
		};

		Specular specular_;

		struct ClearCoat
		{
			Float clearCoatFactor_ = 0.0f;

			Float clearCoatRoughnessFactor_ = 0.0f;

			Uint32 clearCoatTextureIndex_ = SC_INVALID;

			Uint32 clearCoatRoughnessTextureIndex_ = SC_INVALID;

			Uint32 clearCoatNormalTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("clear_coat_factor", clearCoatFactor_);
				archive.TryField("clear_coat_roughness_factor", clearCoatRoughnessFactor_);
				archive.TryField("clear_coat_texture_index", clearCoatTextureIndex_);
				archive.TryField("clear_coat_roughness_texture_index", clearCoatRoughnessTextureIndex_);
				archive.TryField("clear_coat_normal_texture_index", clearCoatNormalTextureIndex_);
			}
		};

		ClearCoat clearCoat_;

		struct Transmission
		{
			Float transmissionFactor_ = 0.0f;

			Uint32 transmissionTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("transmission_factor", transmissionFactor_);
				archive.TryField("transmission_texture_index", transmissionTextureIndex_);
			}
		};

		Transmission transmission_;

		struct Volume
		{
			Float thicknessFactor_ = 0.0f;

			Float attenuationDistance_ = FLT_MAX;

			Float attenuationColor_[3] = { 1,1,1 };

			Uint32 thicknessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("thickness_factor", thicknessFactor_);
				archive.TryField("attenuation_distance", attenuationDistance_);
				archive.TryField("attenuation_color", attenuationColor_);
				archive.TryField("thickness_texture_index", thicknessTextureIndex_);
			}
		};

		Volume volume_;

		struct Sheen
		{
			Float sheenColorFactor_[3] = { 0,0,0 };

			Float sheenRoughnessFactor_ = 0.0f;

			Uint32 sheenColorTextureIndex_ = SC_INVALID;

			Uint32 sheenRoughnessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("sheen_color_factor", sheenColorFactor_);
				archive.TryField("sheen_roughness_factor", sheenRoughnessFactor_);
				archive.TryField("sheen_color_texture_index", sheenColorTextureIndex_);
				archive.TryField("sheen_roughness_texture_index", sheenRoughnessTextureIndex_);
			}
		};

		Sheen sheen_;

		struct Iridescence
		{
			Float iridescenceFactor_ = 0.0f;

			Float iridescenceIor_ = 1.3f;

			Float iridescenceThicknessMinimum_ = 100.0f;

			Float iridescenceThicknessMaximum_ = 400.0f;

			Uint32 iridescenceTextureIndex_ = SC_INVALID;

			Uint32 iridescenceThicknessTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("iridescence_factor", iridescenceFactor_);
				archive.TryField("iridescence_ior", iridescenceIor_);
				archive.TryField("iridescence_thickness_minimum", iridescenceThicknessMinimum_);
				archive.TryField("iridescence_thickness_maximum", iridescenceThicknessMaximum_);
				archive.TryField("iridescence_texture_index", iridescenceTextureIndex_);
				archive.TryField("iridescence_thickness_texture_index", iridescenceThicknessTextureIndex_);
			}
		};

		Iridescence iridescence_;

		struct Anisotropy
		{
			Float anisotropyStrength_ = 0.0f;

			Float anisotropyRotation_ = 0.0f;

			Uint32 anisotropyTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("anisotropy_strength", anisotropyStrength_);
				archive.TryField("anisotropy_rotation", anisotropyRotation_);
				archive.TryField("anisotropy_texture_index", anisotropyTextureIndex_);
			}
		};

		Anisotropy anisotropy_;

		struct Unlit
		{
			Int unlit_ = 0;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("unlit", unlit_);
			}
		};

		Unlit unlit_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("emissive_strength", emissiveStrength_);
			archive.TryField("ior", ior_);
			archive.TryField("specular", specular_);
			archive.TryField("clear_coat", clearCoat_);
			archive.TryField("transmission", transmission_);
			archive.TryField("volume", volume_);
			archive.TryField("sheen", sheen_);
			archive.TryField("iridescence", iridescence_);
			archive.TryField("anisotropy", anisotropy_);
			archive.TryField("unlit", unlit_);
		}
	};

	enum class ShadingModel :Uint32
	{
		Pbr = 0,

		Unlit = 1,

		Phong = 2,

		Toon = 3,

		Lambert = 4,

		Flat = 5,

		Fur = 6,
	};

	enum class MaterialMapping :Uint32
	{
		Stretch = 0,

		Tile = 1,

		TriplanarObject = 2,

		TriplanarWorld = 3,
	};

	struct Surface
	{
		std::string name_;

		Color baseColor_ = { 1,1,1,1 };

		Float metallic_ = 0.0f;

		Float roughness_ = 1.0f;

		Float emissiveFactor_[3] = { 0,0,0 };

		Int alphaMode_ = 0;

		Float alphaCutoff_ = 0.5f;

		Int doubleSided_ = 0;

		ShadingModel shadingModel_ = ShadingModel::Pbr;

		KHR khr_;

		Float furLength_ = 0.03f;

		Float furDensity_ = 1.0f;

		Int furShellCount_ = 16;

		Uint32 baseColorTextureIndex_ = SC_INVALID;

		Uint32 normalTextureIndex_ = SC_INVALID;

		Uint32 metallicRoughnessTextureIndex_ = SC_INVALID;

		Uint32 occlusionTextureIndex_ = SC_INVALID;

		Uint32 emissiveTextureIndex_ = SC_INVALID;

		Uint32 baseColorTextureAssetID_ = 0;

		Uint32 normalTextureAssetID_ = 0;

		Uint32 metallicRoughnessTextureAssetID_ = 0;

		Uint32 occlusionTextureAssetID_ = 0;

		Uint32 emissiveTextureAssetID_ = 0;

		MaterialMapping materialMapping_ = MaterialMapping::Stretch;

		Float tiling_ = 1.0f;

		Float triplanarSharpness_ = 4.0f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("name", name_);
			archive.TryField("base_color", baseColor_);
			archive.TryField("metallic", metallic_);
			archive.TryField("roughness", roughness_);
			archive.TryField("emissive_factor", emissiveFactor_);
			archive.TryField("alpha_mode", alphaMode_);
			archive.TryField("alpha_cutoff", alphaCutoff_);
			archive.TryField("double_sided", doubleSided_);
			archive.TryField("shading_model", shadingModel_);
			archive.TryField("khr", khr_);
			archive.TryField("fur_length", furLength_);
			archive.TryField("fur_density", furDensity_);
			archive.TryField("fur_shell_count", furShellCount_);
			archive.TryField("base_color_texture_index", baseColorTextureIndex_);
			archive.TryField("normal_texture_index", normalTextureIndex_);
			archive.TryField("metallic_roughness_texture_index", metallicRoughnessTextureIndex_);
			archive.TryField("occlusion_texture_index", occlusionTextureIndex_);
			archive.TryField("emissive_texture_index", emissiveTextureIndex_);
			archive.TryField("base_color_texture_asset_id", baseColorTextureAssetID_);
			archive.TryField("normal_texture_asset_id", normalTextureAssetID_);
			archive.TryField("metallic_roughness_texture_asset_id", metallicRoughnessTextureAssetID_);
			archive.TryField("occlusion_texture_asset_id", occlusionTextureAssetID_);
			archive.TryField("emissive_texture_asset_id", emissiveTextureAssetID_);
			archive.TryField("material_mapping", materialMapping_);
			archive.TryField("tiling", tiling_);
			archive.TryField("triplanar_sharpness", triplanarSharpness_);
		}
	};

	struct Bitmap
	{
		std::string name_;

		Int width_ = -1;

		Int height_ = -1;

		Int component_ = -1;

		Int bits_ = -1;

		Int mipCount_ = 1;

		std::string mimeType_;

		DynamicArray<Uchar> cacheData_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("width", width_);
			archive.Field("height", height_);
			archive.Field("component", component_);
			archive.Field("bits", bits_);
			archive.Field("mip_count", mipCount_);
			archive.Field("mime_type", mimeType_);
			archive.Field("cache_data", cacheData_);
		}
	};

	struct CristerMaterial
	{
		DynamicArray<Surface> surfaces_;

		DynamicArray<Bitmap> bitmaps_;
	};
}
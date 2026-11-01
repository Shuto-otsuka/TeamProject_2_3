#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Bundles every glTF KHR_materials_* extension a Material can carry.
	* Each extension's default is set so that "extension absent from the
	* glTF" and "extension present with default values" behave identically
	* (neutral / no effect).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の KHR_materials_* 拡張をまとめて持つ。各拡張のデフォルトは
	* 「拡張が glTF に無い時＝中立（効果なし）」になるよう設定している。
	*/
	struct KHR
	{
		/**
		* [EN]
		* KHR_materials_emissive_strength: emissive intensity multiplier
		* (>1 for HDR emission). Default 1.0 = neutral.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_emissive_strength: エミッシブ強度（>1 で HDR 発光）。既定 1.0＝中立。
		*/
		struct EmissiveStrength
		{
			/// [EN] Multiplier on the emissive color; values above 1 give HDR emission.
			/// [JP] エミッシブ色に掛ける倍率。1 を超えると HDR の発光になる。
			Float emissiveStrength_ = 1.0f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("emissive_strength", emissiveStrength_);
			}
		};

		/// [EN] KHR_materials_emissive_strength.
		/// [JP] KHR_materials_emissive_strength。
		EmissiveStrength emissiveStrength_;

		/**
		* [EN]
		* KHR_materials_ior: dielectric index of refraction. Default 1.5
		* (glTF's own default). Feeds the F0 (Fresnel reflectance at normal
		* incidence) computation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_ior: 誘電体の屈折率。既定 1.5（glTF 既定）。F0 計算に効く。
		*/
		struct Ior
		{
			/// [EN] Index of refraction of the dielectric; F0 is derived from it.
			/// [JP] 誘電体の屈折率。F0 はこれから求める。
			Float ior_ = 1.5f;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("ior", ior_);
			}
		};

		/// [EN] KHR_materials_ior.
		/// [JP] KHR_materials_ior。
		Ior ior_;

		/**
		* [EN]
		* KHR_materials_specular: specular intensity and tint. Default
		* factor=1 / color=white = full specular (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_specular: スペキュラ強度と色。既定 factor=1/color=白＝フルスペキュラ（中立）。
		*/
		struct Specular
		{
			/// [EN] Strength of the specular reflection.
			/// [JP] スペキュラ反射の強さ。
			Float specularFactor_ = 1.0f;

			/// [EN] Linear RGB tint of the specular reflection at normal incidence.
			/// [JP] 垂直入射でのスペキュラ反射の色味(リニア RGB)。
			Float specularColorFactor_[3] = { 1,1,1 };

			/// [EN] Image in bitmaps_ whose alpha scales specularFactor_; SC_INVALID when absent.
			/// [JP] アルファで specularFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 specularTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose RGB scales specularColorFactor_; SC_INVALID when absent.
			/// [JP] RGB で specularColorFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
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

		/// [EN] KHR_materials_specular.
		/// [JP] KHR_materials_specular。
		Specular specular_;

		/**
		* [EN]
		* KHR_materials_clearcoat: an extra clear-coat lobe. Default
		* factor=0 = no coat (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_clearcoat: クリアコート層。既定 factor=0＝コート無し（中立）。
		*/
		struct ClearCoat
		{
			/// [EN] Strength of the clear-coat layer; 0 means no coat.
			/// [JP] クリアコート層の強さ。0 ならコート無し。
			Float clearCoatFactor_ = 0.0f;

			/// [EN] Roughness of the clear-coat layer.
			/// [JP] クリアコート層のラフネス。
			Float clearCoatRoughnessFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose red channel scales clearCoatFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで clearCoatFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 clearCoatTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose green channel scales clearCoatRoughnessFactor_; SC_INVALID when absent.
			/// [JP] G チャンネルで clearCoatRoughnessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 clearCoatRoughnessTextureIndex_ = SC_INVALID;

			/// [EN] Normal map of the clear-coat layer in bitmaps_; SC_INVALID when absent.
			/// [JP] クリアコート層の法線マップ(bitmaps_ の画像)。無ければ SC_INVALID。
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

		/// [EN] KHR_materials_clearcoat.
		/// [JP] KHR_materials_clearcoat。
		ClearCoat clearCoat_;

		/**
		* [EN]
		* KHR_materials_transmission: thin-glass transmission. Default
		* factor=0 = opaque (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_transmission: 透過（薄いガラス）。既定 factor=0＝不透過（中立）。
		*/
		struct Transmission
		{
			/// [EN] Fraction of light transmitted through the surface; 0 is opaque.
			/// [JP] 表面を透過する光の割合。0 なら不透過。
			Float transmissionFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose red channel scales transmissionFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで transmissionFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 transmissionTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("transmission_factor", transmissionFactor_);
				archive.TryField("transmission_texture_index", transmissionTextureIndex_);
			}
		};

		/// [EN] KHR_materials_transmission.
		/// [JP] KHR_materials_transmission。
		Transmission transmission_;

		/**
		* [EN]
		* KHR_materials_volume: internal volume (thickness + absorption),
		* used together with Transmission. Default thickness=0 /
		* attenuationDistance=infinity (no absorption) / color=white =
		* neutral.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_volume: 内部ボリューム（厚み・吸収）。transmission と併用。
		* 既定 thickness=0/attenuationDistance=∞（吸収なし）/color=白＝中立。
		*/
		struct Volume
		{
			/// [EN] Thickness of the volume under the surface, in model units; 0 makes it thin-walled.
			/// [JP] 表面の下にあるボリュームの厚み(モデル空間の単位)。0 なら薄い壁として扱う。
			Float thicknessFactor_ = 0.0f;

			/// [EN] Distance light travels in the medium before reaching attenuationColor_; FLT_MAX means no absorption.
			/// [JP] 媒質内で光が attenuationColor_ になるまでに進む距離。FLT_MAX なら吸収しない。
			Float attenuationDistance_ = FLT_MAX;

			/// [EN] Linear RGB color white light becomes after attenuationDistance_ in the medium.
			/// [JP] 白色光が媒質内を attenuationDistance_ 進んだ後になる色(リニア RGB)。
			Float attenuationColor_[3] = { 1,1,1 };

			/// [EN] Image in bitmaps_ whose green channel scales thicknessFactor_; SC_INVALID when absent.
			/// [JP] G チャンネルで thicknessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
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

		/// [EN] KHR_materials_volume.
		/// [JP] KHR_materials_volume。
		Volume volume_;

		/**
		* [EN]
		* KHR_materials_sheen: fabric-like sheen lobe. Default color=black =
		* no sheen (neutral).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_sheen: 布の光沢。既定 color=黒＝シーン無し（中立）。
		*/
		struct Sheen
		{
			/// [EN] Linear RGB color of the sheen; black means no sheen.
			/// [JP] シーンの色(リニア RGB)。黒ならシーン無し。
			Float sheenColorFactor_[3] = { 0,0,0 };

			/// [EN] Roughness of the sheen lobe.
			/// [JP] シーンのローブのラフネス。
			Float sheenRoughnessFactor_ = 0.0f;

			/// [EN] Image in bitmaps_ whose RGB scales sheenColorFactor_; SC_INVALID when absent.
			/// [JP] RGB で sheenColorFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 sheenColorTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose alpha scales sheenRoughnessFactor_; SC_INVALID when absent.
			/// [JP] アルファで sheenRoughnessFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
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

		/// [EN] KHR_materials_sheen.
		/// [JP] KHR_materials_sheen。
		Sheen sheen_;

		/**
		* [EN]
		* KHR_materials_iridescence: thin-film iridescence (soap bubble /
		* oil slick). Default factor=0 = none (neutral). Thickness is in
		* nanometres (default min=100/max=400), ior default 1.3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_iridescence: 虹色（シャボン玉・油膜）。既定 factor=0＝無し（中立）。
		* 厚みは nm 単位（既定 min=100/max=400）、ior 既定 1.3。
		*/
		struct Iridescence
		{
			/// [EN] Strength of the thin-film iridescence; 0 means none.
			/// [JP] 薄膜干渉による虹色の強さ。0 なら無し。
			Float iridescenceFactor_ = 0.0f;

			/// [EN] Index of refraction of the thin film.
			/// [JP] 薄膜の屈折率。
			Float iridescenceIor_ = 1.3f;

			/// [EN] Film thickness in nanometres where the thickness texture reads 0.
			/// [JP] 厚みテクスチャが 0 の位置での膜厚(nm)。
			Float iridescenceThicknessMinimum_ = 100.0f;

			/// [EN] Film thickness in nanometres where the thickness texture reads 1, or everywhere when there is no texture.
			/// [JP] 厚みテクスチャが 1 の位置での膜厚(nm)。テクスチャが無ければ全体がこの厚み。
			Float iridescenceThicknessMaximum_ = 400.0f;

			/// [EN] Image in bitmaps_ whose red channel scales iridescenceFactor_; SC_INVALID when absent.
			/// [JP] R チャンネルで iridescenceFactor_ を掛ける、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 iridescenceTextureIndex_ = SC_INVALID;

			/// [EN] Image in bitmaps_ whose green channel blends between the minimum and maximum thickness; SC_INVALID when absent.
			/// [JP] G チャンネルで最小と最大の膜厚の間を補間する、bitmaps_ の画像。無ければ SC_INVALID。
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

		/// [EN] KHR_materials_iridescence.
		/// [JP] KHR_materials_iridescence。
		Iridescence iridescence_;

		/**
		* [EN]
		* KHR_materials_anisotropy: anisotropic specular (brushed metal,
		* hair). Default strength=0 = isotropic (neutral). rotation is in
		* radians.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_anisotropy: 異方性スペキュラ（ヘアライン金属・髪）。
		* 既定 strength=0＝等方（中立）。rotation はラジアン。
		*/
		struct Anisotropy
		{
			/// [EN] Strength of the anisotropy; 0 is isotropic.
			/// [JP] 異方性の強さ。0 なら等方。
			Float anisotropyStrength_ = 0.0f;

			/// [EN] Rotation of the anisotropy direction from the tangent, in radians.
			/// [JP] 接線から測った異方性の向きの回転(ラジアン)。
			Float anisotropyRotation_ = 0.0f;

			/// [EN] Image in bitmaps_ holding the direction (red/green) and strength (blue) of the anisotropy; SC_INVALID when absent.
			/// [JP] 異方性の向き(R/G)と強さ(B)を持つ、bitmaps_ の画像。無ければ SC_INVALID。
			Uint32 anisotropyTextureIndex_ = SC_INVALID;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("anisotropy_strength", anisotropyStrength_);
				archive.TryField("anisotropy_rotation", anisotropyRotation_);
				archive.TryField("anisotropy_texture_index", anisotropyTextureIndex_);
			}
		};

		/// [EN] KHR_materials_anisotropy.
		/// [JP] KHR_materials_anisotropy。
		Anisotropy anisotropy_;

		/**
		* [EN]
		* KHR_materials_unlit: disables lighting (flat shading). 1 when the
		* extension is present, default 0 = normal lighting.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_materials_unlit: ライティング無効（フラット）。存在で 1、既定 0＝通常ライティング。
		*/
		struct Unlit
		{
			/// [EN] 1 when the material is unlit, 0 for normal lighting.
			/// [JP] ライティング無しのマテリアルなら 1、通常のライティングなら 0。
			Int unlit_ = 0;

			template<class Archive>
			void Serialize(Archive& archive)
			{
				archive.TryField("unlit", unlit_);
			}
		};

		/// [EN] KHR_materials_unlit.
		/// [JP] KHR_materials_unlit。
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

	/**
	* [EN]
	* Which lighting response DeferredLightingPS.hlsl evaluates a material's
	* direct/indirect light with. Each model is not a separate shader or
	* PSO: the deferred lighting pass reads this per instance and switches
	* within one shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DeferredLightingPS.hlsl がマテリアルの直接光/間接光をどのライティング
	* 応答で評価するか。モデルごとに別のシェーダーや PSO があるのではなく、
	* デフォードライティングパスがインスタンスごとにこれを読み、1つの
	* シェーダーの中で切り替える。
	*/
	enum class ShadingModel :Uint32
	{
		/// [EN] Physically based metallic-roughness shading.
		/// [JP] 物理ベースの metallic-roughness シェーディング。
		Pbr = 0,

		/// [EN] No lighting; the base color is shown as is.
		/// [JP] ライティング無し。ベースカラーをそのまま表示する。
		Unlit = 1,

		/// [EN] Blinn-Phong diffuse and specular.
		/// [JP] Blinn-Phong の拡散とスペキュラ。
		Phong = 2,

		/// [EN] Banded cel shading.
		/// [JP] 段階的に塗り分けるセルシェーディング。
		Toon = 3,

		/// [EN] Lambert diffuse only.
		/// [JP] Lambert の拡散のみ。
		Lambert = 4,

		/// [EN] Faceted shading from the face normal.
		/// [JP] 面法線による面ごとのシェーディング。
		Flat = 5,

		/// [EN] Fur drawn as stacked shells (see Surface::furLength_ and its siblings).
		/// [JP] 重ねたシェルで描く毛(Surface::furLength_ などを参照)。
		Fur = 6,
	};

	/**
	* [EN]
	* glTF PBR metallic-roughness material: base factors, alpha mode/
	* cutoff/double-sidedness, bundled KHR_materials_* extensions (khr_),
	* and bindless-heap-agnostic texture indices (resolved into actual
	* bindless indices at Upload() time via TextureBindlessIndex).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF の PBR metallic-roughness マテリアル: 基本ファクタ、アルファ
	* モード/カットオフ/両面描画フラグ、まとめた KHR_materials_* 拡張
	* (khr_)、そして bindless ヒープに依存しないテクスチャインデックス
	* （Upload() 時に TextureBindlessIndex 経由で実際の bindless
	* インデックスへ解決される）。
	*/
	struct Surface
	{
		/// [EN] Material name from the source file.
		/// [JP] 元ファイルでのマテリアル名。
		std::string name_;

		/// [EN] Linear RGBA base color factor, multiplied with the base color texture.
		/// [JP] ベースカラーの係数(リニア RGBA)。ベースカラーテクスチャに掛ける。
		Color baseColor_ = { 1,1,1,1 };

		/// [EN] Metalness factor, multiplied with the texture's blue channel.
		/// [JP] メタリックの係数。テクスチャの B チャンネルに掛ける。
		Float metallic_ = 0.0f;

		/// [EN] Perceptual roughness factor, multiplied with the texture's green channel.
		/// [JP] 知覚的ラフネスの係数。テクスチャの G チャンネルに掛ける。
		Float roughness_ = 1.0f;

		/// [EN] Linear RGB emissive factor, multiplied with the emissive texture and khr_.emissiveStrength_.
		/// [JP] エミッシブの係数(リニア RGB)。エミッシブテクスチャと khr_.emissiveStrength_ に掛ける。
		Float emissiveFactor_[3] = { 0,0,0 };

		/// [EN] glTF alpha mode: 0 opaque, 1 mask (cut at alphaCutoff_), 2 blend.
		/// [JP] glTF のアルファモード。0 が不透明、1 がマスク(alphaCutoff_ で切り抜く)、2 がブレンド。
		Int alphaMode_ = 0;

		/// [EN] Alpha below which a pixel is discarded in mask mode.
		/// [JP] マスクモードで、これ未満のアルファのピクセルを捨てる。
		Float alphaCutoff_ = 0.5f;

		/// [EN] Nonzero to draw both faces without back-face culling.
		/// [JP] 0 以外なら裏面カリングせず両面を描く。
		Int doubleSided_ = 0;

		/// [EN] Lighting response the deferred lighting pass uses for this material.
		/// [JP] デフォードライティングパスがこのマテリアルに使うライティング応答。
		ShadingModel shadingModel_ = ShadingModel::Pbr;

		/// [EN] KHR_materials_* extensions of this material.
		/// [JP] このマテリアルの KHR_materials_* 拡張。
		KHR khr_;

		/// [EN] Length of the fur, the distance between the base surface and the outermost shell, in model units.
		/// [JP] 毛の長さ。元の表面から最も外側のシェルまでの距離(モデル空間の単位)。
		Float furLength_ = 0.03f;

		/// [EN] Density of the fur strands.
		/// [JP] 毛の密度。
		Float furDensity_ = 1.0f;

		/// [EN] Number of shells the fur is drawn with; more shells give smoother strands at a higher cost.
		/// [JP] 毛を描くシェルの数。多いほど毛がなめらかになるが、負荷も上がる。
		Int furShellCount_ = 16;

		/// [EN] Base color image in bitmaps_; SC_INVALID when absent.
		/// [JP] ベースカラーの画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 baseColorTextureIndex_ = SC_INVALID;

		/// [EN] Tangent-space normal map in bitmaps_; SC_INVALID when absent.
		/// [JP] 接線空間の法線マップ(bitmaps_)。無ければ SC_INVALID。
		Uint32 normalTextureIndex_ = SC_INVALID;

		/// [EN] Metallic (blue) and roughness (green) image in bitmaps_; SC_INVALID when absent.
		/// [JP] メタリック(B)とラフネス(G)の画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 metallicRoughnessTextureIndex_ = SC_INVALID;

		/// [EN] Ambient occlusion image (red) in bitmaps_; SC_INVALID when absent.
		/// [JP] アンビエントオクルージョンの画像(R、bitmaps_)。無ければ SC_INVALID。
		Uint32 occlusionTextureIndex_ = SC_INVALID;

		/// [EN] Emissive color image in bitmaps_; SC_INVALID when absent.
		/// [JP] エミッシブ色の画像(bitmaps_)。無ければ SC_INVALID。
		Uint32 emissiveTextureIndex_ = SC_INVALID;

		/// [EN] Texture asset that replaces baseColorTextureIndex_ when nonzero.
		/// [JP] 0 以外なら baseColorTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 baseColorTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces normalTextureIndex_ when nonzero.
		/// [JP] 0 以外なら normalTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 normalTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces metallicRoughnessTextureIndex_ when nonzero.
		/// [JP] 0 以外なら metallicRoughnessTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 metallicRoughnessTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces occlusionTextureIndex_ when nonzero.
		/// [JP] 0 以外なら occlusionTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 occlusionTextureAssetID_ = 0;

		/// [EN] Texture asset that replaces emissiveTextureIndex_ when nonzero.
		/// [JP] 0 以外なら emissiveTextureIndex_ の代わりに使うテクスチャアセット。
		Uint32 emissiveTextureAssetID_ = 0;

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
		}
	};

	/**
	* [EN]
	* width_/height_/mipCount_ describe cacheData_ as baked by
	* CristerCompressor::BakeBitmap(): BC7-compressed blocks for mip 0 through
	* mipCount_-1, concatenated in mip order (standard BC block
	* layout — 16 bytes per 4x4 texel block, no explicit per-mip
	* offsets stored; Upload() recomputes them from width_/height_).
	* component_/bits_ describe the original glTF source image only.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* width_/height_/mipCount_ は CristerCompressor::BakeBitmap() が焼いた
	* cacheData_ の内容を表す: mip 0 から mipCount_-1 までの BC7 圧縮
	* ブロックをミップ順に連結したもの（標準的な BC ブロックレイアウト
	* — 4x4 テクセルブロックあたり 16 バイト、ミップごとのオフセットは
	* 持たず Upload() が width_/height_ から再計算する）。
	* component_/bits_ は元の glTF ソース画像の情報のみ。
	*/
	struct Bitmap
	{
		/// [EN] Image name from the source file.
		/// [JP] 元ファイルでの画像名。
		std::string name_;

		/// [EN] Width of mip 0 in cacheData_, in texels.
		/// [JP] cacheData_ のミップ 0 の幅(テクセル)。
		Int width_ = -1;

		/// [EN] Height of mip 0 in cacheData_, in texels.
		/// [JP] cacheData_ のミップ 0 の高さ(テクセル)。
		Int height_ = -1;

		/// [EN] Channel count of the source image.
		/// [JP] 元画像のチャンネル数。
		Int component_ = -1;

		/// [EN] Bits per channel of the source image.
		/// [JP] 元画像のチャンネルあたりのビット数。
		Int bits_ = -1;

		/// [EN] Number of mip levels in cacheData_.
		/// [JP] cacheData_ に入っているミップの段数。
		Int mipCount_ = 1;

		/// [EN] MIME type of the source image, such as image/png.
		/// [JP] 元画像の MIME タイプ(image/png など)。
		std::string mimeType_;

		/// [EN] BC7 blocks of every mip, mip 0 first.
		/// [JP] 全ミップの BC7 ブロック。ミップ 0 から順に並ぶ。
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

	/**
	* [EN]
	* Materials of a Crister: the Surfaces its SubMeshes refer to, and
	* the embedded Bitmaps the Surfaces' texture indices refer to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister のマテリアル。SubMesh が参照する Surface と、Surface の
	* テクスチャインデックスが参照する内蔵 Bitmap を持つ。
	*/
	struct CristerMaterial
	{
		/// [EN] Materials, referred to by SubMesh::surfaceIndex_.
		/// [JP] マテリアル。SubMesh::surfaceIndex_ から参照される。
		DynamicArray<Surface> surfaces_;

		/// [EN] Embedded images, referred to by the texture indices in Surface.
		/// [JP] 内蔵画像。Surface のテクスチャインデックスから参照される。
		DynamicArray<Bitmap> bitmaps_;
	};
}
#pragma once
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	enum class EffectEmitterKind :Uint8
	{
		Zephyr,
		Effekseer,
		Houdini,
	};

	struct EffectParameter
	{
		String name_;

		Vector4 value_ = Vector4::Zero;

		Uint32 asset_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("value", value_);
			archive.Field("asset", asset_);
		}
	};

	struct EffectNode
	{
		Uint32 id_ = 0;

		String type_;

		Vector2 position_ = Vector2::Zero;

		DynamicArray<EffectParameter> parameters_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("id", id_);
			archive.Field("type", type_);
			archive.Field("position", position_);
			archive.Field("parameters", parameters_);
		}

		NODISCARD SEEDCORE_API Vector4 Value(const String& input)const;
	};

	struct EffectLink
	{
		Uint32 id_ = 0;

		Uint32 fromNode_ = 0;

		String fromPin_;

		Uint32 toNode_ = 0;

		String toPin_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("id", id_);
			archive.Field("from_node", fromNode_);
			archive.Field("from_pin", fromPin_);
			archive.Field("to_node", toNode_);
			archive.Field("to_pin", toPin_);
		}
	};

	struct EffectEmitter
	{
		String name_;

		EffectEmitterKind kind_ = EffectEmitterKind::Zephyr;

		Bool enabled_ = true;

		DynamicArray<EffectNode> nodes_;

		DynamicArray<EffectLink> links_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("kind", kind_);
			archive.Field("enabled", enabled_);
			archive.Field("nodes", nodes_);
			archive.Field("links", links_);
		}

		NODISCARD SEEDCORE_API static EffectEmitter Create(const String& name);

		NODISCARD SEEDCORE_API const EffectNode* Find(const String& type)const;
	};

	struct EffectAsset
	{
		Uint32 version_ = 1;

		DynamicArray<EffectEmitter> emitters_;

		DynamicArray<Uint32> textureAssetIDs_;

		DynamicArray<Bitmap> textures_;

		DynamicArray<Uint32> modelAssetIDs_;

		DynamicArray<Crister> models_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("version", version_);
			archive.Field("emitters", emitters_);
			archive.TryField("baked_texture_asset_ids", textureAssetIDs_);
			archive.TryField("baked_textures", textures_);
			archive.TryField("baked_model_asset_ids", modelAssetIDs_);
			archive.TryField("baked_models", models_);
		}
	};
}
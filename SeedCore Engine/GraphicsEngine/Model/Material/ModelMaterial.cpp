#include <GraphicsEngine/Model/Material/ModelMaterial.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Texture/TextureStreaming.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	ModelMaterial::ModelMaterial()
	{
		/// No Code
	}

	ModelMaterial::~ModelMaterial() = default;

	void ModelMaterial::Begin(Uint64 frame)
	{
		frame_ = frame;
		updates_.clear();
	}

	Uint32 ModelMaterial::Write(Uint32 slot, Bool reset, std::span<const Surface> surfaces, Uint32 surfaceCount, Crister& crister, TextureStreaming& textureStreaming)
	{
		if (slots_.size() <= slot)
		{
			slots_.resize(static_cast<Size>(slot) + 1);
		}
		if (reset || slots_[slot].count_ != surfaceCount)
		{
			Release(slot);
		}

		Slot& entry = slots_[slot];
		Bool allocated = false;
		if (entry.offset_ == SC_INVALID)
		{
			for (Size rangeIndex = 0;rangeIndex < freeRanges_.size();++rangeIndex)
			{
				if (freeRanges_[rangeIndex].second < surfaceCount)
				{
					continue;
				}
				entry.offset_ = freeRanges_[rangeIndex].first;
				freeRanges_[rangeIndex].first += surfaceCount;
				freeRanges_[rangeIndex].second -= surfaceCount;
				if (freeRanges_[rangeIndex].second == 0)
				{
					freeRanges_.erase(freeRanges_.begin() + rangeIndex);
				}
				break;
			}
			if (entry.offset_ == SC_INVALID && used_ + surfaceCount <= capacity_)
			{
				entry.offset_ = used_;
				used_ += surfaceCount;
			}
			if (entry.offset_ == SC_INVALID)
			{
				if (!capacityLogged_)
				{
					SC_LOG_ERROR("ModelMaterial: マテリアルの表の上限（{}）を超えたので、このアクターは描きません", capacity_);
					capacityLogged_ = true;
				}
				return SC_INVALID;
			}
			entry.count_ = surfaceCount;
			entry.materials_.assign(surfaceCount, ModelMaterialStructuredBuffer{});
			allocated = true;
		}

		const Surface emptySurface{};
		for (Uint32 surfaceIndex = 0;surfaceIndex < surfaceCount;++surfaceIndex)
		{
			ModelMaterialStructuredBuffer material = Build(surfaceIndex < surfaces.size() ? surfaces[surfaceIndex] : emptySurface, crister, textureStreaming, frame_);
			if (!allocated && std::memcmp(&material, &entry.materials_[surfaceIndex], sizeof(ModelMaterialStructuredBuffer)) == 0)
			{
				continue;
			}
			entry.materials_[surfaceIndex] = material;
			ModelMaterialUpdate update;
			update.index_ = entry.offset_ + surfaceIndex;
			update.data_ = material;
			updates_.push_back(update);
		}
		return entry.offset_;
	}

	void ModelMaterial::Release(Uint32 slot)
	{
		if (slots_.size() <= slot || slots_[slot].offset_ == SC_INVALID)
		{
			return;
		}
		freeRanges_.push_back({ slots_[slot].offset_, slots_[slot].count_ });
		slots_[slot] = Slot{};
	}

	std::span<const ModelMaterialStructuredBuffer> ModelMaterial::Materials(Uint32 slot)const
	{
		if (slots_.size() <= slot)
		{
			return {};
		}
		return slots_[slot].materials_;
	}

	std::span<const ModelMaterialUpdate> ModelMaterial::Updates()const
	{
		return updates_;
	}

	ModelMaterialStructuredBuffer ModelMaterial::Build(const Surface& surface, Crister& crister, TextureStreaming& textureStreaming, Uint64 frame)
	{
		CristerResidency& residency = crister.Residency();
		for (Uint32 textureIndex : { surface.baseColorTextureAssetID_ != 0 ? SC_INVALID : surface.baseColorTextureIndex_, surface.normalTextureAssetID_ != 0 ? SC_INVALID : surface.normalTextureIndex_, surface.metallicRoughnessTextureAssetID_ != 0 ? SC_INVALID : surface.metallicRoughnessTextureIndex_, surface.emissiveTextureAssetID_ != 0 ? SC_INVALID : surface.emissiveTextureIndex_, surface.occlusionTextureAssetID_ != 0 ? SC_INVALID : surface.occlusionTextureIndex_, surface.khr_.specular_.specularTextureIndex_, surface.khr_.specular_.specularColorTextureIndex_, surface.khr_.clearCoat_.clearCoatTextureIndex_, surface.khr_.clearCoat_.clearCoatRoughnessTextureIndex_, surface.khr_.clearCoat_.clearCoatNormalTextureIndex_, surface.khr_.transmission_.transmissionTextureIndex_, surface.khr_.volume_.thicknessTextureIndex_, surface.khr_.sheen_.sheenColorTextureIndex_, surface.khr_.sheen_.sheenRoughnessTextureIndex_, surface.khr_.iridescence_.iridescenceTextureIndex_, surface.khr_.iridescence_.iridescenceThicknessTextureIndex_, surface.khr_.anisotropy_.anisotropyTextureIndex_ })
		{
			if (textureIndex != SC_INVALID)
			{
				residency.TouchTexture(textureIndex, frame);
			}
		}

		ModelMaterialStructuredBuffer material{};
		material.texture_.baseColor_ = surface.baseColor_;
		material.texture_.metallic_ = surface.metallic_;
		material.texture_.roughness_ = surface.roughness_;
		material.texture_.alphaCutoff_ = surface.alphaMode_ == 1 ? surface.alphaCutoff_ : (surface.alphaMode_ == 2 ? 0.01f : 0.0f);
		material.texture_.emissive_ = Vector3(surface.emissiveFactor_[0], surface.emissiveFactor_[1], surface.emissiveFactor_[2]);
		material.texture_.ior_ = surface.khr_.ior_.ior_;
		material.texture_.emissiveStrength_ = surface.khr_.emissiveStrength_.emissiveStrength_;
		material.texture_.baseColorTextureIndex_ = surface.baseColorTextureAssetID_ != 0 ? textureStreaming.GetID(surface.baseColorTextureAssetID_, frame) : residency.TextureBindlessIndex(surface.baseColorTextureIndex_);
		material.texture_.normalTextureIndex_ = surface.normalTextureAssetID_ != 0 ? textureStreaming.GetID(surface.normalTextureAssetID_, frame) : residency.TextureBindlessIndex(surface.normalTextureIndex_);
		material.texture_.metallicRoughnessTextureIndex_ = surface.metallicRoughnessTextureAssetID_ != 0 ? textureStreaming.GetID(surface.metallicRoughnessTextureAssetID_, frame) : residency.TextureBindlessIndex(surface.metallicRoughnessTextureIndex_);
		material.texture_.emissiveTextureIndex_ = surface.emissiveTextureAssetID_ != 0 ? textureStreaming.GetID(surface.emissiveTextureAssetID_, frame) : residency.TextureBindlessIndex(surface.emissiveTextureIndex_);
		material.texture_.occlusionTextureIndex_ = surface.occlusionTextureAssetID_ != 0 ? textureStreaming.GetID(surface.occlusionTextureAssetID_, frame) : residency.TextureBindlessIndex(surface.occlusionTextureIndex_);

		material.extension_.specularFactor_ = surface.khr_.specular_.specularFactor_;
		material.extension_.specularColor_ = Vector3(surface.khr_.specular_.specularColorFactor_[0], surface.khr_.specular_.specularColorFactor_[1], surface.khr_.specular_.specularColorFactor_[2]);
		material.extension_.clearCoatFactor_ = surface.khr_.clearCoat_.clearCoatFactor_;
		material.extension_.clearCoatRoughness_ = surface.khr_.clearCoat_.clearCoatRoughnessFactor_;
		material.extension_.anisotropy_ = surface.khr_.anisotropy_.anisotropyStrength_;
		material.extension_.anisotropyRotation_ = surface.khr_.anisotropy_.anisotropyRotation_;
		material.extension_.transmissionFactor_ = surface.khr_.transmission_.transmissionFactor_;
		material.extension_.volumeThicknessFactor_ = surface.khr_.volume_.thicknessFactor_;
		material.extension_.volumeAttenuationDistance_ = surface.khr_.volume_.attenuationDistance_;
		material.extension_.volumeAttenuationColor_ = Vector3(surface.khr_.volume_.attenuationColor_[0], surface.khr_.volume_.attenuationColor_[1], surface.khr_.volume_.attenuationColor_[2]);
		material.extension_.sheenColor_ = Vector3(surface.khr_.sheen_.sheenColorFactor_[0], surface.khr_.sheen_.sheenColorFactor_[1], surface.khr_.sheen_.sheenColorFactor_[2]);
		material.extension_.sheenRoughness_ = surface.khr_.sheen_.sheenRoughnessFactor_;
		material.extension_.iridescenceFactor_ = surface.khr_.iridescence_.iridescenceFactor_;
		material.extension_.iridescenceIor_ = surface.khr_.iridescence_.iridescenceIor_;
		material.extension_.iridescenceThickness_ = (surface.khr_.iridescence_.iridescenceThicknessMinimum_ + surface.khr_.iridescence_.iridescenceThicknessMaximum_) * 0.5f;
		material.extension_.unlit_ = surface.khr_.unlit_.unlit_ != 0 ? 1.0f : 0.0f;
		material.extension_.specularTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.specular_.specularTextureIndex_);
		material.extension_.specularColorTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.specular_.specularColorTextureIndex_);
		material.extension_.clearCoatTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.clearCoat_.clearCoatTextureIndex_);
		material.extension_.clearCoatRoughnessTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.clearCoat_.clearCoatRoughnessTextureIndex_);
		material.extension_.clearCoatNormalTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.clearCoat_.clearCoatNormalTextureIndex_);
		material.extension_.transmissionTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.transmission_.transmissionTextureIndex_);
		material.extension_.thicknessTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.volume_.thicknessTextureIndex_);
		material.extension_.sheenColorTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.sheen_.sheenColorTextureIndex_);
		material.extension_.sheenRoughnessTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.sheen_.sheenRoughnessTextureIndex_);
		material.extension_.iridescenceTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.iridescence_.iridescenceTextureIndex_);
		material.extension_.iridescenceThicknessTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.iridescence_.iridescenceThicknessTextureIndex_);
		material.extension_.anisotropyTextureIndex_ = residency.TextureBindlessIndex(surface.khr_.anisotropy_.anisotropyTextureIndex_);

		material.shading_.shadingModel_ = surface.khr_.unlit_.unlit_ != 0 ? static_cast<Uint>(ShadingModel::Unlit) : static_cast<Uint>(surface.shadingModel_);
		material.shading_.furLength_ = surface.furLength_;
		material.shading_.furDensity_ = surface.furDensity_;
		material.shading_.furShellCount_ = static_cast<Uint>(surface.furShellCount_);
		material.shading_.doubleSided_ = surface.doubleSided_ ? 1 : 0;
		material.shading_.blend_ = surface.alphaMode_ == 2 ? 1 : 0;
		material.shading_.materialMapping_ = static_cast<Uint>(surface.materialMapping_);
		material.shading_.tiling_ = surface.tiling_;
		material.shading_.triplanarSharpness_ = surface.triplanarSharpness_;
		material.shading_.outlineColor_ = static_cast<Uint>(Clamp(surface.outlineColor_.r, 0.0f, 1.0f) * 255.0f + 0.5f) | (static_cast<Uint>(Clamp(surface.outlineColor_.g, 0.0f, 1.0f) * 255.0f + 0.5f) << 8) | (static_cast<Uint>(Clamp(surface.outlineColor_.b, 0.0f, 1.0f) * 255.0f + 0.5f) << 16) | (static_cast<Uint>(Clamp(surface.outlineColor_.a, 0.0f, 1.0f) * 255.0f + 0.5f) << 24);
		material.shading_.outlineWidth_ = surface.outlineWidth_;
		return material;
	}
}

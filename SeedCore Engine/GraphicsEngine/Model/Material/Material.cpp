#include <GraphicsEngine/Model/Material/Material.h>
#include <GraphicsEngine/Model/Material/MaterialState.h>
#include <GraphicsEngine/Model/Material/MaterialResource.h>

namespace SeedCore
{
	Bool MaterialPanelRequest::openRequested_ = false;

	void Material::OnInspectorGUI()
	{
		ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));

		if (ImGui::Button("マテリアルビューアを開く"))
		{
			MaterialPanelRequest::openRequested_ = true;
		}
	}

	void Material::Set(Uint32 materialID, String slotName)
	{
		if (slotName.view().empty())
		{
			return;
		}

		for (Size lerpIndex = 0; lerpIndex < lerpSlotNames_.size(); lerpIndex++)
		{
			if (lerpSlotNames_[lerpIndex] == slotName)
			{
				lerpSlotNames_.erase(lerpSlotNames_.begin() + lerpIndex);
				lerpFromIDs_.erase(lerpFromIDs_.begin() + lerpIndex);
				lerpFactors_.erase(lerpFactors_.begin() + lerpIndex);
				break;
			}
		}

		slotMaterialIDs_.resize(slotNames_.size(), 0);

		Size assignmentIndex = slotNames_.size();
		for (Size index = 0; index < slotNames_.size(); index++)
		{
			if (slotNames_[index] == slotName)
			{
				assignmentIndex = index;
				break;
			}
		}

		if (materialID == 0)
		{
			if (assignmentIndex < slotNames_.size())
			{
				slotNames_.erase(slotNames_.begin() + assignmentIndex);
				slotMaterialIDs_.erase(slotMaterialIDs_.begin() + assignmentIndex);
			}
			return;
		}

		if (!std::ranges::contains(materialIDs_, materialID))
		{
			materialIDs_.push_back(materialID);
		}

		if (assignmentIndex < slotNames_.size())
		{
			slotMaterialIDs_[assignmentIndex] = materialID;
		}
		else
		{
			slotNames_.push_back(slotName);
			slotMaterialIDs_.push_back(materialID);
		}
	}

	void Material::Set(Uint32 materialID, String slotName, Float lerpFactor)
	{
		if (slotName.view().empty())
		{
			return;
		}

		Uint32 fromID = 0;
		Bool lerping = false;
		for (Size lerpIndex = 0; lerpIndex < lerpSlotNames_.size(); lerpIndex++)
		{
			if (lerpSlotNames_[lerpIndex] == slotName)
			{
				fromID = lerpFromIDs_[lerpIndex];
				lerping = true;
				break;
			}
		}

		if (!lerping)
		{
			Size count = Min(slotNames_.size(), slotMaterialIDs_.size());
			for (Size index = 0; index < count; index++)
			{
				if (slotNames_[index] == slotName)
				{
					Uint32 assignedID = slotMaterialIDs_[index];
					fromID = std::ranges::contains(materialIDs_, assignedID) ? assignedID : 0;
					break;
				}
			}
		}

		Set(materialID, slotName);

		if (lerpFactor >= 1.0f)
		{
			return;
		}

		lerpSlotNames_.push_back(slotName);
		lerpFromIDs_.push_back(fromID);
		lerpFactors_.push_back(Max(lerpFactor, 0.0f));
	}

	void Material::Apply(const Crister& crister, const Handle<Crister>& cristerHandle, MaterialResource& materialResource)
	{
		const DynamicArray<Surface>& seeds = crister.Surfaces();

		Uint64 signature = 14695981039346656037ull;
		signature = (signature ^ cristerHandle.index_) * 1099511628211ull;
		signature = (signature ^ cristerHandle.generation_) * 1099511628211ull;
		signature = (signature ^ seeds.size()) * 1099511628211ull;
		signature = (signature ^ materialResource.Revision()) * 1099511628211ull;
		for (Uint32 materialID : materialIDs_)
		{
			signature = (signature ^ materialID) * 1099511628211ull;
		}

		Size count = Min(slotNames_.size(), slotMaterialIDs_.size());
		for (Size index = 0; index < count; index++)
		{
			signature = (signature ^ std::hash<String>{}(slotNames_[index])) * 1099511628211ull;
			signature = (signature ^ slotMaterialIDs_[index]) * 1099511628211ull;
		}

		for (Size lerpIndex = 0; lerpIndex < lerpSlotNames_.size(); lerpIndex++)
		{
			signature = (signature ^ std::hash<String>{}(lerpSlotNames_[lerpIndex])) * 1099511628211ull;
			signature = (signature ^ lerpFromIDs_[lerpIndex]) * 1099511628211ull;
			signature = (signature ^ std::bit_cast<Uint32>(lerpFactors_[lerpIndex])) * 1099511628211ull;
		}

		if (signature == resolvedSignature_ && resolvedSurfaces_.size() == seeds.size())
		{
			return;
		}
		resolvedSignature_ = signature;

		resolvedSurfaces_.assign(seeds.begin(), seeds.end());

		for (Size surfaceIndex = 0; surfaceIndex < seeds.size(); surfaceIndex++)
		{
			const Surface& seed = seeds[surfaceIndex];
			if (seed.name_.empty())
			{
				continue;
			}
			String surfaceName = String(std::string_view(seed.name_));

			const Surface* target = &seed;
			for (Size index = 0; index < count; index++)
			{
				if (slotNames_[index] == surfaceName)
				{
					Uint32 assignedID = slotMaterialIDs_[index];
					if (assignedID != 0 && std::ranges::contains(materialIDs_, assignedID))
					{
						if (const Surface* assigned = materialResource.Resolve(materialResource.GetHandle(assignedID)))
						{
							target = assigned;
						}
					}
					break;
				}
			}

			Surface& resolved = resolvedSurfaces_[surfaceIndex];
			resolved = *target;

			for (Size lerpIndex = 0; lerpIndex < lerpSlotNames_.size(); lerpIndex++)
			{
				if (lerpSlotNames_[lerpIndex] != surfaceName)
				{
					continue;
				}

				const Surface* from = &seed;
				if (lerpFromIDs_[lerpIndex] != 0)
				{
					if (const Surface* lerpSource = materialResource.Resolve(materialResource.GetHandle(lerpFromIDs_[lerpIndex])))
					{
						from = lerpSource;
					}
				}

				Float lerpFactor = lerpFactors_[lerpIndex];
				if (lerpFactor < 0.5f)
				{
					resolved = *from;
				}

				resolved.baseColor_ = Color::Lerp(from->baseColor_, target->baseColor_, lerpFactor);
				resolved.metallic_ = Lerp(from->metallic_, target->metallic_, lerpFactor);
				resolved.roughness_ = Lerp(from->roughness_, target->roughness_, lerpFactor);
				resolved.alphaCutoff_ = Lerp(from->alphaCutoff_, target->alphaCutoff_, lerpFactor);
				resolved.furLength_ = Lerp(from->furLength_, target->furLength_, lerpFactor);
				resolved.furDensity_ = Lerp(from->furDensity_, target->furDensity_, lerpFactor);

				resolved.khr_.emissiveStrength_.emissiveStrength_ = Lerp(from->khr_.emissiveStrength_.emissiveStrength_, target->khr_.emissiveStrength_.emissiveStrength_, lerpFactor);
				resolved.khr_.ior_.ior_ = Lerp(from->khr_.ior_.ior_, target->khr_.ior_.ior_, lerpFactor);
				resolved.khr_.specular_.specularFactor_ = Lerp(from->khr_.specular_.specularFactor_, target->khr_.specular_.specularFactor_, lerpFactor);
				resolved.khr_.clearCoat_.clearCoatFactor_ = Lerp(from->khr_.clearCoat_.clearCoatFactor_, target->khr_.clearCoat_.clearCoatFactor_, lerpFactor);
				resolved.khr_.clearCoat_.clearCoatRoughnessFactor_ = Lerp(from->khr_.clearCoat_.clearCoatRoughnessFactor_, target->khr_.clearCoat_.clearCoatRoughnessFactor_, lerpFactor);
				resolved.khr_.transmission_.transmissionFactor_ = Lerp(from->khr_.transmission_.transmissionFactor_, target->khr_.transmission_.transmissionFactor_, lerpFactor);
				resolved.khr_.volume_.thicknessFactor_ = Lerp(from->khr_.volume_.thicknessFactor_, target->khr_.volume_.thicknessFactor_, lerpFactor);
				resolved.khr_.volume_.attenuationDistance_ = Lerp(from->khr_.volume_.attenuationDistance_, target->khr_.volume_.attenuationDistance_, lerpFactor);
				resolved.khr_.sheen_.sheenRoughnessFactor_ = Lerp(from->khr_.sheen_.sheenRoughnessFactor_, target->khr_.sheen_.sheenRoughnessFactor_, lerpFactor);
				resolved.khr_.iridescence_.iridescenceFactor_ = Lerp(from->khr_.iridescence_.iridescenceFactor_, target->khr_.iridescence_.iridescenceFactor_, lerpFactor);
				resolved.khr_.iridescence_.iridescenceIor_ = Lerp(from->khr_.iridescence_.iridescenceIor_, target->khr_.iridescence_.iridescenceIor_, lerpFactor);
				resolved.khr_.iridescence_.iridescenceThicknessMinimum_ = Lerp(from->khr_.iridescence_.iridescenceThicknessMinimum_, target->khr_.iridescence_.iridescenceThicknessMinimum_, lerpFactor);
				resolved.khr_.iridescence_.iridescenceThicknessMaximum_ = Lerp(from->khr_.iridescence_.iridescenceThicknessMaximum_, target->khr_.iridescence_.iridescenceThicknessMaximum_, lerpFactor);
				resolved.khr_.anisotropy_.anisotropyStrength_ = Lerp(from->khr_.anisotropy_.anisotropyStrength_, target->khr_.anisotropy_.anisotropyStrength_, lerpFactor);
				resolved.khr_.anisotropy_.anisotropyRotation_ = Lerp(from->khr_.anisotropy_.anisotropyRotation_, target->khr_.anisotropy_.anisotropyRotation_, lerpFactor);

				for (Size componentIndex = 0; componentIndex < 3; componentIndex++)
				{
					resolved.emissiveFactor_[componentIndex] = Lerp(from->emissiveFactor_[componentIndex], target->emissiveFactor_[componentIndex], lerpFactor);
					resolved.khr_.specular_.specularColorFactor_[componentIndex] = Lerp(from->khr_.specular_.specularColorFactor_[componentIndex], target->khr_.specular_.specularColorFactor_[componentIndex], lerpFactor);
					resolved.khr_.volume_.attenuationColor_[componentIndex] = Lerp(from->khr_.volume_.attenuationColor_[componentIndex], target->khr_.volume_.attenuationColor_[componentIndex], lerpFactor);
					resolved.khr_.sheen_.sheenColorFactor_[componentIndex] = Lerp(from->khr_.sheen_.sheenColorFactor_[componentIndex], target->khr_.sheen_.sheenColorFactor_[componentIndex], lerpFactor);
				}
				break;
			}
		}
	}
}

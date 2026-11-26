#pragma once
#include <GraphicsEngine/Avatar/Human/HumanCharacterModel.h>
#include <GraphicsEngine/Avatar/Human/HumanCharacterEvaluator.h>
#include <GraphicsEngine/Avatar/Animal/AnimalCharacterModel.h>
#include <GraphicsEngine/Avatar/Animal/AnimalCharacterEvaluator.h>
#include <GraphicsEngine/Texture/TextureLoader.h>

namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class AvatarMesh;
	enum class ExportPreset;

	enum class AvatarKind
	{
		Human,
		Animal
	};

	class AvatarPanel
	{
	public:
		AvatarPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~AvatarPanel();

		void Draw();

		void Inspect();

		void Open();

		NODISCARD Bool Focused()const;

	private:
		SC_CONSTEX Uint32 regionSlotCount_ = 4;

		void EnsureLoaded();

		void Bake(ExportPreset preset);

		void LoadRegionTexture(Uint32 regionIndex, String filePath);

		void ClearRegionTextures();

		void ExportUvLayout();

		NODISCARD Uint32 ActiveRegionCount()const;

		NODISCARD std::span<const Char* const> ActiveRegionLabels()const;

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		Bool cameraReady_ = false;

		AvatarKind kind_ = AvatarKind::Human;

		Int animalGroupPending_ = -1;

		Bool humanLoadAttempted_ = false;
		Bool animalLoadAttempted_ = false;

		HumanCharacterModel humanModel_;
		HumanCharacterEvaluator humanEvaluator_;
		AnimalCharacterModel animalModel_;
		AnimalCharacterEvaluator animalEvaluator_;

		ResourcePtr<AvatarMesh> humanMesh_;
		ResourcePtr<AvatarMesh> animalMesh_;

		Microsoft::WRL::ComPtr<ID3D12Resource> regionTextureResources_[regionSlotCount_];
		Uint32 regionTextureIndices_[regionSlotCount_] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
		String regionTexturePaths_[regionSlotCount_];
	};
}

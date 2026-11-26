#pragma once
#include <FoundationEngine/File/FilePath.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Utility/ArtMap.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <Editor/Editor/Panel/Embed/ResourceSyncControlPanel.h>

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture;

	enum class ExportPreset;

	class ContentsDrawerPanel
	{
	public:
		ContentsDrawerPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~ContentsDrawerPanel() = default;

		void Draw();

	private:
		struct FolderNode;
		enum class ScriptType;

		void DrawDirectoryTree(FolderNode& node);

		void DrawAssetListMode(FolderNode& folder);

		void DrawAssetGridMode(FolderNode& folder);

		void DrawAssetTooltip(const AssetRecord& asset);

		void DrawFolderMenu(const FilePath& folderPath);

		void DrawAssetMenu(const AssetRecord& asset, const FilePath& assetPath);

		void DrawBackgroundMenu();

		void DrawScriptMenu();

		Bool DrawRenameMenu(const FilePath& itemPath);

	private:
		void SelectDirectory(const FilePath& directoryPath);

		const FilePath& SelectedDirectory()const;

		void BuildDirectory();

	private:
		Uint64 GetAssetIcon(const AssetRecord& asset)const;

		Uint64 GetSharingIcon(const AssetRecord& asset)const;

		Uint64 GetFolderIcon(const FolderNode& node)const;

	private:
		const Char* GetPayloadType(AssetType type)const;

	private:
		void GenerateAsset(const AssetRecord& asset, Asset& resource, const Char* label);

	private:
		void ExecuteDelete(const FilePath& targetPath);

		void ExecuteRename(const FilePath& targetPath);

		void ExecutePaste(const FilePath& directoryPath);

		Bool ExecuteRegister(const FilePath& headerPath, const FilePath& cppPath);

	private:
		void CreateNewFolder(const FilePath& targetPath);

		void CreateNewEffect(const FilePath& folderPath);

		void CreateNewScript(const FilePath& targetPath);

	private:
		void OpenAssetPopup(const AssetRecord& asset);

		void OpenScriptPopup(const FilePath& parentPath, ScriptType script);

	private:
		void ExportModel(const AssetRecord& asset, ExportPreset preset, const Wchar* extension);

	private:
		enum class ViewType
		{
			List,
			Grid,
		};

		enum class ScriptType
		{
			Cpp,
			Csharp,
		};

		enum class ClipboardType
		{
			None,
			Cut,
			Copy,
		};

	private:
		struct FolderNode
		{
			FilePath path_;

			ArtMap<std::string, FolderNode> children_;

			DynamicArray<const AssetRecord*> assets_;
		};

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		ResourceSyncControlPanel resourceSyncControlPanel_;

		ImGuiTextFilter searchFilter_;

		ViewType viewType_ = ViewType::Grid;

		FolderNode root_;

		DynamicArray<AssetRecord> assetList_;

		Bool scriptMenuOpenRequested_ = false;
		Bool scriptMenuFocusRequested_ = false;
		ScriptType scriptMenuType_ = ScriptType::Cpp;
		std::optional<FilePath> scriptMenuTargetPath_;

		Bool renameMenuFocusRequested_ = false;
		std::optional<FilePath> renameMenuTargetPath_;

		ClipboardType clipboardType_ = ClipboardType::None;
		FilePath clipboardPath_;

		DynamicArray<FilePath> historyDirectoryList_;
		Size historyIndex_ = 0;

		Uint64 syncRevision_ = 0;

		Uint64 resourceRevision_ = 0;

		Float gridIconSize_ = 64.0f;

		mutable FlatMap<Uint32, Uint64> thumbnailCache_;
	};
}

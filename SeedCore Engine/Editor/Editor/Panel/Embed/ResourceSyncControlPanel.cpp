#include <Editor/Editor/Panel/Embed/ResourceSyncControlPanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	ResourceSyncControlPanel::ResourceSyncControlPanel(EditorContext& context, ImGuiTexture& imguiTexture) : context_(context), imguiTexture_(imguiTexture)
	{
		/// No Code
	}

	void ResourceSyncControlPanel::Draw()
	{
		ResourceSync& sync = *context_.application_.resourceSync_;
		if (!sync.Configured())
		{
			return;
		}

		ImGui::TextUnformatted(sync.Online() ? "共有ライブラリ: 接続中" : "共有ライブラリ: 未接続");
		ImGui::SameLine();
		if (ImGui::SmallButton("最新を取得"))
		{
			sync.RequestRefresh();
		}

		if (!sync.Operation().str().empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("%s", sync.Operation().c_str());
		}
		if (!sync.Error().str().empty())
		{
			ImGui::TextWrapped("%s", sync.Error().c_str());
		}
	}

	void ResourceSyncControlPanel::DrawActions(const AssetRecord& asset)
	{
		ResourceSync& sync = *context_.application_.resourceSync_;
		if (!sync.Configured())
		{
			return;
		}

		if (!sync.Shareable(std::filesystem::path(asset.fullpath_.str())))
		{
			return;
		}

		ImGui::BeginDisabled(!sync.Online());
		const SharedAsset* shared = sync.GetAsset(asset.assetID_);
		const SharedAsset* samePath = sync.GetAsset(std::filesystem::path(asset.fullpath_.str()));
		if (!shared && samePath)
		{
			ImGui::TextDisabled("ライブラリと識別子が違います");
			if (ImGui::MenuItem("ライブラリの版で置き換え"))
			{
				sync.RequestAdopt(samePath->runtimeId_);
			}
		}
		else if (!shared)
		{
			if (ImGui::MenuItem("チームへ共有"))
			{
				sync.RequestRegister(asset);
			}
		}
		else
		{
			DrawState(asset);
			if (ImGui::MenuItem("取得 / 更新"))
			{
				sync.RequestGet(asset.assetID_);
			}

			if (ImGui::MenuItem("ライブラリの版で置き換え"))
			{
				sync.RequestAdopt(asset.assetID_);
			}

			if (ImGui::MenuItem("変更を公開"))
			{
				sync.RequestPublish(asset.assetID_);
			}
			if (ImGui::BeginMenu("共有を解除..."))
			{
				ImGui::TextUnformatted("ドライブから削除されます。フォルダのファイルは残ります。");
				if (ImGui::MenuItem("実行する"))
				{
					sync.RequestUnshare(asset.assetID_);
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndDisabled();
		ImGui::Separator();
	}

	void ResourceSyncControlPanel::DrawState(const AssetRecord& asset)
	{
		ResourceSync& sync = *context_.application_.resourceSync_;
		const SharedAsset* shared = sync.GetAsset(asset.assetID_);
		if (!shared)
		{
			return;
		}

		ImGui::Text("共有 / r%llu", shared->revision_);
		if (sync.RemoteOnly(asset.assetID_))
		{
			ImGui::TextDisabled("この PC にはまだありません");
		}

		if (sync.Conflicted(asset.assetID_))
		{
			ImGui::Text("競合: 他のメンバーが先に公開しました。最新を取得すると手元の変更は上書きされます");
		}
		else if (sync.Modified(asset.assetID_))
		{
			ImGui::Text("未公開の変更があります");
		}
	}
}

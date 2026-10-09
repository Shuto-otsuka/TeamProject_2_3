#include <Editor/Editor/Panel/ResourceSyncControlPanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	/**
	* [EN]
	* Draws the one-line state of the shared library at the top of the
	* content browser: whether it is reachable, what it is doing, and
	* anything that went wrong.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コンテンツブラウザの上部に、共有ライブラリの状態を1行で描く。到達
	* できているか、今何をしているか、何か問題が起きていないか。
	*/
	void ResourceSyncControlPanel::DrawStatus(EditorContext& context)
	{
		ResourceSync& sync = *context.application_.resourceSync_;
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

		/// [EN] What the worker is doing is shown while it runs, because an upload can take long enough to look like a hang.
		/// [JP] 実行中の内容を表示する。アップロードは、固まって見えるほど時間がかかることがあるため。
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

	/**
	* [EN]
	* Adds the shared-library entries to an asset's context menu, which
	* differ depending on whether the team already has it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの右クリックメニューへ、共有ライブラリ用の項目を足す。
	* 内容は、チームが既にそれを持っているかどうかで変わる。
	*/
	void ResourceSyncControlPanel::DrawActions(EditorContext& context, const AssetRecord& asset)
	{
		ResourceSync& sync = *context.application_.resourceSync_;
		if (!sync.Configured())
		{
			return;
		}

		/// [EN] Source code and anything outside the workspace get no entries at all, rather than ones that could only fail.
		/// [JP] ソースコードやワークスペース外のものには、失敗するしかない項目を出すのではなく、項目自体を出さない。
		if (!sync.Shareable(std::filesystem::path(asset.fullpath_.str())))
		{
			return;
		}

		/// [EN] Every entry is disabled while the library is out of reach, since each one needs it to answer.
		/// [JP] ライブラリへ届いていない間は全ての項目を無効にする。どれも応答を必要とするため。
		ImGui::BeginDisabled(!sync.Online());
		const SharedAsset* shared = sync.GetAsset(asset.assetID_);
		const SharedAsset* samePath = sync.GetAsset(std::filesystem::path(asset.fullpath_.str()));
		if (!shared && samePath)
		{
			/// [EN] The library holds this path under another identifier, so the only sensible step is to take its copy and .meta.
			/// [JP] ライブラリはこの位置を別の識別子で持っている。取るべき手は、その写しと .meta を受け取ることだけ。
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
			DrawState(context, asset);
			if (ImGui::MenuItem("取得 / 更新"))
			{
				sync.RequestGet(asset.assetID_);
			}

			/// [EN] Unlike a get, this replaces local files even when they hold unpublished work, so it is the way out of a conflict that keeps the team's side.
			/// [JP] 取得と違い、未公開の作業があってもローカルのファイルを置き換える。チーム側を残して競合を抜ける手段。
			if (ImGui::MenuItem("ライブラリの版で置き換え"))
			{
				sync.RequestAdopt(asset.assetID_);
			}

			/// [EN] Publishing always goes through and replaces whatever the library held, so the last upload is what the team gets.
			/// [JP] 公開は常に通り、ライブラリの内容を置き換える。チームに届くのは最後にアップロードしたもの。
			if (ImGui::MenuItem("変更を公開"))
			{
				sync.RequestPublish(asset.assetID_);
			}
			/// [EN] Unsharing removes it from Drive for everyone, while every member's local copy stays where it is.
			/// [JP] 共有の解除は Drive から全員分を取り除く。各メンバーの手元の写しはそのまま残る。
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

	/**
	* [EN]
	* Draws an asset's revision and how this copy stands against the
	* library, for the details pane and the context menu.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの Revision と、手元の写しがライブラリに対してどういう状態
	* かを描く。詳細欄と右クリックメニューで使う。
	*/
	void ResourceSyncControlPanel::DrawState(EditorContext& context, const AssetRecord& asset)
	{
		ResourceSync& sync = *context.application_.resourceSync_;
		const SharedAsset* shared = sync.GetAsset(asset.assetID_);
		if (!shared)
		{
			return;
		}

		/// [EN] The revision is what a member compares when deciding whether their copy is the current one.
		/// [JP] Revision は、手元の写しが最新かどうかをメンバーが判断する際の比較対象になる。
		ImGui::Text("共有 / r%llu", shared->revision_);
		if (sync.RemoteOnly(asset.assetID_))
		{
			ImGui::TextDisabled("この PC にはまだありません");
		}

		/// [EN] Both sides having moved is stated plainly, since publishing now replaces the other member's newer copy.
		/// [JP] 両側が動いている場合ははっきり示す。今公開すると、他のメンバーの新しい写しを置き換えることになるため。
		if (sync.Conflicted(asset.assetID_))
		{
			ImGui::Text("競合: 公開するとライブラリの新しい版を上書きします");
		}
		else if (sync.Modified(asset.assetID_))
		{
			ImGui::Text("未公開の変更があります");
		}
	}
}

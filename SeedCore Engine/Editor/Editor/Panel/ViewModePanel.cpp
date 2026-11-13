#include <Editor/Editor/Panel/ViewModePanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	ViewModePanel::ViewModePanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

	/**
	* [EN]
	* Draws the view mode tree followed by the show flag submenu, inside an
	* open popup or menu.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表示モードのツリーと、その後ろに表示フラグのサブメニューを描く。開いている
	* ポップアップかメニューの内側で呼ぶこと。
	*/
	void ViewModePanel::Draw()
	{
		DrawMode();

		ImGui::Separator();

		if (ImGui::BeginMenu("表示フラグ"))
		{
			DrawFlag();
			ImGui::EndMenu();
		}
	}

	/**
	* [EN]
	* Lists the basic modes at the top and every visualization category as a
	* submenu, selecting into EditorViewContext::viewMode_. The ReSTIR and
	* denoiser submenus also pick the ray-traced effect they inspect into
	* EditorViewContext::debugSource_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 基本のモードを先頭に、各ビジュアライゼーションのカテゴリをサブメニューで並べ、
	* 選んだものを EditorViewContext::viewMode_ に入れる。ReSTIR とデノイザーの
	* サブメニューでは、調べるレイトレーシングの効果も EditorViewContext::debugSource_
	* に選ぶ。
	*/
	void ViewModePanel::DrawMode()
	{
		RenderViewMode& mode = context_.view_.editor_.viewMode_;
		RenderDebugSource& source = context_.view_.editor_.debugSource_;
		auto item = [&](const Char* label, RenderViewMode value)
			{
				if (ImGui::MenuItem(label, nullptr, mode == value))
				{
					mode = value;
				}
			};
		auto sourceItem = [&](const Char* label, RenderDebugSource value)
			{
				if (ImGui::MenuItem(label, nullptr, source == value))
				{
					source = value;
				}
			};

		item("ライティングあり", RenderViewMode::Lit);
		item("ライティングなし", RenderViewMode::Unlit);
		item("ワイヤーフレーム", RenderViewMode::Wireframe);
		item("ディテールライティング", RenderViewMode::DetailLighting);
		item("ライティングのみ", RenderViewMode::LightingOnly);
		item("反射", RenderViewMode::Reflections);

		ImGui::Separator();

		if (ImGui::BeginMenu("バッファービジュアライゼーション"))
		{
			item("ベースカラー", RenderViewMode::BufferBaseColor);
			item("法線", RenderViewMode::BufferNormal);
			item("ラフネス", RenderViewMode::BufferRoughness);
			item("メタルネス", RenderViewMode::BufferMetallic);
			item("エミッシブ", RenderViewMode::BufferEmissive);
			item("深度", RenderViewMode::BufferDepth);
			item("モーションベクター", RenderViewMode::BufferVelocity);
			item("ワールド位置", RenderViewMode::BufferPosition);
			item("マテリアル ID", RenderViewMode::BufferMaterial);
			ImGui::Separator();
			item("バッファー一覧", RenderViewMode::BufferOverview);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("メッシュレットビジュアライゼーション"))
		{
			item("メッシュレット", RenderViewMode::MeshletId);
			item("三角形", RenderViewMode::MeshletTriangle);
			item("クラスタ", RenderViewMode::MeshletCluster);
			item("インスタンス", RenderViewMode::MeshletInstance);
			item("LOD レベル", RenderViewMode::MeshletLevel);
			item("LOD 誤差", RenderViewMode::MeshletError);
			item("オクルージョンカリング", RenderViewMode::MeshletCulling);
			item("バウンド", RenderViewMode::MeshletBounds);
			item("オーバードロー", RenderViewMode::MeshletOverdraw);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ライティングビジュアライゼーション"))
		{
			item("直接光（拡散）", RenderViewMode::DirectDiffuse);
			item("直接光（鏡面）", RenderViewMode::DirectSpecular);
			item("間接光（拡散）", RenderViewMode::IndirectDiffuse);
			item("間接光（鏡面）", RenderViewMode::IndirectSpecular);
			ImGui::Separator();
			item("ライトの複雑さ", RenderViewMode::LightComplexity);
			item("ライトクラスタ", RenderViewMode::LightCluster);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("レイトレーシングビジュアライゼーション"))
		{
			item("ヒット距離", RenderViewMode::RayDistance);
			item("ヒット／ミス", RenderViewMode::RayHit);
			item("レイの方向", RenderViewMode::RayDirection);
			item("レイの本数", RenderViewMode::RayCount);
			item("インスタンス ID", RenderViewMode::RayInstance);
			item("ジオメトリ ID", RenderViewMode::RayGeometry);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ReSTIR ビジュアライゼーション"))
		{
			ImGui::SeparatorText("対象");
			sourceItem("反射", RenderDebugSource::Reflection);
			sourceItem("グローバルイルミネーション", RenderDebugSource::GlobalIllumination);
			ImGui::SeparatorText("表示");
			item("Reservoir M", RenderViewMode::ReservoirCount);
			item("ウェイト", RenderViewMode::ReservoirWeight);
			item("経過フレーム", RenderViewMode::ReservoirAge);
			item("サンプルの放射輝度", RenderViewMode::ReservoirRadiance);
			item("時間方向の再利用", RenderViewMode::ReservoirTemporal);
			item("空間方向の再利用", RenderViewMode::ReservoirSpatial);
			item("履歴の棄却", RenderViewMode::ReservoirRejection);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("デノイザービジュアライゼーション"))
		{
			ImGui::SeparatorText("対象");
			sourceItem("反射", RenderDebugSource::Reflection);
			sourceItem("グローバルイルミネーション", RenderDebugSource::GlobalIllumination);
			sourceItem("シャドウ", RenderDebugSource::Shadow);
			sourceItem("アンビエントオクルージョン", RenderDebugSource::AmbientOcclusion);
			ImGui::SeparatorText("表示");
			item("デノイズ前", RenderViewMode::DenoiserRaw);
			item("デノイズ後", RenderViewMode::DenoiserDenoised);
			item("分散", RenderViewMode::DenoiserVariance);
			item("履歴の長さ", RenderViewMode::DenoiserLength);
			item("履歴の有効性", RenderViewMode::DenoiserValidity);
			item("再投影", RenderViewMode::DenoiserReprojection);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("最適化ビューモード"))
		{
			item("シェーダーの複雑さ", RenderViewMode::ShaderComplexity);
			item("クアッドオーバードロー", RenderViewMode::QuadOverdraw);
			item("三角形の密度", RenderViewMode::TriangleDensity);
			item("メッシュレットの密度", RenderViewMode::MeshletDensity);
			item("テクスチャのミップ", RenderViewMode::TextureMip);
			item("LOD の色分け", RenderViewMode::LodColoration);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ポストプロセスビジュアライゼーション"))
		{
			item("HDR カラー", RenderViewMode::PostColor);
			item("輝度", RenderViewMode::PostLuminance);
			item("ブルーム", RenderViewMode::PostBloom);
			item("露出", RenderViewMode::PostExposure);
			item("トーンマッピング", RenderViewMode::PostTonemap);
			ImGui::Separator();
			item("DLSS 入力", RenderViewMode::UpscaleInput);
			item("DLSS 出力", RenderViewMode::UpscaleOutput);
			ImGui::EndMenu();
		}
	}

	/**
	* [EN]
	* Lists every show flag as a check item, grouped into submenus by
	* category, toggling its bit in EditorViewContext::showFlags_. Clicking
	* an item keeps the menu open so several flags can be changed in a row.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全ての表示フラグを、カテゴリごとのサブメニューにチェック項目で並べ、
	* EditorViewContext::showFlags_ のビットを切り替える。続けていくつも
	* 切り替えられるよう、押してもメニューは閉じない。
	*/
	void ViewModePanel::DrawFlag()
	{
		Uint64& flags = context_.view_.editor_.showFlags_;
		auto item = [&](const Char* label, RenderShowFlag flag)
			{
				Uint64 bit = static_cast<Uint64>(flag);
				if (ImGui::MenuItem(label, nullptr, (flags & bit) != 0))
				{
					flags ^= bit;
				}
			};

		ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

		if (ImGui::BeginMenu("ジオメトリ"))
		{
			item("モデル", RenderShowFlag::Model);
			item("プリミティブ", RenderShowFlag::Primitive);
			item("半透明", RenderShowFlag::Transparent);
			item("ファー", RenderShowFlag::Fur);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("環境"))
		{
			item("空", RenderShowFlag::Sky);
			item("ボリュメトリッククラウド", RenderShowFlag::VolumetricCloud);
			item("星空", RenderShowFlag::VolumetricStar);
			item("天候", RenderShowFlag::Weather);
			item("ボリュメトリックライト", RenderShowFlag::VolumetricLight);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ライティング"))
		{
			item("直接光", RenderShowFlag::DirectLight);
			item("シャドウ", RenderShowFlag::Shadow);
			item("反射", RenderShowFlag::Reflection);
			item("グローバルイルミネーション", RenderShowFlag::GlobalIllumination);
			item("アンビエントオクルージョン", RenderShowFlag::AmbientOcclusion);
			item("サブサーフェススキャタリング", RenderShowFlag::SubsurfaceScattering);
			item("屈折", RenderShowFlag::Refraction);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("ポストプロセス"))
		{
			item("ポストプロセス", RenderShowFlag::PostProcess);
			ImGui::Separator();
			item("ブルーム", RenderShowFlag::Bloom);
			item("レンズフレア", RenderShowFlag::LensFlare);
			item("アナモルフィックフレア", RenderShowFlag::AnamorphicFlare);
			item("被写界深度", RenderShowFlag::DepthOfField);
			item("ボケ", RenderShowFlag::Bokeh);
			item("色収差", RenderShowFlag::ChromaticAberration);
			item("ビネット", RenderShowFlag::Vignette);
			item("フィルムグレイン", RenderShowFlag::FilmGrain);
			item("レンズ歪み", RenderShowFlag::LensDistortion);
			item("カラーグレーディング", RenderShowFlag::ColorGrading);
			item("トーンマッピング", RenderShowFlag::ToneMapping);
			item("自動露出", RenderShowFlag::AutoExposure);
			item("シャープネス", RenderShowFlag::Sharpness);
			item("アウトライン", RenderShowFlag::Outline);
			ImGui::EndMenu();
		}

		ImGui::PopItemFlag();
	}
}

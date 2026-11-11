#include <Editor/Editor/Panel/GraphicsMenuPanel.h>
#include <Editor/Editor/Context/EditorContext.h>

namespace SeedCore
{
	GraphicsMenuPanel::GraphicsMenuPanel(EditorContext& context) : context_(context)
	{
		/// No Code
	}

	void GraphicsMenuPanel::Draw()
	{
		if (ImGui::BeginMenu("グラフィックス"))
		{
			if (ImGui::BeginMenu("ビューモード"))
			{
				auto item = [&](const Char* label, RenderViewMode mode)
				{
					if (ImGui::MenuItem(label, nullptr, context_.view_.editor_.viewMode_ == mode))
					{
						context_.view_.editor_.viewMode_ = mode;
					}
				};

				item("ライティングあり", RenderViewMode::Lit);
				item("ライティングなし", RenderViewMode::Unlit);
				item("ワイヤーフレーム", RenderViewMode::Wireframe);
				item("深度", RenderViewMode::Depth);
				item("メッシュレット", RenderViewMode::Meshlet);

				if (ImGui::BeginMenu("バッファービジュアライゼーション"))
				{
					item("法線", RenderViewMode::Normal);
					item("ラフネス", RenderViewMode::Roughness);
					item("メタルネス", RenderViewMode::Metallic);
					item("エミッシブ", RenderViewMode::Emissive);
					item("モーションベクター", RenderViewMode::Velocity);
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("レイトレーシング"))
				{
					item("反射（生）", RenderViewMode::ReflectionRaw);
					item("反射（デノイズ後）", RenderViewMode::ReflectionDenoised);
					ImGui::Separator();
					item("グローバルイルミネーション（生）", RenderViewMode::GlobalIlluminationRaw);
					item("グローバルイルミネーション（デノイズ後）", RenderViewMode::GlobalIlluminationDenoised);
					ImGui::Separator();
					item("シャドウ（生）", RenderViewMode::ShadowRaw);
					item("シャドウ（デノイズ後）", RenderViewMode::ShadowDenoised);
					ImGui::Separator();
					item("アンビエントオクルージョン（生）", RenderViewMode::AmbientOcclusionRaw);
					item("アンビエントオクルージョン（デノイズ後）", RenderViewMode::AmbientOcclusionDenoised);
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}
	}
}

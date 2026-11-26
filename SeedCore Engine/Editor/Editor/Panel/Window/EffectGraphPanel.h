#pragma once
#include <Editor/Editor/Node/NodeCanvas.h>

namespace SeedCore
{
	struct EditorContext;
	struct EffectAsset;
	struct EffectEmitter;

	class ImGuiTexture;

	class EffectGraphPanel
	{
	public:
		explicit EffectGraphPanel(EditorContext& context, ImGuiTexture& imguiTexture);

		~EffectGraphPanel() = default;

		void Draw();

		void Open(Uint32 assetID);

		void Inspect();

		NODISCARD Bool Focused()const;

	private:
		void DrawGraph(EffectAsset& effect);

		void DrawView(EffectAsset& effect);

		void DrawEmitters(EffectAsset& effect);

		void DrawNodes(EffectEmitter& emitter);

		void DrawLinks(EffectEmitter& emitter);

		void DrawMenu(EffectEmitter& emitter);

	private:
		void UpdateConnect(EffectEmitter& emitter);

		void UpdateLayout(EffectEmitter& emitter);

	private:
		EditorContext& context_;

		ImGuiTexture& imguiTexture_;

		Bool open_ = false;

		NodeCanvas canvas_;

		Uint32 assetID_ = 0;

		Size selectedEmitterIndex_ = 0;

		Bool dirty_ = false;
	};
}
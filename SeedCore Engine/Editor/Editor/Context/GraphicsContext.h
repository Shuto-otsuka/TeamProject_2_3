#pragma once

namespace SeedCore
{
	class Graphics;
	class ImGuiRenderer;

	struct GraphicsContext
	{
		Graphics* graphics_ = nullptr;

		ImGuiRenderer* imgui_ = nullptr;
	};
}

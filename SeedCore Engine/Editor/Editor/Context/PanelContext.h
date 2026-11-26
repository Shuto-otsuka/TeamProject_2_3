#pragma once
#include <Editor/Editor/Panel/PanelGroup.h>

namespace SeedCore
{
	struct PanelContext
	{
		ToolPanels* tools_ = nullptr;

		DockPanels* docks_ = nullptr;
	};
}

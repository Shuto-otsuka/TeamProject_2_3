#include <FoundationEngine/Interop/ImGuiInstance.h>

namespace SeedCore
{
	void ImGuiInstance::Bind(ImGuiContext* context)
	{
		ImGui::SetCurrentContext(context);
	}
}

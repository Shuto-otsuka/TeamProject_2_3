#include <FoundationEngine/Interop/ImGuiInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* Makes context the current ImGui context of SeedCore.Cplusplus.dll's
	* copy of ImGui.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* context を、SeedCore.Cplusplus.dll の ImGui のカレントコンテキストに
	* する。
	*/
	void ImGuiInstance::Bind(ImGuiContext* context)
	{
		/// [EN] Compiled into SeedCore.Cplusplus.dll, so this sets that module's own current-context pointer, not the caller's.
		/// [JP] SeedCore.Cplusplus.dll の中にコンパイルされるため、呼び出し側ではなく、このモジュール自身のカレントコンテキストのポインタを設定する。
		ImGui::SetCurrentContext(context);
	}
}

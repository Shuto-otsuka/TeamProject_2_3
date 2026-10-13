#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	/**
	* [EN]
	* Releases whatever this manager still holds after a per-asset
	* Unload pass. Defaults to doing nothing, for managers whose Unload
	* already leaves nothing behind.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット単位の Unload パスの後に、このマネージャがまだ保持して
	* いるものを解放する。既定では何もしない（Unload の時点で何も残らない
	* マネージャ向け）。
	*/
	void Asset::Clear()
	{
		/// No Code
	}
}

#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>

namespace SeedCore
{
	/**
	* [EN]
	* Where one of an entity's sparse-stored components is: which
	* component's sparse set, and the row within that set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンティティのスパース格納コンポーネント1つがある場所。どの
	* コンポーネントのスパースセットか、そのセット内の行。
	*/
	struct SparseSetLocation
	{
		/// [EN] The component, which also identifies the sparse set it is stored in.
		/// [JP] コンポーネント。格納先のスパースセットもこれで決まる。
		ComponentID id_ = nullptr;

		/// [EN] Row within that sparse set; UINT32_MAX while unplaced.
		/// [JP] そのスパースセット内の行。未配置の間は UINT32_MAX。
		Uint32 row_ = UINT32_MAX;

		/// [EN] The component itself; it never moves while present, so reads skip the sparse set entirely.
		/// [JP] コンポーネント本体。存在している間は動かないので、読むときはスパースセットを経由しない。
		void* data_ = nullptr;
	};
}
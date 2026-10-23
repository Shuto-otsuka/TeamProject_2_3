#include <FoundationEngine/World/Service.h>

namespace SeedCore
{
	/**
	* [EN]
	* Destroys every service, in the reverse order they were added.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのサービスを、入れた順の逆に壊す。
	*/
	Service::~Service()
	{
		/// [EN] Reverse order, so a service added later and holding a reference to an earlier one is gone before what it refers to.
		/// [JP] 逆順に壊すので、先に入れたものへの参照を持つ後のサービスが、参照先より先に無くなる。
		for (auto it = slots_.rbegin(); it != slots_.rend(); ++it)
		{
			it->destroy_(it->service_);
		}
	}

	/**
	* [EN]
	* Type-erased overload of Get: returns the service whose type's name is
	* type (a ServiceType<T>::name_), or nullptr if none has been added.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Get の型消去版: 型の名前が type（ServiceType<T>::name_）である
	* サービスを返す。入れられていなければ nullptr を返す。
	*/
	void* Service::Get(const String& type)const
	{
		/// [EN] There are only a handful of services, so the slots are walked comparing interned names, one pointer comparison each.
		/// [JP] サービスは数個しか無いので、intern した名前を比べながら枠をたどる。1つにつきポインタの比較1回。
		for (const Slot& slot : slots_)
		{
			if (slot.type_ == type)
			{
				return slot.service_;
			}
		}
		return nullptr;
	}
}
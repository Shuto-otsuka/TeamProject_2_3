#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Per-type statics for service type T: the interned name of T's typeid,
	* which identifies T's slot in a Service. Each module that names T gets
	* its own copy, and every copy holds the same interned name, so a lookup
	* from any module finds the same slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* サービスの型 T ごとの静的なもの: T の typeid の名前を intern した
	* もので、Service の中の T の枠を見分ける。T を使う各モジュールが
	* それぞれ自分の写しを持つが、どの写しも同じ intern 済みの名前を持つ
	* ので、どのモジュールから引いても同じ枠に当たる。
	*/
	template<typename T>
	struct ServiceType
	{
		/// [EN] The interned name of T's typeid. Interned during static initialization, before any service is added or looked up.
		/// [JP] T の typeid の名前を intern したもの。静的初期化の間に intern されるので、サービスを入れたり引いたりするより前に用意できている。
		static inline const String name_ = String::intern(typeid(T).name());
	};

	/**
	* [EN]
	* The per-World services scripts and engine systems reach through
	* World::GetService<T> (Physics, Audio, DebugDraw, and the records they
	* write into). Service knows none of them: each module creates its own
	* service and adds it here when the engine boots, so the dependency
	* points from the modules down to FoundationEngine and never back up.
	*
	* Each service sits in a slot together with its type's name
	* (ServiceType<T>::name_) and the function that destroys it, both
	* captured by Add<T> while T is still known. There are only a handful of
	* services, so a lookup walks the slots comparing interned names (one
	* pointer comparison each). Services are destroyed in the reverse order
	* they were added, so one added later may hold a reference to one added
	* earlier (Physics to its query record).
	*
	* Individual non-template members carry SEEDCORE_API rather than the
	* class itself, for the same reason as Actor: a class-level
	* dllexport/dllimport propagates to member function templates under
	* Clang.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スクリプトとエンジンのシステムが World::GetService<T> で引く、World
	* ごとのサービス（Physics、Audio、DebugDraw と、それらが書き込む記録）。
	* Service はどれも知らない: 各モジュールが自分のサービスを作り、
	* エンジンの起動時にここへ入れる。そのため依存は各モジュールから
	* FoundationEngine へ向かうだけで、逆向きにはならない。
	*
	* 各サービスは、型の名前（ServiceType<T>::name_）と、それを壊す関数と
	* 一緒に1つの枠に入る。どちらも T がまだ分かっている Add<T> が記録
	* する。サービスは数個しか無いので、探すときは intern した名前を比べ
	* ながら枠をたどる（1つにつきポインタの比較1回）。サービスは入れた順の
	* 逆に壊すので、後から入れたものが先に入れたものへの参照を持ってよい
	* （Physics がクエリの記録を持つなど）。
	*
	* テンプレートでないメンバーには、クラス全体ではなく個別に
	* SEEDCORE_API を付ける。Actor と同じ理由で、Clang ではクラス単位の
	* dllexport/dllimport がメンバー関数テンプレートにまで伝わるため。
	*/
	class Service :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Destroys every service, in the reverse order they were added.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全てのサービスを、入れた順の逆に壊す。
		*/
		SEEDCORE_API ~Service();

		/**
		* [EN]
		* Creates a T from arguments, adds it as this owner's T service, and
		* returns it. If a T service is already present, nothing is created
		* and the existing one is returned.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* arguments から T を作り、この持ち主の T サービスとして入れて、
		* それを返す。T サービスが既にあれば、何も作らずに既存のものを返す。
		*/
		template<typename T, typename... Arguments>
		T* Add(Arguments&&... arguments)
		{
			if (T* existing = Get<T>())
			{
				return existing;
			}

			/// [EN] The destroy function is captured here, while T is known, so the slot can destroy the service later without knowing its type.
			/// [JP] T が分かっているここで壊す関数を記録しておき、後で枠が型を知らないままサービスを壊せるようにする。
			T* service = new T(std::forward<Arguments>(arguments)...);
			slots_.push_back(Slot{ ServiceType<T>::name_, service, [](void* pointer) { delete static_cast<T*>(pointer); } });
			return service;
		}

		/**
		* [EN]
		* Returns the T service, or nullptr if none has been added.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T サービスを返す。入れられていなければ nullptr を返す。
		*/
		template<typename T>
		T* Get()const
		{
			return static_cast<T*>(Get(ServiceType<T>::name_));
		}

		/**
		* [EN]
		* Type-erased overload of Get: returns the service whose type's name
		* is type (a ServiceType<T>::name_), or nullptr if none has been
		* added. Lets code that cannot name T's owner, such as Actor, still
		* reach the service.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Get の型消去版: 型の名前が type（ServiceType<T>::name_）である
		* サービスを返す。入れられていなければ nullptr を返す。Actor のように、
		* 持ち主の定義を見られないコードからもサービスに届くようにする。
		*/
		SEEDCORE_API void* Get(const String& type)const;

		/**
		* [EN]
		* Destroys the T service now, before its owner, for a service whose
		* engine subsystem shuts down first. Does nothing if none is present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T サービスを、持ち主より先に今破棄する。そのサービスが頼る
		* エンジンの仕組みの方が先に終わる場合に使う。無ければ何もしない。
		*/
		template<typename T>
		void Remove()
		{
			auto it = std::ranges::find(slots_, ServiceType<T>::name_, &Slot::type_);
			if (it == slots_.end())
			{
				return;
			}

			it->destroy_(it->service_);
			slots_.erase(it);
		}

	private:
		/**
		* [EN]
		* One added service, with what is needed to find and destroy it
		* without knowing its type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 入れられたサービス1つと、型を知らずにそれを探して壊すのに必要な
		* もの。
		*/
		struct Slot
		{
			/// [EN] ServiceType<T>::name_ of the service's type.
			/// [JP] サービスの型の ServiceType<T>::name_。
			String type_;

			/// [EN] The service itself.
			/// [JP] サービス自体。
			void* service_ = nullptr;

			/// [EN] Destroys service_ as its real type; captured by Add<T>.
			/// [JP] service_ を本当の型として壊す。Add<T> が記録する。
			void (*destroy_)(void*) = nullptr;
		};

		/// [EN] The services, in the order they were added.
		/// [JP] サービスを入れた順に並べたもの。
		DynamicArray<Slot> slots_;
	};
}
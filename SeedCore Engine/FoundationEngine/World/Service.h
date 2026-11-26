#pragma once

namespace SeedCore
{
	template<typename T>
	struct ServiceType
	{
		static inline const String name_ = String::intern(typeid(T).name());
	};

	class Service :public NonTransferable
	{
	public:
		SEEDCORE_API ~Service();

		template<typename T, typename... Arguments>
		T* Add(Arguments&&... arguments)
		{
			if (T* existing = Get<T>())
			{
				return existing;
			}

			T* service = new T(std::forward<Arguments>(arguments)...);
			slots_.push_back(Slot{ ServiceType<T>::name_, service, [](void* pointer) { delete static_cast<T*>(pointer); } });
			return service;
		}

		template<typename T>
		T* Get()const
		{
			return static_cast<T*>(Get(ServiceType<T>::name_));
		}

		SEEDCORE_API void* Get(const String& type)const;

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
		struct Slot
		{
			String type_;

			void* service_ = nullptr;

			void (*destroy_)(void*) = nullptr;
		};

		DynamicArray<Slot> slots_;
	};
}
#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	template<typename T, typename H, Size LogSize>
	class ObjectPool;

	template <typename T>
	class ResourcePtr
	{
	public:
		constexpr ResourcePtr()noexcept = default;

		constexpr ResourcePtr(std::nullptr_t)noexcept
		{
			/// No Code
		}

		explicit ResourcePtr(T* pointer)noexcept: pointer_(pointer)
		{
			/// No Code
		}

		ResourcePtr(T* pointer, void* pool, void (*recycle)(void*, void*))noexcept: pointer_(pointer), pool_(pool), recycle_(recycle)
		{
			/// No Code
		}

		ResourcePtr(const ResourcePtr&) = delete;

		ResourcePtr& operator=(const ResourcePtr&) = delete;

		ResourcePtr(ResourcePtr&& other)noexcept: pointer_(other.pointer_), pool_(other.pool_), recycle_(other.recycle_)
		{
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourcePtr(ResourcePtr<U>&& other)noexcept: pointer_(other.pointer_), pool_(other.pool_), recycle_(other.recycle_)
		{
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
		}

		ResourcePtr& operator=(ResourcePtr&& other)noexcept
		{
			if (this != &other)
			{
				reset();
				pointer_ = other.pointer_;
				pool_ = other.pool_;
				recycle_ = other.recycle_;
				other.pointer_ = nullptr;
				other.pool_ = nullptr;
				other.recycle_ = nullptr;
			}
			return *this;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourcePtr& operator=(ResourcePtr<U>&& other)noexcept
		{
			reset();
			pointer_ = other.pointer_;
			pool_ = other.pool_;
			recycle_ = other.recycle_;
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
			return *this;
		}

		ResourcePtr& operator=(std::nullptr_t)noexcept
		{
			reset();
			return *this;
		}

		~ResourcePtr()
		{
			reset();
		}

		void reset()noexcept
		{
			if (pointer_ != nullptr)
			{
				if (recycle_ != nullptr)
				{
					void* object = nullptr;
					if constexpr (std::is_polymorphic_v<T>)
					{
						object = dynamic_cast<void*>(const_cast<std::remove_cv_t<T>*>(pointer_));
					}
					else
					{
						object = const_cast<std::remove_cv_t<T>*>(pointer_);
					}
					recycle_(pool_, object);
				}
				else
				{
					delete pointer_;
				}
				pointer_ = nullptr;
				pool_ = nullptr;
				recycle_ = nullptr;
			}
		}

		T* get()const noexcept
		{
			return pointer_;
		}

		T* operator->()const
		{
			SC_ASSERT(pointer_ != nullptr, "Dereferencing a null ResourcePtr.");
			return pointer_;
		}

		T& operator*()const
		{
			SC_ASSERT(pointer_ != nullptr, "Dereferencing a null ResourcePtr.");
			return *pointer_;
		}

		explicit operator bool()const noexcept
		{
			return pointer_ != nullptr;
		}

		friend Bool operator==(const ResourcePtr& lhs, const ResourcePtr& rhs)noexcept
		{
			return lhs.pointer_ == rhs.pointer_;
		}

		friend Bool operator==(const ResourcePtr& lhs, std::nullptr_t)noexcept
		{
			return lhs.pointer_ == nullptr;
		}

	private:
		template <typename U>
		friend class ResourcePtr;

		T* pointer_ = nullptr;

		void* pool_ = nullptr;

		void (*recycle_)(void*, void*) = nullptr;
	};

	template <typename T, typename... Args>
	NODISCARD ResourcePtr<T> MakePtr(Args&&... args)
	{
		return ResourcePtr<T>(new T(std::forward<Args>(args)...));
	}

	template <typename T, typename H, Size LogSize, typename... Args>
	NODISCARD ResourcePtr<T> MakePtr(ObjectPool<T, H, LogSize>& pool, Args&&... args)
	{
		return ResourcePtr<T>(pool.Create(std::forward<Args>(args)...), &pool, [](void* owner, void* object) { static_cast<ObjectPool<T, H, LogSize>*>(owner)->Recycle(static_cast<T*>(object)); });
	}
}

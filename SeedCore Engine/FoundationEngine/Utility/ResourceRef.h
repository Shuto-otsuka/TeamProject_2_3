#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	template <typename T>
	class ResourceRef;

	template <typename T, typename... Args>
	NODISCARD ResourceRef<T> MakeRef(Args&&... args);

	template <typename T>
	NODISCARD ResourceRef<T> MakeObserve(const ResourceRef<T>& owner)noexcept;

	enum class RefMode : Uint8
	{
		Owning,

		Observing,
	};

	namespace Detail
	{
		struct RefControlBase
		{
			std::atomic<Uint32> strongCount_{ 1 };

			std::atomic<Uint32> observerCount_{ 1 };

			std::atomic<Bool> alive_{ true };

			virtual ~RefControlBase() = default;

			virtual void DestroyObject() = 0;
		};

		template <typename T>
		struct RefControlBlock : RefControlBase
		{
			alignas(T) Byte storage_[sizeof(T)];

			T* GetPointer()
			{
				return reinterpret_cast<T*>(storage_);
			}

			void DestroyObject()override
			{
				GetPointer()->~T();
				alive_.store(false, std::memory_order_release);
			}
		};
	}

	template <typename T>
	class ResourceRef
	{
	public:
		constexpr ResourceRef()noexcept = default;

		constexpr ResourceRef(std::nullptr_t)noexcept
		{
			/// No Code
		}

		ResourceRef(const ResourceRef& other)noexcept: control_(other.control_), pointer_(other.pointer_), mode_(other.mode_)
		{
			AddRef();
		}

		ResourceRef(ResourceRef&& other)noexcept: control_(other.control_), pointer_(other.pointer_), mode_(other.mode_)
		{
			other.control_ = nullptr;
			other.pointer_ = nullptr;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourceRef(const ResourceRef<U>& other)noexcept: control_(other.control_), pointer_(other.pointer_), mode_(other.mode_)
		{
			AddRef();
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourceRef(ResourceRef<U>&& other)noexcept: control_(other.control_), pointer_(other.pointer_), mode_(other.mode_)
		{
			other.control_ = nullptr;
			other.pointer_ = nullptr;
		}

		~ResourceRef()
		{
			Release();
		}

		ResourceRef& operator=(const ResourceRef& other)noexcept
		{
			if (this != &other)
			{
				Release();
				control_ = other.control_;
				pointer_ = other.pointer_;
				mode_ = other.mode_;
				AddRef();
			}
			return *this;
		}

		ResourceRef& operator=(ResourceRef&& other)noexcept
		{
			if (this != &other)
			{
				Release();
				control_ = other.control_;
				pointer_ = other.pointer_;
				mode_ = other.mode_;
				other.control_ = nullptr;
				other.pointer_ = nullptr;
			}
			return *this;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourceRef& operator=(const ResourceRef<U>& other)noexcept
		{
			Release();
			control_ = other.control_;
			pointer_ = other.pointer_;
			mode_ = other.mode_;
			AddRef();
			return *this;
		}

		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourceRef& operator=(ResourceRef<U>&& other)noexcept
		{
			Release();
			control_ = other.control_;
			pointer_ = other.pointer_;
			mode_ = other.mode_;
			other.control_ = nullptr;
			other.pointer_ = nullptr;
			return *this;
		}

		ResourceRef& operator=(std::nullptr_t)noexcept
		{
			Release();
			return *this;
		}

		void reset()
		{
			Release();
		}

		T* get()const noexcept
		{
			return (control_ != nullptr && control_->alive_.load(std::memory_order_acquire)) ? pointer_ : nullptr;
		}

		T* operator->()const
		{
			SC_ASSERT(control_ != nullptr && control_->alive_.load(std::memory_order_acquire), "Dereferencing an empty or expired ResourceRef.");
			return pointer_;
		}

		T& operator*()const
		{
			SC_ASSERT(control_ != nullptr && control_->alive_.load(std::memory_order_acquire), "Dereferencing an empty or expired ResourceRef.");
			return *pointer_;
		}

		Uint32 use_count()const noexcept
		{
			return (control_ != nullptr) ? control_->strongCount_.load(std::memory_order_relaxed) : 0;
		}

		Bool observing()const noexcept
		{
			return mode_ == RefMode::Observing;
		}

		Bool exists()const noexcept
		{
			return control_ != nullptr && control_->alive_.load(std::memory_order_acquire);
		}

		explicit operator bool()const noexcept
		{
			return exists();
		}

		friend Bool operator==(const ResourceRef& lhs, const ResourceRef& rhs)noexcept
		{
			return lhs.control_ == rhs.control_;
		}

		friend Bool operator==(const ResourceRef& lhs, std::nullptr_t)noexcept
		{
			return !lhs.exists();
		}

	private:
		template <typename U>
		friend class ResourceRef;

		template <typename U, typename... Args>
		friend ResourceRef<U> MakeRef(Args&&... args);

		template <typename U>
		friend ResourceRef<U> MakeObserve(const ResourceRef<U>& owner)noexcept;

		ResourceRef(Detail::RefControlBase* control, T* pointer, RefMode mode)noexcept: control_(control), pointer_(pointer), mode_(mode)
		{
			/// No Code
		}

		void AddRef()noexcept
		{
			if (control_ == nullptr)
			{
				return;
			}

			if (mode_ == RefMode::Owning)
			{
				control_->strongCount_.fetch_add(1, std::memory_order_relaxed);
			}
			else
			{
				control_->observerCount_.fetch_add(1, std::memory_order_relaxed);
			}
		}

		void Release()
		{
			if (control_ == nullptr)
			{
				return;
			}

			if (mode_ == RefMode::Owning)
			{
				if (control_->strongCount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					control_->DestroyObject();
					if (control_->observerCount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
					{
						delete control_;
					}
				}
			}
			else
			{
				if (control_->observerCount_.fetch_sub(1, std::memory_order_acq_rel) == 1)
				{
					delete control_;
				}
			}

			control_ = nullptr;
			pointer_ = nullptr;
		}

		Detail::RefControlBase* control_ = nullptr;

		T* pointer_ = nullptr;

		RefMode mode_ = RefMode::Owning;
	};

	template <typename T, typename... Args>
	NODISCARD ResourceRef<T> MakeRef(Args&&... args)
	{
		Detail::RefControlBlock<T>* block = new Detail::RefControlBlock<T>();
		try
		{
			new (block->storage_) T(std::forward<Args>(args)...);
		}
		catch (...)
		{
			delete block;
			throw;
		}
		return ResourceRef<T>(block, block->GetPointer(), RefMode::Owning);
	}

	template <typename T>
	NODISCARD ResourceRef<T> MakeObserve(const ResourceRef<T>& owner)noexcept
	{
		ResourceRef<T> observer(owner.control_, owner.pointer_, RefMode::Observing);
		observer.AddRef();
		return observer;
	}
}

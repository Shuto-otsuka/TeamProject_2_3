#pragma once

namespace SeedCore
{
	struct SEEDCORE_API DelegateHandle
	{
	public:
		DelegateHandle() = default;

	public:
		static DelegateHandle Generate();

		Bool Valid()const;

	public:
		friend Bool operator==(const DelegateHandle&, const DelegateHandle&) = default;

	private:
		Uint64 id_ = 0;
	};

	template <typename Signature>
	class SinglecastDelegate;

	template <typename Ret, typename... Args>
	class SinglecastDelegate<Ret(Args...)>
	{
	public:
		SinglecastDelegate() = default;

	public:
		template <typename Callable>
		void Bind(Callable&& callable)
		{
			function_ = std::forward<Callable>(callable);
		}

		void Unbind()
		{
			function_ = nullptr;
		}

		Bool Bound()const
		{
			return static_cast<Bool>(function_);
		}

		Ret Execute(Args... args)const
		{
			return function_(args...);
		}

		Ret ExecuteIfBound(Args... args)const
		{
			if (function_)
			{
				return function_(args...);
			}
			return Ret();
		}

	private:
		std::function<Ret(Args...)> function_;
	};

	template <typename... Args>
	class MulticastDelegate
	{
	public:
		MulticastDelegate() = default;

	public:
		template <typename Callable>
		DelegateHandle Bind(Callable&& callable)
		{
			DelegateHandle handle = DelegateHandle::Generate();
			listeners_.push_back(Listener{ handle, std::function<void(Args...)>(std::forward<Callable>(callable)) });
			return handle;
		}

		void Unbind(const DelegateHandle& handle)
		{
			for (Size index = 0; index < listeners_.size(); ++index)
			{
				if (listeners_[index].handle_ == handle)
				{
					listeners_.erase(listeners_.begin() + index);
					return;
				}
			}
		}

		void Clear()
		{
			listeners_.clear();
		}

		Bool Bound()const
		{
			return !listeners_.empty();
		}

		void Broadcast(Args... args)const
		{
			for (const Listener& listener : listeners_)
			{
				listener.function_(args...);
			}
		}

	private:
		struct Listener
		{
			DelegateHandle handle_;

			std::function<void(Args...)> function_;
		};

	private:
		DynamicArray<Listener> listeners_;
	};
}

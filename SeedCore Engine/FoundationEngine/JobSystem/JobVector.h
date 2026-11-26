#pragma once
#include <FoundationEngine/Utility/Keyword.h>
#include <FoundationEngine/Utility/Types.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <vector>

namespace SeedCore
{
	inline Uint64 ArrayNextCapacity(Uint64 array);

	template<typename T>
	struct IsPod :std::integral_constant<Bool, std::is_standard_layout<T>::value&& std::is_trivial<T>::value>
	{
		/// No Code
	};

	class SEEDCORE_API JobVectorBase
	{
	protected:
		void* begin_;

		void* end_;

		void* capacity_;

	protected:
		JobVectorBase(void* firstElement, Size size);

		void grow_pod(void* firstElement, Size minSizeInBytes, Size size);

	public:
		Size size_in_byte()const;

		Size capacity_in_byte()const;

		Bool empty()const;
	};

	template<typename T, Unsigned N>
	struct JobVectorStorage;

	template<typename T, typename = void>
	class JobVectorTemplateCommon :public JobVectorBase
	{
	private:
		template<typename, Unsigned>
		friend struct JobVectorStorage;

		template<typename X>
		struct AlignedUnionType
		{
			SC_CONSTEX Size maxSize = (sizeof(std::byte) > sizeof(X)) ? sizeof(std::byte) : sizeof(X);

			alignas(X) std::byte buffer_[maxSize];
		};

		typedef AlignedUnionType<T> U;

		U firstElement;

	protected:
		JobVectorTemplateCommon(Size size) :JobVectorBase(&firstElement, size)
		{
			/// No Code
		}

		void grow_pod(Size minSizeInBytes, Size size)
		{
			JobVectorBase::grow_pod(&firstElement, minSizeInBytes, size);
		}

		Bool is()const
		{
			return begin_ == static_cast<const void*>(&firstElement);
		}

		void reset()
		{
			begin_ = end_ = capacity_ = &firstElement;
		}

		void set_end(T* P)
		{
			this->end_ = P;
		}

	public:
		using size_type = Size;
		using difference_type = std::ptrdiff_t;
		using value_type = T;

		using Iterator = T*;
		using ConstIterator = const T*;

		using ReverseIterator = std::reverse_iterator<Iterator>;
		using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

		using reference = T&;
		using const_reference = const T&;

		using pointer = T*;
		using const_pointer = const T*;

		inline Iterator begin()
		{
			return (Iterator)this->begin_;
		}

		inline ConstIterator begin()const
		{
			return (ConstIterator)this->begin_;
		}

		inline Iterator end()
		{
			return (Iterator)this->end_;
		}

		inline ConstIterator end()const
		{
			return (ConstIterator)this->end_;
		}

	protected:
		Iterator capacity_ptr()
		{
			return (Iterator)this->capacity_;
		}

		ConstIterator capacity_ptr()const
		{
			return (ConstIterator)this->capacity_;
		}

	public:
		ReverseIterator reverse_begin()
		{
			return ReverseIterator(end());
		}

		ConstReverseIterator reverse_begin()const
		{
			return ConstReverseIterator(end());
		}

		ReverseIterator reverse_end()
		{
			return ReverseIterator(begin());
		}

		ConstReverseIterator reverse_end()const
		{
			return ConstReverseIterator(begin());
		}

		inline size_type size()const
		{
			return end() - begin();
		}

		inline size_type max_size()const
		{
			return size_type(-1) / sizeof(T);
		}

		Size capacity()const
		{
			return capacity_ptr() - begin();
		}

		pointer data()
		{
			return pointer(begin());
		}

		const_pointer data()const
		{
			return const_pointer(begin());
		}

		inline reference operator[](size_type index)
		{
			return begin()[index];
		}

		inline const_reference operator[](size_type index)const
		{
			return begin()[index];
		}

		reference front()
		{
			return begin()[0];
		}

		const_reference front()const
		{
			return begin()[0];
		}

		reference back()
		{
			return end()[-1];
		}

		const_reference back()const
		{
			return end()[-1];
		}
	};

	template<typename T, Bool IsPodLike>
	class JobVectorTemplateBase :public JobVectorTemplateCommon<T>
	{
	protected:
		JobVectorTemplateBase(Size size) :JobVectorTemplateCommon<T>(size)
		{
			/// No Code
		}

		static void destroy_range(T* storage, T* element)
		{
			while (storage != element)
			{
				--element;
				element->~T();
			}
		}

		template<typename A, typename B>
		void uninitialized_move(A a, A element, B destination)
		{
			std::uninitialized_copy(std::make_move_iterator(a), std::make_move_iterator(element), destination);
		}

		template<typename A, typename B>
		void uninitialized_copy(A a, A element, B destination)
		{
			std::uninitialized_copy(a, element, destination);
		}

		void grow(Size minSize = 0)
		{
			Size currentCapacity = this->capacity();
			Size currentSize = this->size();
			Size newCapacity = Size(ArrayNextCapacity(currentCapacity + 2));
			if (newCapacity < minSize)
			{
				newCapacity = minSize;
			}

			T* newElement = static_cast<T*>(std::malloc(newCapacity * sizeof(T)));

			this->uninitialized_move(this->begin(), this->end(), newElement);

			destroy_range(this->begin(), this->end());

			if (!this->is())
			{
				std::free(this->begin());
			}

			this->set_end(newElement + currentSize);
			this->begin_ = newElement;
			this->capacity_ = this->begin() + newCapacity;
		}

	public:
		void push_back(const T& element)
		{
			if ((this->end_ >= this->capacity_)) [[unlikely]]
			{
				this->grow();
			}

			::new((void*)this->end()) T(element);
			this->set_end(this->end() + 1);
		}

		void push_back(T&& element)
		{
			if ((this->end_ >= this->capacity_)) [[unlikely]]
			{
				this->grow();
			}

			::new((void*)this->end()) T(::std::move(element));
			this->set_end(this->end() + 1);
		}

		void pop_back()
		{
			this->set_end(this->end() - 1);
			this->end()->~T();
		}
	};

	template<typename T>
	class JobVectorTemplateBase<T, true> :public JobVectorTemplateCommon<T>
	{
	protected:
		JobVectorTemplateBase(Size size) :JobVectorTemplateCommon<T>(size)
		{
			/// No Code
		}

		static void destroy_range(T*, T*)
		{
			/// No Code
		}

		template<typename A, typename B>
		static void uninitialized_move(A a, A element, B destination)
		{
			uninitialized_copy(a, element, destination);
		}

		template<typename A, typename B>
		static void uninitialized_copy(A a, A element, B destination)
		{
			std::uninitialized_copy(a, element, destination);
		}

		template<typename A, typename B>
		static void uninitialized_copy(A* a, A* element, B* destination, typename std::enable_if<std::is_same<typename std::remove_const<A>::type, B>::value>::type* = nullptr)
		{
			if (a != element)
			{
				std::memcpy(destination, a, (element - a) * sizeof(T));
			}
		}

		void grow(Size minSize = 0)
		{
			this->grow_pod(minSize * sizeof(T), sizeof(T));
		}

	public:
		void push_back(const T& element)
		{
			if ((this->end_ >= this->capacity_)) [[unlikely]]
			{
				this->grow();
			}

			memcpy(this->end(), &element, sizeof(T));
			this->set_end(this->end() + 1);
		}

		void pop_back()
		{
			this->set_end(this->end() - 1);
		}
	};

	template<typename T>
	class JobVectorImplementation :public JobVectorTemplateBase<T, IsPod<T>::value>
	{
	private:
		typedef JobVectorTemplateBase<T, IsPod<T>::value> SuperClass;

		JobVectorImplementation(const JobVectorImplementation&) = delete;

	public:
		typedef typename SuperClass::Iterator Iterator;
		typedef typename SuperClass::ConstIterator ConstIterator;
		typedef typename SuperClass::size_type size_type;

	protected:
		explicit JobVectorImplementation(Unsigned n) :JobVectorTemplateBase<T, IsPod<T>::value>(n * sizeof(T))
		{
			/// No Code
		}

	public:
		~JobVectorImplementation()
		{
			this->destroy_range(this->begin(), this->end());

			if (!this->is())
			{
				std::free(this->begin());
			}
		}

		void clear()
		{
			this->destroy_range(this->begin(), this->end());
			this->end_ = this->begin_;
		}

		void resize(size_type n)
		{
			if (n < this->size())
			{
				this->destroy_range(this->begin() + n, this->end());
				this->set_end(this->begin() + n);
			}
			else if (n > this->size())
			{
				if (this->capacity() < n)
				{
					this->grow(n);
				}

				for (auto index = this->end(), element = this->begin() + n;index != element;++index)
				{
					new(&*index)T();
				}
				this->set_end(this->begin() + n);
			}
		}

		void resize(size_type n, const T& nv)
		{
			if (n < this->size())
			{
				this->destroy_range(this->begin() + n, this->end());
				this->set_end(this->begin() + n);
			}
			else if (n > this->size())
			{
				if (this->capacity() < n)
				{
					this->grow(n);
				}

				std::uninitialized_fill(this->end(), this->begin() + n, nv);
				this->set_end(this->begin() + n);
			}
		}

		void reserve(size_type n)
		{
			if (this->capacity() < n)
			{
				this->grow(n);
			}
		}

		T pop_back_value()
		{
			T result = ::std::move(this->back());
			this->pop_back();
			return result;
		}

		void swap(JobVectorImplementation& rhs)
		{
			if (this == &rhs)
			{
				return;
			}

			if (!this->is() && !rhs.is())
			{
				std::swap(this->begin_, rhs.begin_);
				std::swap(this->end_, rhs.end_);
				std::swap(this->capacity_, rhs.capacity_);
				return;
			}

			if (rhs.size() > this->capacity())
			{
				this->grow(rhs.size());
			}

			if (this->size() > rhs.capacity())
			{
				rhs.grow(this->size());
			}

			Size numberShared = this->size();
			if (numberShared > rhs.size())
			{
				numberShared = rhs.size();
			}

			for (size_type index = 0;index != numberShared;++index)
			{
				std::swap((*this)[index], rhs[index]);
			}

			if (this->size() > rhs.size())
			{
				Size elementDefference = this->size() - rhs.size();
				this->uninitialized_copy(this->begin() + numberShared, this->end(), rhs.end());
				rhs.set_end(rhs.end() + elementDefference);
				this->destroy_range(this->begin() + numberShared, this->end());
				this->set_end(this->begin() + numberShared);
			}
			else if (rhs.size() > this->size())
			{
				Size elementDefference = rhs.size() - this->size();
				this->uninitialized_copy(rhs.begin() + numberShared, rhs.end(), this->end());
				this->set_end(this->end() + elementDefference);
				this->destroy_range(rhs.begin() + numberShared, rhs.end());
				rhs.set_end(rhs.begin() + numberShared);
			}
		}

		template<typename InIterator>
		void append(InIterator inStart, InIterator inEnd)
		{
			size_type numberInputs = std::distance(inStart, inEnd);

			if (numberInputs > size_type(this->capacity_ptr() - this->end()))
			{
				this->grow(this->size() + numberInputs);
			}
			this->uninitialized_copy(inStart, inEnd, this->end());
			this->set_end(this->end() + numberInputs);
		}

		void append(size_type numberInputs, const T& element)
		{
			if (numberInputs > SizeType(this->capacity_ptr() - this->end()))
			{
				this->grow(this->size() + numberInputs);
			}
			std::uninitialized_fill_n(this->end(), numberInputs, element);
			this->set_end(this->end() + numberInputs);
		}

		void append(std::initializer_list<T> initialList)
		{
			append(initialList.begin(), initialList.end());
		}

		void assign(size_type numberElments, const T& element)
		{
			clear();
			if (this->capacity() < numberElments)
			{
				this->grow(numberElments);
			}
			this->set_end(this->begin() + numberElments);
			std::uninitialized_fill(this->begin(), this->end(), element);
		}

		void assign(std::initializer_list<T> initialList)
		{
			clear();
			append(initialList);
		}

		Iterator erase(ConstIterator cIterator)
		{
			Iterator iterator = const_cast<Iterator>(cIterator);

			Iterator n = iterator;
			std::move(iterator + 1, this->end(), iterator);

			this->pop_back();
			return(n);
		}

		Iterator erase(ConstIterator cSize, ConstIterator cElement)
		{
			Iterator size = const_cast<Iterator>(cSize);
			Iterator element = const_cast<Iterator>(cElement);

			Iterator n = size;
			Iterator iterator = std::move(element, this->end(), size);

			this->destroy_range(iterator, this->end());
			this->set_end(iterator);
			return(n);
		}

		Iterator insert(Iterator iterator, T&& element)
		{
			if (iterator == this->end())
			{
				this->push_back(::std::move(element));
				return this->end() - 1;
			}

			if (this->end_ >= this->capacity_)
			{
				Size elementNo = iterator - this->begin();
				this->grow();
				iterator = this->begin() + elementNo;
			}

			::new((void*)this->end())T(::std::move(this->back()));
			std::move_backward(iterator, this->end() - 1, this->end());
			this->set_end(this->end() + 1);

			T* elementPtr = &element;
			if (iterator <= elementPtr && elementPtr < this->end_)
			{
				++elementPtr;
			}

			*iterator = std::move(*elementPtr);
			return iterator;
		}

		Iterator insert(Iterator iterator, const T& element)
		{
			if (iterator == this->end())
			{
				this->push_back(element);
				return this->end() - 1;
			}

			if (this->end_ >= this->capacity_)
			{
				Size elementNamber = iterator - this->begin();
				this->grow();
				iterator = this->begin() + elementNamber;
			}

			::new((void*)this->end())T(std::move(this->back()));
			std::move_backward(iterator, this->end() - 1, this->end());
			this->set_end(this->end() + 1);

			const T* elementPtr = &element;
			if (iterator <= elementPtr && elementPtr < this->end_)
			{
				++elementPtr;
			}

			*iterator = *elementPtr;
			return iterator;
		}

		Iterator insert(Iterator iterator, size_type numberInsert, const T& element)
		{
			Size insertElement = iterator - this->begin();

			if (iterator == this->end())
			{
				append(numberInsert, element);
				return this->begin() + insertElement;
			}

			reserve(this->size() + numberInsert);

			iterator = this->begin() + insertElement;

			if (Size(this->end() - iterator) >= numberInsert)
			{
				T* oldEnd = this->end();
				append(std::move_iterator<Iterator>(this->end() - numberInsert), std::move_iterator<Iterator>(this->end()));

				std::move_backward(iterator, oldEnd - numberInsert, oldEnd);

				std::fill_n(iterator, numberInsert, element);
				return iterator;
			}

			T* oldEnd = this->end();
			this->set_end(this->end() + numberInsert);
			Size numberOverwritten = oldEnd - iterator;
			this->uninitialized_move(iterator, oldEnd, this->end() - numberOverwritten);

			std::fill_n(iterator, numberOverwritten, element);

			std::uninitialized_fill_n(oldEnd, numberInsert - numberOverwritten, element);
			return iterator;
		}

		template<typename Ty>
		Iterator insert(Iterator iterator, Ty from, Ty to)
		{
			Size insertElement = iterator - this->begin();

			if (iterator == this->end())
			{
				append(from, to);
				return this->begin() + insertElement;
			}

			Size numberInsert = std::distance(from, to);

			reserve(this->size() + numberInsert);

			iterator = this->begin() + insertElement;

			if (Size(this->end() - iterator) >= numberInsert)
			{
				T* oldEnd = this->end();
				append(std::move_iterator<Iterator>(this->end() - numberInsert), std::move_iterator<Iterator>(this->end()));

				std::move_backward(iterator, oldEnd - numberInsert, oldEnd);

				std::copy(from, to, iterator);
				return iterator;
			}

			T* oldEnd = this->end();
			this->set_end(this->end() + numberInsert);
			Size numberOverwritten = oldEnd - iterator;
			this->uninitialized_move(iterator, oldEnd, this->end() - numberOverwritten);

			for (T* index = iterator;numberOverwritten > 0;--numberOverwritten)
			{
				*index = *from;
				++index;
				++from;
			}

			this->uninitialized_copy(from, to, oldEnd);
			return iterator;
		}

		void insert(Iterator iterator, std::initializer_list<T> initialList)
		{
			insert(iterator, initialList.begin(), initialList.end());
		}

		template<typename... Args>
		void emplace_back(Args&&... args)
		{
			if ((this->end_ >= this->capacity_)) [[unlikely]]
			{
				this->grow();
			}

			::new((void*)this->end())T(std::forward<Args>(args)...);
			this->set_end(this->end() + 1);
		}

		JobVectorImplementation<T>& operator=(const JobVectorImplementation& rhs)
		{
			if (this == &rhs)
			{
				return *this;
			}

			Size rhsSize = rhs.size();
			Size currentSize = this->size();
			if (currentSize >= rhsSize)
			{
				Iterator newEnd;
				if (rhsSize)
				{
					newEnd = std::copy(rhs.begin(), rhs.begin() + rhsSize, this->begin());
				}
				else
				{
					newEnd = this->begin();
				}

				this->destroy_range(newEnd, this->end());

				this->set_end(newEnd);
				return *this;
			}

			if (this->capacity() < rhsSize)
			{
				this->destroy_range(this->begin(), this->end());
				this->set_end(this->begin());
				currentSize = 0;
				this->grow(rhsSize);
			}
			else if (currentSize)
			{
				std::copy(rhs.begin(), rhs.begin() + currentSize, this->begin());
			}

			this->uninitialized_copy(rhs.begin() + currentSize, rhs.end(), this->begin() + currentSize);

			this->set_end(this->begin() + rhsSize);
			return *this;
		}

		JobVectorImplementation<T>& operator=(JobVectorImplementation&& rhs)
		{
			if (this == &rhs)
			{
				return *this;
			}

			if (!rhs.is())
			{
				this->destroy_range(this->begin(), this->end());
				if (!this->is())
				{
					std::free(this->begin());
				}
				this->begin_ = rhs.begin_;
				this->end_ = rhs.end_;
				this->capacity_ = rhs.capacity_;
				rhs.reset();
				return *this;
			}

			Size rhsSize = rhs.size();
			Size currentSize = this->size();
			if (currentSize >= rhsSize)
			{
				Iterator newEnd = this->begin();
				if (rhsSize)
				{
					newEnd = std::move(rhs.begin(), rhs.end(), newEnd);
				}
				this->destroy_range(newEnd, this->end());
				this->set_end(newEnd);

				rhs.clear();

				return *this;
			}

			if (this->capacity() < rhsSize)
			{
				this->destroy_range(this->begin(), this->end());
				this->set_end(this->begin());
				currentSize = 0;
				this->grow(rhsSize);
			}
			else if (currentSize)
			{
				std::move(rhs.begin(), rhs.begin() + currentSize, this->begin());
			}

			this->uninitialized_move(rhs.begin() + currentSize, rhs.end(), this->begin() + currentSize);

			this->set_end(this->begin() + rhsSize);

			rhs.clear();
			return *this;
		}

		Bool operator==(const JobVectorImplementation& rhs)const
		{
			if (this->size() != rhs.size())
			{
				return false;
			}
			return std::equal(this->begin(), this->end(), rhs.begin());
		}

		Bool operator!=(const JobVectorImplementation& rhs)const
		{
			return !(*this == rhs);
		}

		Bool operator<(const JobVectorImplementation& rhs)const
		{
			return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
		}

		void set_size(size_type n)
		{
			this->set_end(this->begin() + n);
		}
	};

	template<typename T, Unsigned N>
	struct JobVectorStorage
	{
		typename JobVectorTemplateCommon<T>::U InlineElements[N - 1];
	};

	template<typename T>
	struct JobVectorStorage<T, 1>
	{
		/// No Code
	};

	template<typename T>
	struct JobVectorStorage<T, 0>
	{
		/// No Code
	};

	template<typename T, Unsigned N = 2>
	class JobVector :public JobVectorImplementation<T>
	{
	private:
		JobVectorStorage<T, N> storage_;

	public:
		JobVector() :JobVectorImplementation<T>(N)
		{
			/// No Code
		}

		explicit JobVector(Size size, const T& value = T()) :JobVectorImplementation<T>(N)
		{
			this->assign(size, value);
		}

		template<typename Ty>
		JobVector(Ty storage, Ty element) : JobVectorImplementation<T>(N)
		{
			this->append(storage, element);
		}

		JobVector(std::initializer_list<T> initialList) :JobVectorImplementation<T>(N)
		{
			this->assign(initialList);
		}

		JobVector(const JobVector& rhs) :JobVectorImplementation<T>(N)
		{
			if (!rhs.empty())
			{
				JobVectorImplementation<T>::operator=(rhs);
			}
		}

		JobVector(JobVector&& rhs) :JobVectorImplementation<T>(N)
		{
			if (!rhs.empty())
			{
				JobVectorImplementation<T>::operator=(::std::move(rhs));
			}
		}

		const JobVector& operator=(const JobVector& rhs)
		{
			JobVectorImplementation<T>::operator=(rhs);
			return *this;
		}

		const JobVector& operator=(JobVector&& rhs)
		{
			JobVectorImplementation<T>::operator=(::std::move(rhs));
			return *this;
		}

		JobVector(JobVectorImplementation<T>&& rhs) :JobVectorImplementation<T>(N)
		{
			if (!rhs.empty())
			{
				JobVectorImplementation<T>::operator=(::std::move(rhs));
			}
		}

		const JobVector& operator=(JobVectorImplementation<T>&& rhs)
		{
			JobVectorImplementation<T>::operator=(::std::move(rhs));
			return *this;
		}

		const JobVector& operator=(std::initializer_list<T> initialList)
		{
			this->assign(initialList);
			return *this;
		}

		inline Size capacity_in_byte(const JobVector<T, N>& x)
		{
			return x.capacity_in_byte();
		}
	};
}

namespace std
{
	template<typename T>
	inline void swap(SeedCore::JobVectorImplementation<T>& lhs, SeedCore::JobVectorImplementation<T>& rhs)
	{
		lhs.swap(rhs);
	}

	template<typename T, SeedCore::Unsigned N>
	inline void swap(SeedCore::JobVector<T, N>& lhs, SeedCore::JobVector<T, N>& rhs)
	{
		lhs.swap(rhs);
	}
}

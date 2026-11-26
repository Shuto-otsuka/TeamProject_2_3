#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace SeedCore
{
	template<typename T>
	class DynamicArray
	{
	public:
		using value_type = T;

		using size_type = Size;

		using difference_type = std::ptrdiff_t;

		using reference = T&;

		using const_reference = const T&;

		using pointer = T*;

		using const_pointer = const T*;

		using iterator = T*;

		using const_iterator = const T*;

		using reverse_iterator = std::reverse_iterator<iterator>;

		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		DynamicArray() = default;

		explicit DynamicArray(Size count)
		{
			if (count > 0)
			{
				data_ = Allocate(count);
				capacity_ = count;
				std::uninitialized_default_construct_n(data_, count);
				size_ = count;
			}
		}

		DynamicArray(Size count, const T& value)
		{
			if (count > 0)
			{
				data_ = Allocate(count);
				capacity_ = count;
				std::uninitialized_fill_n(data_, count, value);
				size_ = count;
			}
		}

		template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
		DynamicArray(InputIt first, InputIt last)
		{
			AssignRange(first, last);
		}

		DynamicArray(std::initializer_list<T> values)
		{
			AssignRange(values.begin(), values.end());
		}

		template<typename Allocator>
		DynamicArray(const std::vector<T, Allocator>& source)
		{
			AssignRange(source.begin(), source.end());
		}

		DynamicArray(const DynamicArray& other)
		{
			AssignRange(other.data_, other.data_ + other.size_);
		}

		DynamicArray(DynamicArray&& other)noexcept :data_(other.data_), size_(other.size_), capacity_(other.capacity_)
		{
			other.data_ = nullptr;
			other.size_ = 0;
			other.capacity_ = 0;
		}

		~DynamicArray()
		{
			DestroyAll();
			Deallocate(data_);
		}

		DynamicArray& operator=(const DynamicArray& other)
		{
			if (this != &other)
			{
				assign(other.data_, other.data_ + other.size_);
			}
			return *this;
		}

		DynamicArray& operator=(DynamicArray&& other)noexcept
		{
			if (this != &other)
			{
				DestroyAll();
				Deallocate(data_);
				data_ = other.data_;
				size_ = other.size_;
				capacity_ = other.capacity_;
				other.data_ = nullptr;
				other.size_ = 0;
				other.capacity_ = 0;
			}
			return *this;
		}

		DynamicArray& operator=(std::initializer_list<T> values)
		{
			assign(values.begin(), values.end());
			return *this;
		}

		template<typename Allocator>
		DynamicArray& operator=(const std::vector<T, Allocator>& source)
		{
			assign(source.begin(), source.end());
			return *this;
		}

		void assign(Size count, const T& value)
		{
			clear();
			if (count > 0)
			{
				Reserve(count);
				std::uninitialized_fill_n(data_, count, value);
				size_ = count;
			}
		}

		template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
		void assign(InputIt first, InputIt last)
		{
			clear();
			for (; first != last; ++first)
			{
				push_back(*first);
			}
		}

		void assign(std::initializer_list<T> values)
		{
			assign(values.begin(), values.end());
		}

		reference operator[](Size index)
		{
			return data_[index];
		}

		const_reference operator[](Size index)const
		{
			return data_[index];
		}

		reference at(Size index)
		{
			if (index >= size_)
			{
				throw std::out_of_range("DynamicArray::at");
			}
			return data_[index];
		}

		const_reference at(Size index)const
		{
			if (index >= size_)
			{
				throw std::out_of_range("DynamicArray::at");
			}
			return data_[index];
		}

		reference front()
		{
			return data_[0];
		}

		const_reference front()const
		{
			return data_[0];
		}

		reference back()
		{
			return data_[size_ - 1];
		}

		const_reference back()const
		{
			return data_[size_ - 1];
		}

		pointer data()noexcept
		{
			return data_;
		}

		const_pointer data()const noexcept
		{
			return data_;
		}

		iterator begin()noexcept
		{
			return data_;
		}

		const_iterator begin()const noexcept
		{
			return data_;
		}

		const_iterator cbegin()const noexcept
		{
			return data_;
		}

		iterator end()noexcept
		{
			return data_ + size_;
		}

		const_iterator end()const noexcept
		{
			return data_ + size_;
		}

		const_iterator cend()const noexcept
		{
			return data_ + size_;
		}

		reverse_iterator rbegin()noexcept
		{
			return reverse_iterator(end());
		}

		const_reverse_iterator rbegin()const noexcept
		{
			return const_reverse_iterator(end());
		}

		reverse_iterator rend()noexcept
		{
			return reverse_iterator(begin());
		}

		const_reverse_iterator rend()const noexcept
		{
			return const_reverse_iterator(begin());
		}

		Bool empty()const noexcept
		{
			return size_ == 0;
		}

		Size size()const noexcept
		{
			return size_;
		}

		Size capacity()const noexcept
		{
			return capacity_;
		}

		Size max_size()const noexcept
		{
			return static_cast<Size>(-1) / sizeof(T);
		}

		void reserve(Size newCapacity)
		{
			if (newCapacity > capacity_)
			{
				Reallocate(newCapacity);
			}
		}

		void shrink_to_fit()
		{
			if (capacity_ > size_)
			{
				if (size_ == 0)
				{
					Deallocate(data_);
					data_ = nullptr;
					capacity_ = 0;
				}
				else
				{
					Reallocate(size_);
				}
			}
		}

		void clear()noexcept
		{
			DestroyAll();
			size_ = 0;
		}

		void push_back(const T& value)
		{
			EnsureOneMore();
			new (data_ + size_) T(value);
			++size_;
		}

		void push_back(T&& value)
		{
			EnsureOneMore();
			new (data_ + size_) T(std::move(value));
			++size_;
		}

		template<typename... Args>
		reference emplace_back(Args&&... args)
		{
			EnsureOneMore();
			new (data_ + size_) T(std::forward<Args>(args)...);
			++size_;
			return data_[size_ - 1];
		}

		void pop_back()
		{
			--size_;
			data_[size_].~T();
		}

		void resize(Size count)
		{
			if (count < size_)
			{
				std::destroy(data_ + count, data_ + size_);
			}
			else if (count > size_)
			{
				Reserve(count);
				std::uninitialized_default_construct_n(data_ + size_, count - size_);
			}
			size_ = count;
		}

		void resize(Size count, const T& value)
		{
			if (count < size_)
			{
				std::destroy(data_ + count, data_ + size_);
			}
			else if (count > size_)
			{
				Reserve(count);
				std::uninitialized_fill_n(data_ + size_, count - size_, value);
			}
			size_ = count;
		}

		iterator insert(const_iterator position, const T& value)
		{
			return EmplaceAt(position, value);
		}

		iterator insert(const_iterator position, T&& value)
		{
			return EmplaceAt(position, std::move(value));
		}

		iterator insert(const_iterator position, Size count, const T& value)
		{
			Size offset = static_cast<Size>(position - data_);
			if (count == 0)
			{
				return data_ + offset;
			}

			MakeGap(offset, count);
			for (Size index = 0; index < count; ++index)
			{
				new (data_ + offset + index) T(value);
			}
			size_ += count;
			return data_ + offset;
		}

		template<typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
		iterator insert(const_iterator position, InputIt first, InputIt last)
		{
			Size offset = static_cast<Size>(position - data_);
			Size count = static_cast<Size>(std::distance(first, last));
			if (count == 0)
			{
				return data_ + offset;
			}

			MakeGap(offset, count);
			Size index = 0;
			for (; first != last; ++first, ++index)
			{
				new (data_ + offset + index) T(*first);
			}
			size_ += count;
			return data_ + offset;
		}

		iterator insert(const_iterator position, std::initializer_list<T> values)
		{
			return insert(position, values.begin(), values.end());
		}

		template<typename... Args>
		iterator emplace(const_iterator position, Args&&... args)
		{
			return EmplaceAt(position, std::forward<Args>(args)...);
		}

		iterator erase(const_iterator position)
		{
			Size offset = static_cast<Size>(position - data_);
			std::move(data_ + offset + 1, data_ + size_, data_ + offset);
			--size_;
			data_[size_].~T();
			return data_ + offset;
		}

		iterator erase(const_iterator first, const_iterator last)
		{
			Size offset = static_cast<Size>(first - data_);
			Size count = static_cast<Size>(last - first);
			if (count == 0)
			{
				return data_ + offset;
			}
			std::move(data_ + offset + count, data_ + size_, data_ + offset);
			std::destroy(data_ + size_ - count, data_ + size_);
			size_ -= count;
			return data_ + offset;
		}

		void swap(DynamicArray& other)noexcept
		{
			std::swap(data_, other.data_);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
		}

		friend Bool operator==(const DynamicArray& lhs, const DynamicArray& rhs)
		{
			if (lhs.size_ != rhs.size_)
			{
				return false;
			}
			for (Size index = 0; index < lhs.size_; ++index)
			{
				if (!(lhs.data_[index] == rhs.data_[index]))
				{
					return false;
				}
			}
			return true;
		}

	private:
		static pointer Allocate(Size count)
		{
			return static_cast<pointer>(::operator new(count * sizeof(T), std::align_val_t{ alignof(T) }));
		}

		static void Deallocate(pointer block)noexcept
		{
			::operator delete(block, std::align_val_t{ alignof(T) });
		}

		void DestroyAll()noexcept
		{
			std::destroy(data_, data_ + size_);
		}

		Size GrowthTarget(Size required)const
		{
			Size grown = capacity_ + capacity_ / 2;
			if (grown < required)
			{
				grown = required;
			}
			if (grown < 1)
			{
				grown = 1;
			}
			return grown;
		}

		void EnsureOneMore()
		{
			if (size_ == capacity_)
			{
				Reallocate(GrowthTarget(size_ + 1));
			}
		}

		void Reserve(Size required)
		{
			if (required > capacity_)
			{
				Reallocate(GrowthTarget(required));
			}
		}

		void MakeGap(Size offset, Size count)
		{
			if (size_ + count > capacity_)
			{
				Reallocate(GrowthTarget(size_ + count));
			}

			for (Size index = size_; index > offset; --index)
			{
				new (data_ + index - 1 + count) T(std::move(data_[index - 1]));
				data_[index - 1].~T();
			}
		}

		template<typename... Args>
		iterator EmplaceAt(const_iterator position, Args&&... args)
		{
			Size offset = static_cast<Size>(position - data_);
			MakeGap(offset, 1);
			new (data_ + offset) T(std::forward<Args>(args)...);
			++size_;
			return data_ + offset;
		}

		static void Relocate(pointer destination, pointer source, Size count)
		{
			if constexpr (std::is_trivially_copyable_v<T>)
			{
				std::memcpy(destination, source, count * sizeof(T));
			}
			else
			{
				for (Size index = 0; index < count; ++index)
				{
					new (destination + index) T(std::move_if_noexcept(source[index]));
					source[index].~T();
				}
			}
		}

		void Reallocate(Size newCapacity)
		{
			pointer block = Allocate(newCapacity);
			Relocate(block, data_, size_);
			Deallocate(data_);
			data_ = block;
			capacity_ = newCapacity;
		}

		template<typename InputIt>
		void AssignRange(InputIt first, InputIt last)
		{
			if constexpr (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>)
			{
				reserve(static_cast<Size>(std::distance(first, last)));
			}
			for (; first != last; ++first)
			{
				push_back(*first);
			}
		}

		T* data_ = nullptr;

		Size size_ = 0;

		Size capacity_ = 0;
	};

	template<typename T>
	void swap(DynamicArray<T>& lhs, DynamicArray<T>& rhs)noexcept
	{
		lhs.swap(rhs);
	}

	template<typename T, typename U>
	Size erase(DynamicArray<T>& container, const U& value)
	{
		Size before = container.size();
		auto newEnd = std::remove(container.begin(), container.end(), value);
		container.erase(newEnd, container.end());
		return before - container.size();
	}

	template<typename T, typename Predicate>
	Size erase_if(DynamicArray<T>& container, Predicate predicate)
	{
		Size before = container.size();
		auto newEnd = std::remove_if(container.begin(), container.end(), predicate);
		container.erase(newEnd, container.end());
		return before - container.size();
	}
}

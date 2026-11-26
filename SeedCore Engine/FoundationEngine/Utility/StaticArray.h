#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace SeedCore
{
	template<typename T, Size N>
	struct StaticArray
	{
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

		T data_[N];

		constexpr reference operator[](Size index)
		{
			return data_[index];
		}

		constexpr const_reference operator[](Size index)const
		{
			return data_[index];
		}

		constexpr reference at(Size index)
		{
			if (index >= N)
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data_[index];
		}

		constexpr const_reference at(Size index)const
		{
			if (index >= N)
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data_[index];
		}

		constexpr reference front()
		{
			return data_[0];
		}

		constexpr const_reference front()const
		{
			return data_[0];
		}

		constexpr reference back()
		{
			return data_[N - 1];
		}

		constexpr const_reference back()const
		{
			return data_[N - 1];
		}

		constexpr pointer data()noexcept
		{
			return data_;
		}

		constexpr const_pointer data()const noexcept
		{
			return data_;
		}

		constexpr iterator begin()noexcept
		{
			return data_;
		}

		constexpr const_iterator begin()const noexcept
		{
			return data_;
		}

		constexpr const_iterator cbegin()const noexcept
		{
			return data_;
		}

		constexpr iterator end()noexcept
		{
			return data_ + N;
		}

		constexpr const_iterator end()const noexcept
		{
			return data_ + N;
		}

		constexpr const_iterator cend()const noexcept
		{
			return data_ + N;
		}

		constexpr reverse_iterator rbegin()noexcept
		{
			return reverse_iterator(end());
		}

		constexpr const_reverse_iterator rbegin()const noexcept
		{
			return const_reverse_iterator(end());
		}

		constexpr reverse_iterator rend()noexcept
		{
			return reverse_iterator(begin());
		}

		constexpr const_reverse_iterator rend()const noexcept
		{
			return const_reverse_iterator(begin());
		}

		constexpr Bool empty()const noexcept
		{
			return N == 0;
		}

		constexpr Size size()const noexcept
		{
			return N;
		}

		constexpr Size max_size()const noexcept
		{
			return N;
		}

		constexpr void fill(const T& value)
		{
			for (Size index = 0; index < N; ++index)
			{
				data_[index] = value;
			}
		}

		constexpr void swap(StaticArray& other)noexcept
		{
			for (Size index = 0; index < N; ++index)
			{
				std::swap(data_[index], other.data_[index]);
			}
		}

		friend constexpr Bool operator==(const StaticArray& lhs, const StaticArray& rhs)
		{
			for (Size index = 0; index < N; ++index)
			{
				if (!(lhs.data_[index] == rhs.data_[index]))
				{
					return false;
				}
			}
			return true;
		}
	};

	template<typename T>
	struct StaticArray<T, 0>
	{
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

		constexpr reference operator[](Size index)
		{
			return data()[index];
		}

		constexpr const_reference operator[](Size index)const
		{
			return data()[index];
		}

		constexpr reference at(Size index)
		{
			if (index >= size())
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data()[index];
		}

		constexpr const_reference at(Size index)const
		{
			if (index >= size())
			{
				throw std::out_of_range("StaticArray::at");
			}
			return data()[index];
		}

		constexpr reference front()
		{
			return data()[0];
		}

		constexpr const_reference front()const
		{
			return data()[0];
		}

		constexpr reference back()
		{
			return data()[0];
		}

		constexpr const_reference back()const
		{
			return data()[0];
		}

		constexpr pointer data()noexcept
		{
			return nullptr;
		}

		constexpr const_pointer data()const noexcept
		{
			return nullptr;
		}

		constexpr iterator begin()noexcept
		{
			return nullptr;
		}

		constexpr const_iterator begin()const noexcept
		{
			return nullptr;
		}

		constexpr const_iterator cbegin()const noexcept
		{
			return nullptr;
		}

		constexpr iterator end()noexcept
		{
			return nullptr;
		}

		constexpr const_iterator end()const noexcept
		{
			return nullptr;
		}

		constexpr const_iterator cend()const noexcept
		{
			return nullptr;
		}

		constexpr reverse_iterator rbegin()noexcept
		{
			return reverse_iterator(end());
		}

		constexpr const_reverse_iterator rbegin()const noexcept
		{
			return const_reverse_iterator(end());
		}

		constexpr reverse_iterator rend()noexcept
		{
			return reverse_iterator(begin());
		}

		constexpr const_reverse_iterator rend()const noexcept
		{
			return const_reverse_iterator(begin());
		}

		constexpr Bool empty()const noexcept
		{
			return true;
		}

		constexpr Size size()const noexcept
		{
			return 0;
		}

		constexpr Size max_size()const noexcept
		{
			return 0;
		}

		constexpr void fill(const T&)
		{
			/// No Code
		}

		constexpr void swap(StaticArray&)noexcept
		{
			/// No Code
		}

		friend constexpr Bool operator==(const StaticArray&, const StaticArray&)
		{
			return true;
		}
	};

	template<typename T, Size N>
	constexpr void swap(StaticArray<T, N>& lhs, StaticArray<T, N>& rhs)noexcept
	{
		lhs.swap(rhs);
	}
}

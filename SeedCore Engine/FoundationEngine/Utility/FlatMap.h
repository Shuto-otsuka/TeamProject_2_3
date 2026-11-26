#pragma once

namespace SeedCore
{
	template<typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
	class FlatMap
	{
	public:
		using value_type = std::pair<Key, Value>;
		using key_type = Key;
		using mapped_type = Value;
		using size_type = Size;
		using difference_type = std::ptrdiff_t;
		using hasher = Hash;
		using key_equal = KeyEqual;
		using reference = value_type&;
		using const_reference = const value_type&;

	private:
		enum class SlotState :Uint8
		{
			Empty = 0,

			Occupied = 1,

			Deleted = 2
		};

		struct Slot
		{
			SlotState state_ = SlotState::Empty;

			std::optional<value_type> keyValue_;
		};

		SC_CONSTEX size_type DefaultCapacity = 16;

		SC_CONSTEX Float MaxLoadFactor = 0.75f;

		DynamicArray<Slot> buckets_;

		size_type size_ = 0;

		size_type occupied_ = 0;

		Hash hasher_;

		KeyEqual keyEqual_;

		size_type bucket_index(const Key& key)const noexcept
		{
			return hasher_(key) & (buckets_.size() - 1);
		}

		std::pair<size_type, Bool> probe(const Key& key)const noexcept
		{
			const size_type capacity = buckets_.size();
			const size_type bucketIndex = bucket_index(key);
			size_type firstDeleted = capacity;

			size_type index = bucketIndex;
			for (size_type step = 0; step < capacity; ++step)
			{
				const auto& slot = buckets_[index];

				switch (slot.state_)
				{
				case SlotState::Occupied:
					if (keyEqual_(slot.keyValue_->first, key))
					{
						return { index,true };
					}
					break;
				case SlotState::Deleted:
					if (firstDeleted == capacity)
					{
						firstDeleted = index;
					}
					break;
				case SlotState::Empty:
					FALLTHROUGH;
				default:
					return { (firstDeleted != capacity) ? firstDeleted : index,false };
					break;
				}

				index = (index + 1) & (capacity - 1);
			}

			return { (firstDeleted != capacity) ? firstDeleted : bucketIndex,false };
		}

		void rehash(size_type newCapacity)
		{
			if (newCapacity < DefaultCapacity)
			{
				newCapacity = DefaultCapacity;
			}

			DynamicArray<Slot> oldBackets(newCapacity);
			std::swap(buckets_, oldBackets);
			size_ = 0;
			occupied_ = 0;

			for (auto& slot : oldBackets)
			{
				if (slot.state_ == SlotState::Occupied)
				{
					insert_implementation(std::move(*slot.keyValue_));
				}
			}
		}

		std::pair<size_type, Bool>insert_implementation(value_type keyValue)
		{
			auto [index, found] = probe(keyValue.first);
			if (found)
			{
				return{ index, false };
			}

			if (buckets_[index].state_ != SlotState::Deleted)
			{
				++occupied_;
			}

			buckets_[index].state_ = SlotState::Occupied;
			buckets_[index].keyValue_ = std::move(keyValue);
			++size_;
			return { index,true };
		}

		void ensure_capacity()
		{
			if (buckets_.size() == 0)
			{
				rehash(DefaultCapacity);
				return;
			}

			if (static_cast<Float>(occupied_ + 1) > MaxLoadFactor * static_cast<Float>(buckets_.size()))
			{
				rehash(buckets_.size() * 2);
			}
		}

	public:
		struct ConstIterator;

		struct Iterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = FlatMap::value_type;
			using difference_type = FlatMap::difference_type;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			using SlotVector = DynamicArray<Slot>;

			SlotVector* slots_ = nullptr;

			size_type index_ = 0;

			void skip()
			{
				while (index_ < slots_->size() && (*slots_)[index_].state_ != SlotState::Occupied)
				{
					++index_;
				}
			}

		public:
			Iterator() = default;

			explicit Iterator(SlotVector* slots, size_type index) :slots_(slots), index_(index)
			{
				skip();
			}

			reference operator*()const
			{
				return *(*slots_)[index_].keyValue_;
			}

			pointer operator->()const
			{
				return &*(*slots_)[index_].keyValue_;
			}

			Iterator& operator++()
			{
				++index_;
				skip();
				return *this;
			}

			Iterator operator++(Int)
			{
				auto temporary = *this;
				++(*this);
				return temporary;
			}

			Bool operator==(const Iterator& other)const noexcept
			{
				return index_ == other.index_;
			}

			Bool operator!=(const Iterator& other)const noexcept
			{
				return !(*this == other);
			}

			explicit operator Bool()const noexcept
			{
				return index_ < slots_->size();
			}

			operator ConstIterator()const
			{
				return ConstIterator(slots_, index_);
			}
		};

		struct ConstIterator
		{
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type = FlatMap::value_type;
			using difference_type = FlatMap::difference_type;
			using pointer = const value_type*;
			using reference = const value_type&;

		private:
			using SlotVector = const DynamicArray<Slot>;

			SlotVector* slots_ = nullptr;

			size_type index_ = 0;

			void skip()
			{
				while (index_ < slots_->size() && (*slots_)[index_].state_ != SlotState::Occupied)
				{
					++index_;
				}
			}

		public:
			ConstIterator() = default;

			explicit ConstIterator(SlotVector* slots, size_type index) :slots_(slots), index_(index)
			{
				skip();
			}

			reference operator*()const
			{
				return *(*slots_)[index_].keyValue_;
			}

			pointer operator->()const
			{
				return &*(*slots_)[index_].keyValue_;
			}

			ConstIterator& operator++()
			{
				++index_;
				skip();
				return *this;
			}

			ConstIterator operator++(Int)
			{
				auto temporary = *this;
				++(*this);
				return temporary;
			}

			Bool operator==(const ConstIterator& other)const noexcept
			{
				return index_ == other.index_;
			}

			Bool operator!=(const ConstIterator& other)const noexcept
			{
				return !(*this == other);
			}

			explicit operator Bool()const noexcept
			{
				return index_ < slots_->size();
			}
		};

	public:
		FlatMap() :buckets_(DefaultCapacity)
		{
			/// No Code
		}

		FlatMap(std::initializer_list<value_type> init) :buckets_(std::bit_ceil(Max(DefaultCapacity, static_cast<size_type>(init.size() * 2))))
		{
			for (auto& keyValue : init)
			{
				insert(keyValue);
			}
		}

		template<std::ranges::input_range R>
			requires std::convertible_to<std::ranges::range_value_t<R>, value_type>
		explicit FlatMap(R&& range) :buckets_(DefaultCapacity)
		{
			for (auto&& keyValue : range)
			{
				insert(std::forward<decltype(keyValue)>(keyValue));
			}
		}

		FlatMap(const FlatMap&) = default;
		FlatMap(FlatMap&&) = default;
		FlatMap& operator=(const FlatMap&) = default;
		FlatMap& operator=(FlatMap&&) = default;

		~FlatMap() = default;

		Iterator begin()
		{
			return Iterator(&buckets_, 0);
		}

		Iterator end()
		{
			return Iterator(&buckets_, buckets_.size());
		}

		ConstIterator begin()const
		{
			return ConstIterator(&buckets_, 0);
		}

		ConstIterator end()const
		{
			return ConstIterator(&buckets_, buckets_.size());
		}

		ConstIterator cbegin()const
		{
			return begin();
		}

		ConstIterator cend()const
		{
			return end();
		}

		size_type size()const noexcept
		{
			return size_;
		}

		Bool empty()const noexcept
		{
			return size_ == 0;
		}

		size_type capacity()const noexcept
		{
			return buckets_.size();
		}

		void reserve(size_type n)
		{
			size_type required = static_cast<size_type>(static_cast<Float>(n) / MaxLoadFactor) + 1;
			size_type capacity = DefaultCapacity;

			while (capacity < required)
			{
				capacity *= 2;
			}

			if (capacity > buckets_.size())
			{
				rehash(capacity);
			}
		}

		std::pair<Iterator, Bool> insert(const value_type& keyValue)
		{
			ensure_capacity();
			auto [index, inserted] = insert_implementation(keyValue);
			return { Iterator(&buckets_,index),inserted };
		}

		std::pair<Iterator, Bool> insert(value_type&& keyValue)
		{
			ensure_capacity();
			auto [index, inserted] = insert_implementation(std::move(keyValue));
			return { Iterator(&buckets_,index),inserted };
		}

		template<typename... Args>
		std::pair<Iterator, Bool> emplace(Args&&... args)
		{
			return insert(value_type(std::forward<Args>(args)...));
		}

		template<std::ranges::input_range R>
			requires std::convertible_to<std::ranges::range_value_t<R>, value_type>
		void insert_range(R&& range)
		{
			for (auto&& keyValue : range)
			{
				insert(std::forward<decltype(keyValue)>(keyValue));
			}
		}

		Value& operator[](const Key& key)
		{
			ensure_capacity();
			auto [index, found] = probe(key);
			if (!found)
			{
				if (buckets_[index].state_ != SlotState::Deleted)
				{
					++occupied_;
				}
				buckets_[index].state_ = SlotState::Occupied;
				buckets_[index].keyValue_ = value_type(key, Value{});
				++size_;
			}
			return buckets_[index].keyValue_->second;
		}

		Value& operator[](const Key&& key)
		{
			ensure_capacity();
			auto [index, found] = probe(key);
			if (!found)
			{
				if (buckets_[index].state_ != SlotState::Deleted)
				{
					++occupied_;
				}
				buckets_[index].state_ = SlotState::Occupied;
				buckets_[index].keyValue_ = value_type(std::move(key), Value{});
				++size_;
			}
			return buckets_[index].keyValue_->second;
		}

		Value& at(const Key& key)
		{
			auto [index, found] = probe(key);
			if (!found)
			{
				throw std::out_of_range("FlatMap::at: key not found");
			}
			return buckets_[index].keyValue_->second;
		}

		const Value& at(const Key& key)const
		{
			auto [index, found] = probe(key);
			if (!found)
			{
				throw std::out_of_range("FlatMap::at: key not found");
			}
			return buckets_[index].keyValue_->second;
		}

		Bool erase(const Key& key)
		{
			auto [index, found] = probe(key);
			if (!found)
			{
				return false;
			}

			buckets_[index].state_ = SlotState::Deleted;
			buckets_[index].keyValue_.reset();
			--size_;
			return true;
		}

		Iterator erase(Iterator iterator)
		{
			const Key& key = iterator->first;
			erase(key);
			++iterator;
			return iterator;
		}

		void clear()noexcept
		{
			for (auto& slot : buckets_)
			{
				slot.state_ = SlotState::Empty;
				slot.keyValue_.reset();
			}
			size_ = 0;
			occupied_ = 0;
		}

		Iterator find(const Key& key)
		{
			auto [index, found] = probe(key);
			if (!found)
			{
				return end();
			}
			return Iterator(&buckets_, index);
		}

		ConstIterator find(const Key& key)const
		{
			auto [index, found] = probe(key);
			if (!found)
			{
				return end();
			}
			return ConstIterator(&buckets_, index);
		}

		Bool contains(const Key& key)const noexcept
		{
			auto [index, found] = probe(key);
			return found;
		}

		size_type count(const Key& key)const noexcept
		{
			return contains(key) ? 1 : 0;
		}

		Float load_factor()const noexcept
		{
			return static_cast<Float>(occupied_) / static_cast<Float>(buckets_.size());
		}
	};
}
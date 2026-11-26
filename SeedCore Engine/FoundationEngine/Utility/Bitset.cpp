#include <FoundationEngine/Utility/Bitset.h>
#include <FoundationEngine/Math/Algorithm.h>

#include <algorithm>
#include <bit>

namespace SeedCore
{
	Bitset::Bitset(Size bitCount)
	{
		resize(bitCount);
	}

	void Bitset::resize(Size bitCount, Bool defaultValue)
	{
		const Size oldBitCount = bitCount_;
		const Size oldBlockCount = data_.size();

		bitCount_ = bitCount;

		const Size newBlockCount = get_required_block_count(bitCount_);

		data_.resize(newBlockCount);

		if (newBlockCount > oldBlockCount)
		{
			const Block fillValue = defaultValue ? ~0ULL : 0ULL;

			std::fill(data_.begin() + oldBlockCount, data_.end(), fillValue);
		}

		const Size oldRemainBits = oldBitCount % BitsPerBlock;
		if (defaultValue && bitCount_ > oldBitCount && oldRemainBits != 0)
		{
			data_[oldBlockCount - 1] |= (~0ULL << oldRemainBits);
		}

		clear_unused_bits();
	}

	void Bitset::reserve(Size bitCount)
	{
		data_.reserve(get_required_block_count(bitCount));
	}

	void Bitset::clear()
	{
		std::ranges::fill(data_, 0ULL);
	}

	void Bitset::fill()
	{
		std::ranges::fill(data_, ~0ULL);

		clear_unused_bits();
	}

	void Bitset::set(Size index, Bool value)
	{
		ensure_size(index);

		const Size blockIndex = index / BitsPerBlock;
		const Size bitIndex = index % BitsPerBlock;

		if (value)
		{
			data_[blockIndex] |= (1ULL << bitIndex);
		}
		else
		{
			data_[blockIndex] &= ~(1ULL << bitIndex);
		}
	}

	void Bitset::reset(Size index)
	{
		set(index, false);
	}

	void Bitset::flip(Size index)
	{
		ensure_size(index);

		const Size blockIndex = index / BitsPerBlock;
		const Size bitIndex = index % BitsPerBlock;

		data_[blockIndex] ^= (1ULL << bitIndex);
	}

	Bool Bitset::test(Size index)const
	{
		if (index >= bitCount_) [[unlikely]]
		{
			return false;
		}

		const Size blockIndex = index / BitsPerBlock;
		const Size bitIndex = index % BitsPerBlock;

		return (data_[blockIndex] & (1ULL << bitIndex)) != 0;
	}

	Bool Bitset::any()const
	{
		for (Block block : data_)
		{
			if (block != 0)
			{
				return true;
			}
		}

		return false;
	}

	Bool Bitset::none()const
	{
		return !any();
	}

	Bool Bitset::all()const
	{
		if (bitCount_ == 0)
		{
			return true;
		}

		const Size fullBlocks = bitCount_ / BitsPerBlock;

		for (Size index = 0; index < fullBlocks; ++index)
		{
			if (data_[index] != ~0ULL)
			{
				return false;
			}
		}

		const Size remainBits = bitCount_ % BitsPerBlock;

		if (remainBits == 0)
		{
			return true;
		}

		const Uint64 mask = (1ULL << remainBits) - 1ULL;

		return (data_[fullBlocks] & mask) == mask;
	}

	Size Bitset::count()const
	{
		Size count = 0;

		for (Block block : data_)
		{
			count += std::popcount(block);
		}

		return count;
	}

	Size Bitset::find_first_set()const
	{
		for (Size index = 0; index < data_.size(); ++index)
		{
			Block block = data_[index];

			if (block != 0) [[unlikely]]
			{
				return index * BitsPerBlock + std::countr_zero(block);
			}
		}

		return InvalidIndex;
	}

	Size Bitset::find_next_set(Size startIndex)const
	{
		if (startIndex >= bitCount_) [[unlikely]]
		{
			return InvalidIndex;
		}

		Size blockIndex = startIndex / BitsPerBlock;
		Size bitIndex = startIndex % BitsPerBlock;

		Block block = data_[blockIndex];

		block &= (~0ULL << bitIndex);

		while (true)
		{
#if defined(__cpp_assume)
			[[assume(true)]];
#else
			__assume(true);
#endif

			if (block != 0) [[unlikely]]
			{
				return blockIndex * BitsPerBlock + std::countr_zero(block);
			}

			if (++blockIndex >= data_.size()) [[unlikely]]
			{
				break;
			}

			block = data_[blockIndex];
		}

		return InvalidIndex;
	}

	Size Bitset::size()const
	{
		return bitCount_;
	}

	Bool Bitset::empty()const
	{
		return bitCount_ == 0;
	}

	const Bitset::Block* Bitset::data()const
	{
		return data_.data();
	}

	Bitset::Block* Bitset::data()
	{
		return data_.data();
	}

	Size Bitset::block_count()const
	{
		return data_.size();
	}

	Bool Bitset::operator[](Size index)const
	{
		return test(index);
	}

	Bitset& Bitset::operator&=(const Bitset& other)
	{
		const Size minBlocks = Min(data_.size(), other.data_.size());

		for (Size index = 0; index < minBlocks; ++index)
		{
			data_[index] &= other.data_[index];
		}

		for (Size index = minBlocks; index < data_.size(); ++index)
		{
			data_[index] = 0;
		}

		return *this;
	}

	Bitset& Bitset::operator|=(const Bitset& other)
	{
		if (other.bitCount_ > bitCount_)
		{
			resize(other.bitCount_);
		}

		for (Size index = 0; index < other.data_.size(); ++index)
		{
			data_[index] |= other.data_[index];
		}

		return *this;
	}

	Bitset& Bitset::operator^=(const Bitset& other)
	{
		if (other.bitCount_ > bitCount_)
		{
			resize(other.bitCount_);
		}

		for (Size index = 0; index < other.data_.size(); ++index)
		{
			data_[index] ^= other.data_[index];
		}

		return *this;
	}

	void Bitset::ensure_size(Size bitIndex)
	{
		if (bitIndex < bitCount_)
		{
			return;
		}

		resize(bitIndex + 1);
	}

	Size Bitset::get_required_block_count(Size bitCount)
	{
		return (bitCount + BitsPerBlock - 1) / BitsPerBlock;
	}

	void Bitset::clear_unused_bits()
	{
		const Size remainBits = bitCount_ % BitsPerBlock;

		if (remainBits == 0 || data_.empty())
		{
			return;
		}

		const Uint64 mask = (1ULL << remainBits) - 1ULL;

		data_.back() &= mask;
	}
}
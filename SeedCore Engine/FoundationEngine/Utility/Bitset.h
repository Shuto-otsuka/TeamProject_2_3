#pragma once

namespace SeedCore
{
	class SEEDCORE_API Bitset
	{
	public:
		using Block = Uint64;

		SC_CONSTEX Size BitsPerBlock = sizeof(Block) * 8;

	public:
		Bitset() = default;

		explicit Bitset(Size bitCount);

	public:
		void resize(Size bitCount, Bool defaultValue = false);

		void reserve(Size bitCount);

		void clear();

		void fill();

	public:
		void set(Size index, Bool value = true);

		void reset(Size index);

		void flip(Size index);

		Bool test(Size index)const;

	public:
		Bool any()const;

		Bool none()const;

		Bool all()const;

	public:
		Size count()const;

	public:
		Size find_first_set()const;

		Size find_next_set(Size startIndex)const;

	public:
		Size size()const;

		Bool empty()const;

	public:
		const Block* data()const;

		Block* data();

		Size block_count()const;

	public:
		Bool operator[](Size index)const;

		Bitset& operator&=(const Bitset& other);

		Bitset& operator|=(const Bitset& other);

		Bitset& operator^=(const Bitset& other);

	public:
		friend Bitset operator&(Bitset lhs, const Bitset& rhs)
		{
			lhs &= rhs;
			return lhs;
		}

		friend Bitset operator|(Bitset lhs, const Bitset& rhs)
		{
			lhs |= rhs;
			return lhs;
		}

		friend Bitset operator^(Bitset lhs, const Bitset& rhs)
		{
			lhs ^= rhs;
			return lhs;
		}

		friend Bool operator==(const Bitset& lhs, const Bitset& rhs)
		{
			Size maxBlocks = (lhs.data_.size() > rhs.data_.size()) ? lhs.data_.size() : rhs.data_.size();
			for (Size index = 0; index < maxBlocks; ++index)
			{
				Block a = (index < lhs.data_.size()) ? lhs.data_[index] : 0;
				Block b = (index < rhs.data_.size()) ? rhs.data_[index] : 0;
				if (a != b)
				{
					return false;
				}
			}
			return true;
		}

		friend Bool operator!=(const Bitset& lhs, const Bitset& rhs)
		{
			return !(lhs == rhs);
		}

	public:
		SC_CONSTEX Size InvalidIndex = static_cast<Size>(-1);

	private:
		void ensure_size(Size bitIndex);

		static Size get_required_block_count(Size bitCount);

		void clear_unused_bits();

	private:
		DynamicArray<Block> data_;

		Size bitCount_ = 0;
	};
}
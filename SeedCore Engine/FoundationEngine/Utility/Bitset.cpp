#include <FoundationEngine/Utility/Bitset.h>
#include <FoundationEngine/Math/Algorithm.h>

#include <algorithm>
#include <bit>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs with room for at least bitCount bits, all initialized to 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 少なくとも bitCount ビット分の領域を持って構築する。全て0で初期化される。
	*/
	Bitset::Bitset(Size bitCount)
	{
		resize(bitCount);
	}

	/**
	* [EN]
	* Resizes to bitCount bits. Every newly added bit is set to
	* defaultValue, including those that fall into the old last block;
	* shrinking discards the trailing bits. Bits past the logical size
	* are always kept at 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bitCount ビットにリサイズする。新しく追加されたビットは、元の最後の
	* ブロックに入るものも含めて全て defaultValue にする。縮小では末尾の
	* ビットを捨てる。論理サイズより後ろのビットは常に 0 に保つ。
	*/
	void Bitset::resize(Size bitCount, Bool defaultValue)
	{
		/// [EN] The old size and block count mark where the newly added bits and blocks begin.
		/// [JP] 元の大きさとブロック数が、新しく足したビットとブロックの始まる位置になる。
		const Size oldBitCount = bitCount_;
		const Size oldBlockCount = data_.size();

		bitCount_ = bitCount;

		const Size newBlockCount = get_required_block_count(bitCount_);

		data_.resize(newBlockCount);

		/// [EN] Only the newly-added blocks need filling; existing blocks already hold their previous bits.
		/// [JP] 新しく追加されたブロックのみを埋めればよい。既存のブロックは以前のビットを保持したままである。
		if (newBlockCount > oldBlockCount)
		{
			const Block fillValue = defaultValue ? ~0ULL : 0ULL;

			std::fill(data_.begin() + oldBlockCount, data_.end(), fillValue);
		}

		/// [EN] New bits that land in the old last block are set too; the bits there were 0, since unused bits are always kept cleared.
		/// [JP] 元の最後のブロックに入る新しいビットも立てる。使われないビットは常に 0 にしてあるので、そこは 0 になっている。
		const Size oldRemainBits = oldBitCount % BitsPerBlock;
		if (defaultValue && bitCount_ > oldBitCount && oldRemainBits != 0)
		{
			data_[oldBlockCount - 1] |= (~0ULL << oldRemainBits);
		}

		/// [EN] Bits past the new logical size are cleared, whether they came from filling with 1s or were left over by shrinking.
		/// [JP] 新しい論理サイズより後ろのビットは消す。1 で埋めた余りでも、縮小で残ったものでも同じ。
		clear_unused_bits();
	}

	/**
	* [EN]
	* Reserves backing storage for at least bitCount bits without
	* changing the current logical size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の論理サイズを変えずに、少なくとも bitCount ビット分の
	* 裏付けストレージを予約する。
	*/
	void Bitset::reserve(Size bitCount)
	{
		/// [EN] Only the block array's capacity changes; bitCount_ stays as it is.
		/// [JP] 変わるのはブロック配列の容量だけで、bitCount_ はそのまま。
		data_.reserve(get_required_block_count(bitCount));
	}

	/**
	* [EN]
	* Sets every bit to 0, without changing the logical size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 論理サイズを変えずに、全ビットを0に設定する。
	*/
	void Bitset::clear()
	{
		std::ranges::fill(data_, 0ULL);
	}

	/**
	* [EN]
	* Sets every bit to 1, without changing the logical size.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 論理サイズを変えずに、全ビットを1に設定する。
	*/
	void Bitset::fill()
	{
		std::ranges::fill(data_, ~0ULL);

		/// [EN] The last block was filled past bitCount_ too, so the bits outside the logical size are cleared again.
		/// [JP] 最後のブロックは bitCount_ を超えて埋まっているので、論理サイズの外のビットを消し直す。
		clear_unused_bits();
	}

	/**
	* [EN]
	* Sets the bit at index to value, growing the bitset if index is
	* out of the current range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* index のビットを value に設定する。index が現在の範囲外なら
	* ビットセットを拡張する。
	*/
	void Bitset::set(Size index, Bool value)
	{
		/// [EN] Setting past the end grows the bitset instead of failing.
		/// [JP] 末尾より先へ設定すると、失敗せずにビットセットを広げる。
		ensure_size(index);

		/// [EN] A bit index splits into the block that holds it and its position inside that block.
		/// [JP] ビットの番号を、それを持つブロックと、ブロック内の位置に分ける。
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

	/**
	* [EN]
	* Clears the bit at index (equivalent to set(index, false)).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* index のビットをクリアする（set(index, false) と同等）。
	*/
	void Bitset::reset(Size index)
	{
		set(index, false);
	}

	/**
	* [EN]
	* Toggles the bit at index, growing the bitset if index is out of
	* the current range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* index のビットを反転する。index が現在の範囲外ならビットセットを
	* 拡張する。
	*/
	void Bitset::flip(Size index)
	{
		/// [EN] Flipping past the end grows the bitset first instead of failing.
		/// [JP] 末尾より先を反転すると、失敗せずに先にビットセットを広げる。
		ensure_size(index);

		/// [EN] A bit index splits into the block that holds it and its position inside that block.
		/// [JP] ビットの番号を、それを持つブロックと、ブロック内の位置に分ける。
		const Size blockIndex = index / BitsPerBlock;
		const Size bitIndex = index % BitsPerBlock;

		data_[blockIndex] ^= (1ULL << bitIndex);
	}

	/**
	* [EN]
	* Returns whether the bit at index is set. Returns false (without
	* growing) if index is out of range.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* index のビットが立っているかを返す。index が範囲外の場合は
	* （拡張せずに）false を返す。
	*/
	Bool Bitset::test(Size index)const
	{
		/// [EN] Reading never grows the bitset: a bit outside the logical size reads as 0.
		/// [JP] 読み取りではビットセットを広げない。論理サイズの外のビットは 0 として読む。
		if (index >= bitCount_) [[unlikely]]
		{
			return false;
		}

		/// [EN] A bit index splits into the block that holds it and its position inside that block.
		/// [JP] ビットの番号を、それを持つブロックと、ブロック内の位置に分ける。
		const Size blockIndex = index / BitsPerBlock;
		const Size bitIndex = index % BitsPerBlock;

		return (data_[blockIndex] & (1ULL << bitIndex)) != 0;
	}

	/**
	* [EN]
	* Returns whether any bit is set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* いずれかのビットが立っているかを返す。
	*/
	Bool Bitset::any()const
	{
		/// [EN] Whole blocks are tested at once; the first non-zero one answers the question.
		/// [JP] ブロック単位でまとめて調べ、最初に 0 でないブロックが見つかった時点で答えが出る。
		for (Block block : data_)
		{
			if (block != 0)
			{
				return true;
			}
		}

		return false;
	}

	/**
	* [EN]
	* Returns whether no bit is set (the negation of any()).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* どのビットも立っていないかを返す（any() の否定）。
	*/
	Bool Bitset::none()const
	{
		return !any();
	}

	/**
	* [EN]
	* Returns whether every bit within the logical size is set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 論理サイズ内の全ビットが立っているかを返す。
	*/
	Bool Bitset::all()const
	{
		/// [EN] An empty bitset has no bit that is unset, so it counts as "all set".
		/// [JP] 空のビットセットには立っていないビットが無いので、「全て立っている」とみなす。
		if (bitCount_ == 0)
		{
			return true;
		}

		/// [EN] Check every fully-occupied block first (a fast, all-bits comparison).
		/// [JP] まず完全に占有されている全ブロックをチェックする（全ビット比較による高速な処理）。
		const Size fullBlocks = bitCount_ / BitsPerBlock;

		for (Size index = 0; index < fullBlocks; ++index)
		{
			if (data_[index] != ~0ULL)
			{
				return false;
			}
		}

		/// [EN] The final, partially-occupied block (if any) must be masked before comparing, since bits beyond bitCount_ are don't-cares.
		/// [JP] 最終の部分的に占有されたブロック（あれば）は、比較前にマスクする必要がある。bitCount_ を超えるビットは don't-care であるため。
		const Size remainBits = bitCount_ % BitsPerBlock;

		if (remainBits == 0)
		{
			return true;
		}

		const Uint64 mask = (1ULL << remainBits) - 1ULL;

		return (data_[fullBlocks] & mask) == mask;
	}

	/**
	* [EN]
	* Returns the number of set bits (population count).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 立っているビットの数（ポピュレーションカウント）を返す。
	*/
	Size Bitset::count()const
	{
		/// [EN] std::popcount counts a whole block per call, usually as one CPU instruction.
		/// [JP] std::popcount は1回の呼び出しでブロック全体を数え、多くの場合 CPU 命令1つで済む。
		Size count = 0;

		for (Block block : data_)
		{
			count += std::popcount(block);
		}

		return count;
	}

	/**
	* [EN]
	* Returns the index of the first set bit, or InvalidIndex if none are set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最初に立っているビットのインデックスを返す。1つも立っていなければ
	* InvalidIndex を返す。
	*/
	Size Bitset::find_first_set()const
	{
		for (Size index = 0; index < data_.size(); ++index)
		{
			Block block = data_[index];

			/// [EN] countr_zero gives the position of the lowest set bit within the first non-zero block.
			/// [JP] countr_zero で、最初の 0 でないブロックの中の、最も下位の立っているビットの位置が分かる。
			if (block != 0) [[unlikely]]
			{
				return index * BitsPerBlock + std::countr_zero(block);
			}
		}

		return InvalidIndex;
	}

	/**
	* [EN]
	* Returns the index of the first set bit at or after startIndex,
	* or InvalidIndex if none are set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* startIndex 以降で最初に立っているビットのインデックスを返す。
	* 1つも立っていなければ InvalidIndex を返す。
	*/
	Size Bitset::find_next_set(Size startIndex)const
	{
		/// [EN] Nothing can be found past the logical size.
		/// [JP] 論理サイズより先には何も見つからない。
		if (startIndex >= bitCount_) [[unlikely]]
		{
			return InvalidIndex;
		}

		Size blockIndex = startIndex / BitsPerBlock;
		Size bitIndex = startIndex % BitsPerBlock;

		Block block = data_[blockIndex];

		/// [EN] Mask off every bit before startIndex within its own block, so the scan below can't return a match earlier than requested.
		/// [JP] 自身のブロック内で startIndex より前の全ビットをマスクする。これにより、以下の走査が要求より前の一致を返すことがなくなる。
		block &= (~0ULL << bitIndex);

		/// [EN] Block-by-block scan: test the current block, then move to the next until the array ends.
		/// [JP] ブロック単位の走査。今のブロックを調べ、配列が終わるまで次へ進む。
		while (true)
		{
			/// [EN] assume(true) gives the optimizer no information; it has no effect on the result.
			/// [JP] assume(true) は最適化に何の情報も与えず、結果にも影響しない。
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

	/**
	* [EN]
	* Returns the logical bit count (as set by the constructor/resize()).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* （コンストラクタ/resize() で設定された）論理ビット数を返す。
	*/
	Size Bitset::size()const
	{
		return bitCount_;
	}

	/**
	* [EN]
	* Returns whether the logical bit count is 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 論理ビット数が0かどうかを返す。
	*/
	Bool Bitset::empty()const
	{
		return bitCount_ == 0;
	}

	/**
	* [EN]
	* Returns a const pointer to the backing block array, for bulk/GPU access.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 一括アクセスやGPUアクセス向けに、裏付けブロック配列への
	* const ポインタを返す。
	*/
	const Bitset::Block* Bitset::data()const
	{
		return data_.data();
	}

	/**
	* [EN]
	* Mutable overload of data().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* data() の可変オーバーロード。
	*/
	Bitset::Block* Bitset::data()
	{
		return data_.data();
	}

	/**
	* [EN]
	* Returns the number of Blocks backing this bitset.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このビットセットを裏付ける Block の個数を返す。
	*/
	Size Bitset::block_count()const
	{
		return data_.size();
	}

	/**
	* [EN]
	* Returns whether the bit at index is set (equivalent to test(index)).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* index のビットが立っているかを返す（test(index) と同等）。
	*/
	Bool Bitset::operator[](Size index)const
	{
		return test(index);
	}

	/**
	* [EN]
	* In-place bitwise AND with other. Blocks beyond other's size are
	* cleared to 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのビット単位AND をその場で行う。other のサイズを
	* 超えるブロックは0にクリアされる。
	*/
	Bitset& Bitset::operator&=(const Bitset& other)
	{
		/// [EN] AND only where both bitsets actually have data; bitCount_ is left unchanged.
		/// [JP] 両方のビットセットが実際にデータを持つ範囲でのみ AND を行う。bitCount_ は変えない。
		const Size minBlocks = Min(data_.size(), other.data_.size());

		for (Size index = 0; index < minBlocks; ++index)
		{
			data_[index] &= other.data_[index];
		}

		/// [EN] Any block beyond other's size is implicitly ANDed with 0 (other has no bits there), so clear it directly.
		/// [JP] other のサイズを超えるブロックは、暗黙的に 0 との AND になる（other にはそこにビットが無い）ため、直接クリアする。
		for (Size index = minBlocks; index < data_.size(); ++index)
		{
			data_[index] = 0;
		}

		return *this;
	}

	/**
	* [EN]
	* In-place bitwise OR with other, growing this bitset first if
	* other is logically larger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのビット単位OR をその場で行う。other の方が論理サイズが
	* 大きければ、先にこのビットセットを拡張する。
	*/
	Bitset& Bitset::operator|=(const Bitset& other)
	{
		/// [EN] Growing first guarantees every block of other has a counterpart here.
		/// [JP] 先に広げることで、other の全ブロックに対応するブロックがこちらにもあるようにする。
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

	/**
	* [EN]
	* In-place bitwise XOR with other, growing this bitset first if
	* other is logically larger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのビット単位XOR をその場で行う。other の方が論理サイズが
	* 大きければ、先にこのビットセットを拡張する。
	*/
	Bitset& Bitset::operator^=(const Bitset& other)
	{
		/// [EN] Growing first guarantees every block of other has a counterpart here.
		/// [JP] 先に広げることで、other の全ブロックに対応するブロックがこちらにもあるようにする。
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

	/**
	* [EN]
	* Grows the bitset (via resize) if bitIndex is not yet addressable.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bitIndex がまだアドレス可能でなければ、（resize経由で）
	* ビットセットを拡張する。
	*/
	void Bitset::ensure_size(Size bitIndex)
	{
		if (bitIndex < bitCount_)
		{
			return;
		}

		/// [EN] Grows just enough to make bitIndex the last addressable bit.
		/// [JP] bitIndex がちょうど最後のビットになるだけ広げる。
		resize(bitIndex + 1);
	}

	/**
	* [EN]
	* Returns the number of Blocks needed to hold bitCount bits.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* bitCount ビットを保持するのに必要な Block 数を返す。
	*/
	Size Bitset::get_required_block_count(Size bitCount)
	{
		/// [EN] Integer division rounded up, so a partial last block still counts.
		/// [JP] 切り上げの整数除算なので、途中までしか使わない最後のブロックも数に入る。
		return (bitCount + BitsPerBlock - 1) / BitsPerBlock;
	}

	/**
	* [EN]
	* Zeroes any bits in the last block beyond bitCount_, so bitwise
	* ops/all()/count() don't see stale garbage bits.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最終ブロック中の bitCount_ を超える部分のビットをゼロにする。
	* これにより、ビット演算/all()/count() が古いゴミビットを見ないようにする。
	*/
	void Bitset::clear_unused_bits()
	{
		/// [EN] A logical size that is a whole number of blocks leaves no unused bits.
		/// [JP] 論理サイズがブロックの整数倍なら、使われないビットは無い。
		const Size remainBits = bitCount_ % BitsPerBlock;

		if (remainBits == 0 || data_.empty())
		{
			return;
		}

		/// [EN] The mask keeps the low remainBits bits, which are the ones still inside the logical size.
		/// [JP] マスクは下位 remainBits ビットを残す。それが論理サイズの内側にあるビット。
		const Uint64 mask = (1ULL << remainBits) - 1ULL;

		data_.back() &= mask;
	}
}
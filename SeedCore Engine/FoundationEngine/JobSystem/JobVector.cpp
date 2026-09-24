#include <FoundationEngine/JobSystem/JobVector.h>
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs pointing at firstElement (typically the derived class's
	* inline storage) with size bytes of initial capacity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* firstElement（通常は派生クラスのインラインストレージ）を指し、
	* size バイト分の初期容量で構築する。
	*/
	JobVectorBase::JobVectorBase(void* firstElement, Size size) :begin_(firstElement), end_(firstElement), capacity_((Char*)firstElement + size)
	{
		/// No Code
	}

	/**
	* [EN]
	* POD-path buffer growth to twice the current capacity plus size
	* bytes, or minSizeInBytes if that is larger. A heap buffer is
	* realloc'd; the inline buffer is malloc'd and copied instead.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* POD 経路でのバッファ成長。今の容量の2倍に size バイトを足した
	* 大きさまで、minSizeInBytes の方が大きければそこまで広げる。ヒープの
	* バッファは realloc し、インラインのバッファは malloc してコピーする。
	*/
	void JobVectorBase::grow_pod(void* firstElement, Size minSizeInBytes, Size size)
	{
		/// [EN] Adding size guarantees room for at least one more element even when the capacity is still zero.
		/// [JP] size を足すことで、容量がまだ 0 のときでも少なくとも1要素分の空きができる。
		Size currentSizeBytes = size_in_byte();
		Size newCapacityInBytes = 2 * capacity_in_byte() + size;
		if (newCapacityInBytes < minSizeInBytes)
		{
			newCapacityInBytes = minSizeInBytes;
		}

		void* newElement;
		if (begin_ == firstElement)
		{
			/// [EN] Still on inline storage: cannot realloc it, so malloc fresh storage and copy the live bytes over.
			/// [JP] まだインラインストレージ上のため realloc できない。新しいストレージを malloc し、有効なバイトをコピーする。
			newElement = malloc(newCapacityInBytes);
			memcpy(newElement, this->begin_, currentSizeBytes);
		}
		else
		{
			/// [EN] Already heap-allocated: realloc in place.
			/// [JP] 既にヒープ確保済み: その場で realloc する。
			newElement = realloc(this->begin_, newCapacityInBytes);
		}

		/// [EN] All three pointers are rebuilt relative to the new block.
		/// [JP] 3本のポインタを全て、新しいブロックを基準に作り直す。
		this->end_ = (Char*)newElement + currentSizeBytes;
		this->begin_ = newElement;
		this->capacity_ = (Char*)this->begin_ + newCapacityInBytes;
	}

	/**
	* [EN]
	* Returns the number of live bytes currently stored (end_ - begin_).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在格納されている有効バイト数を返す（end_ - begin_）。
	*/
	Size JobVectorBase::size_in_byte()const
	{
		return Size((Char*)end_ - (Char*)begin_);
	}

	/**
	* [EN]
	* Returns the total allocated capacity in bytes (capacity_ - begin_).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 確保済みの総容量をバイト単位で返す（capacity_ - begin_）。
	*/
	Size JobVectorBase::capacity_in_byte()const
	{
		return Size((Char*)capacity_ - (Char*)begin_);
	}

	/**
	* [EN]
	* Returns whether there are currently no live elements.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在有効な要素が無いかどうかを返す。
	*/
	Bool JobVectorBase::empty()const
	{
		return begin_ == end_;
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the smallest power of two strictly greater than array
	* (used to size new backing storage).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* array より真に大きい最小の2の冪を返す（新しい裏付けストレージの
	* サイズ決定に使う）。
	*/
	Uint64 ArrayNextCapacity(Uint64 array)
	{
		/// [EN] The OR cascade sets every bit below the highest set bit, giving 2^n - 1; adding 1 gives the next power of two.
		/// [JP] OR の連鎖で最上位ビットより下を全て 1 にして 2^n - 1 を作り、1 を足して次の2の冪にする。
		array |= (array >> 1);
		array |= (array >> 2);
		array |= (array >> 4);
		array |= (array >> 8);
		array |= (array >> 16);
		array |= (array >> 32);
		return array + 1;
	}
}

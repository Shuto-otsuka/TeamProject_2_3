#include <FoundationEngine/JobSystem/JobVector.h>
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	JobVectorBase::JobVectorBase(void* firstElement, Size size) :begin_(firstElement), end_(firstElement), capacity_((Char*)firstElement + size)
	{
		/// No Code
	}

	void JobVectorBase::grow_pod(void* firstElement, Size minSizeInBytes, Size size)
	{
		Size currentSizeBytes = size_in_byte();
		Size newCapacityInBytes = 2 * capacity_in_byte() + size;
		if (newCapacityInBytes < minSizeInBytes)
		{
			newCapacityInBytes = minSizeInBytes;
		}

		void* newElement;
		if (begin_ == firstElement)
		{
			newElement = malloc(newCapacityInBytes);
			memcpy(newElement, this->begin_, currentSizeBytes);
		}
		else
		{
			newElement = realloc(this->begin_, newCapacityInBytes);
		}

		this->end_ = (Char*)newElement + currentSizeBytes;
		this->begin_ = newElement;
		this->capacity_ = (Char*)this->begin_ + newCapacityInBytes;
	}

	Size JobVectorBase::size_in_byte()const
	{
		return Size((Char*)end_ - (Char*)begin_);
	}

	Size JobVectorBase::capacity_in_byte()const
	{
		return Size((Char*)capacity_ - (Char*)begin_);
	}

	Bool JobVectorBase::empty()const
	{
		return begin_ == end_;
	}
}

namespace SeedCore
{
	Uint64 ArrayNextCapacity(Uint64 array)
	{
		array |= (array >> 1);
		array |= (array >> 2);
		array |= (array >> 4);
		array |= (array >> 8);
		array |= (array >> 16);
		array |= (array >> 32);
		return array + 1;
	}
}

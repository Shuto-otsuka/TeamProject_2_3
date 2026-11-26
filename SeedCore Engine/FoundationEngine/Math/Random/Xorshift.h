#pragma once

namespace SeedCore
{
	template<typename T>
	class Xorshift
	{
	public:
		Xorshift() = default;

		Xorshift(T value) :state_(value)
		{
			/// No Code
		}

		void Seed(T value)
		{
			state_ = value;
		}

		T operator()()
		{
			if constexpr (sizeof(T) == 8)
			{
				state_ ^= state_ << 13;
				state_ ^= state_ >> 7;
				state_ ^= state_ << 17;
				return state_ * 0x2545F4914F6CDD1DULL;
			}
			else if constexpr (sizeof(T) == 4)
			{
				state_ ^= state_ << 13;
				state_ ^= state_ >> 7;
				state_ ^= state_ << 5;
				return state_;
			}
			else
			{
			}
		}

	private:
		T state_;
	};
}
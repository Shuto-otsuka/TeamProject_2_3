#pragma once
#include <concepts>

namespace SeedCore
{
	template<typename T>
	concept IsInterpolatable = requires(const T & a, const T & b, Float t)
	{
		{ a + b } -> std::same_as<T>;
		{ a - b } -> std::same_as<T>;
		{ a* t } -> std::same_as<T>;
	} && !std::same_as<T, Quaternion>;

	class Easing
	{
	private:
		Easing() = delete;

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T Linear(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t;
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Cos(t * Pi<Float>::Half);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Sin(t * Pi<Float>::Half);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = -(Cos(Pi<Float>::Value * t) - 1.0f) / 2.0f;
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuad(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t * t;
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuad(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - ((1.0f - t) * (1.0f - t));
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutQuad(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = 2.0f * t * t;
			}
			else
			{
				e = 1.0f - (((-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) / 2.0f);
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InCubic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t * t * t;
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutCubic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 3);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutCubic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = 4.0f * Pow(t, 3);
			}
			else
			{
				e = 1.0f - Pow(-2.0f * t + 2.0f, 3) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuart(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, 4);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuart(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 4);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutQuart(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = 8.0f * Pow(t, 4);
			}
			else
			{
				e = 1.0f - Pow(-2.0f * t + 2.0f, 4) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuint(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, 5);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuint(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 5);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutQuint(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = 16.0f * Pow(t, 5);
			}
			else
			{
				e = 1.0f - Pow(-2.0f * t + 2.0f, 5) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, n);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, n);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = Pow(2.0f, n - 1.0f) * Pow(t, n);
			}
			else
			{
				e = 1.0f - Pow(-2.0f * t + 2.0f, n) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			if (t == 0.0f)
			{
				e = 0.0f;
			}
			else
			{
				e = Exp2(10.0f * t - 10.0f);
			}
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			if (t == 1.0f)
			{
				e = 1.0f;
			}
			else
			{
				e = 1.0f - Exp2(-10.0f * t);
			}
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			if (t == 0.0f)
			{
				e = 0.0f;
			}
			else if (t == 1.0f)
			{
				e = 1.0f;
			}
			else if (t < 0.5f)
			{
				e = Exp2(20.0f * t - 10.0f) / 2.0f;
			}
			else
			{
				e = (2.0f - Exp2(-20.0f * t + 10.0f)) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InCirc(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Sqrt(1.0f - t * t);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutCirc(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutCirc(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				e = (1.0f - Sqrt(1.0f - (2.0f * t) * (2.0f * t))) / 2.0f;
			}
			else
			{
				e = (Sqrt(1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) + 1.0f) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c3 = overshoot + 1.0f;
			Float e = c3 * t * t * t - overshoot * t * t;
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c3 = overshoot + 1.0f;

			Float shifted = t - 1.0f;
			Float e = 1.0f + c3 * shifted * shifted * shifted + overshoot * shifted * shifted;
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c2 = overshoot * 1.525f;
			Float e;
			if (t < 0.5f)
			{
				Float doubled = 2.0f * t;
				e = (doubled * doubled * ((c2 + 1.0f) * doubled - c2)) / 2.0f;
			}
			else
			{
				Float shifted = 2.0f * t - 2.0f;
				e = (shifted * shifted * ((c2 + 1.0f) * shifted + c2) + 2.0f) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c4 = Pi<Float>::Two / 3.0f;
			Float e;

			if (t == 0.0f)
			{
				e = 0.0f;
			}
			else if (t == 1.0f)
			{
				e = 1.0f;
			}
			else
			{
				e = -Exp2(10.0f * t - 10.0f) * Sin((10.0f * t - 10.75f) * c4);
			}
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c4 = Pi<Float>::Two / 3.0f;
			Float e;

			if (t == 0.0f)
			{
				e = 0.0f;
			}
			else if (t == 1.0f)
			{
				e = 1.0f;
			}
			else
			{
				e = Exp2(-10.0f * t) * Sin((10.0f * t - 0.75f) * c4) + 1.0f;
			}
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float c5 = Pi<Float>::Two / 4.5f;
			Float e;

			if (t == 0.0f)
			{
				e = 0.0f;
			}
			else if (t == 1.0f)
			{
				e = 1.0f;
			}
			else if (t < 0.5f)
			{
				e = -(Exp2(20.0f * t - 10.0f) * Sin((20.0f * t - 11.125f) * c5)) / 2.0f;
			}
			else
			{
				e = (Exp2(-20.0f * t + 10.0f) * Sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
			}
			return start + (end - start) * e;
		}

	public:
		template<typename T>
			requires IsInterpolatable<T>
		static T InBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float e = 1.0f - OutBounce(0.0f, 1.0f, 1.0f - t);
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T OutBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			Float n1 = 7.5625f;

			Float d1 = 2.75f;
			Float e;
			if (t < 1.0f / d1)
			{
				e = n1 * t * t;
			}
			else if (t < 2.0f / d1)
			{
				Float shifted = t - 1.5f / d1;
				e = n1 * shifted * shifted + 0.75f;
			}
			else if (t < 2.5f / d1)
			{
				Float shifted = t - 2.25f / d1;
				e = n1 * shifted * shifted + 0.9375f;
			}
			else
			{
				Float shifted = t - 2.625f / d1;
				e = n1 * shifted * shifted + 0.984375f;
			}
			return start + (end - start) * e;
		}

		template<typename T>
			requires IsInterpolatable<T>
		static T InOutBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			if (t < 0.5f)
			{
				e = (1.0f - OutBounce(0.0f, 1.0f, 1.0f - 2.0f * t)) / 2.0f;
			}
			else
			{
				e = (1.0f + OutBounce(0.0f, 1.0f, 2.0f * t - 1.0f)) / 2.0f;
			}
			return start + (end - start) * e;
		}
	};
}

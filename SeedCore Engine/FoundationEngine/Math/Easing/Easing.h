#pragma once
#include <FoundationEngine/Prelude.h>
#include <concepts>

namespace SeedCore
{
	/// [EN] Satisfied when T supports the operations linear interpolation start + (end - start) * e needs: addition and subtraction of two T and multiplication by a Float, each yielding T. Quaternion meets these operators but is excluded because linearly blending rotations does not keep them unit length.
	/// [JP] 線形補間 start + (end - start) * e に必要な演算（T 同士の加算・減算と、Float との乗算で、いずれも結果が T になるもの）を T が持つ場合に満たされる。Quaternion もこれらの演算子を持つが、回転を線形に混ぜると単位長が保たれないため除外する。
	template<typename T>
	concept IsInterpolatable = requires(const T & a, const T & b, Float t)
	{
		{ a + b } -> std::same_as<T>;
		{ a - b } -> std::same_as<T>;
		{ a* t } -> std::same_as<T>;
	} && !std::same_as<T, Quaternion>;

	/**
	* [EN]
	* Collection of standard easing curves that interpolate from start to
	* end. Every function clamps the progress t to [0, 1], maps it through
	* its curve to an eased progress e (0 at t = 0, 1 at t = 1), and
	* returns start + (end - start) * e. e itself is not clamped, so Back
	* and Elastic curves overshoot past start or end in between.
	* In = slow start, Out = slow end, InOut = slow at both ends.
	* To get e alone, pass start = 0 and end = 1.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* start から end へ補間する、代表的なイージング曲線の集まり。どの関数も
	* 進行度 t を [0, 1] へクランプし、それぞれの曲線でイージング後の
	* 進行度 e（t = 0 で 0、t = 1 で 1）へ変換して、
	* start + (end - start) * e を返す。e 自体はクランプしないため、Back と
	* Elastic は途中で start や end を行き過ぎる。
	* In = ゆっくり始まる、Out = ゆっくり終わる、InOut = 両端がゆっくり。
	* e だけが欲しい場合は start = 0、end = 1 を渡す。
	*
	* ---------------------------------------------------------------------
	*
	* https://easings.net/ja
	*/
	class Easing
	{
	private:
		/**
		* [EN]
		* Easing is a static-only collection and is never instantiated.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Easing は静的関数のみの集まりであり、インスタンス化しない。
		*/
		Easing() = delete;

	public:
		/**
		* [EN]
		* Constant speed: e = t.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 等速: e = t。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                       **
		*    |                                   ****
		*    |                              *****
		*    |                           ***
		*    |                      *****
		*    |                   ***
		*    |              *****
		*    |           ***
		*    |      *****
		*    |   ***
		*  0 |***
		*    +----------------------------------------- t
		*     0                                       1
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T Linear(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t;
			return start + (end - start) * e;
		}

	public:
		/**
		* [EN]
		* Sine curve, slow start: e = 1 - cos(t * pi / 2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 正弦曲線。ゆっくり始まる: e = 1 - cos(t * pi / 2)。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                       **
		*    |                                     **
		*    |                                  ***
		*    |                               ***
		*    |                             **
		*    |                          ***
		*    |                      ****
		*    |                   ***
		*    |               ****
		*    |         ******
		*  0 |*********
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInSine
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Cos(t * Pi<Float>::Half);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Sine curve, slow end: e = sin(t * pi / 2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 正弦曲線。ゆっくり終わる: e = sin(t * pi / 2)。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                *********
		*    |                          ******
		*    |                      ****
		*    |                   ***
		*    |               ****
		*    |            ***
		*    |          **
		*    |       ***
		*    |    ***
		*    |  **
		*  0 |**
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutSine
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Sin(t * Pi<Float>::Half);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Sine curve, slow at both ends: e = -(cos(pi * t) - 1) / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 正弦曲線。両端がゆっくり: e = -(cos(pi * t) - 1) / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                   ******
		*    |                              *****
		*    |                           ***
		*    |                        ***
		*    |                      **
		*    |                   ***
		*    |                 **
		*    |              ***
		*    |           ***
		*    |      *****
		*  0 |******
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutSine
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutSine(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = -(Cos(Pi<Float>::Value * t) - 1.0f) / 2.0f;
			return start + (end - start) * e;
		}

	public:
		/**
		* [EN]
		* Quadratic curve, slow start: e = t^2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2 次曲線。ゆっくり始まる: e = t^2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                       **
		*    |                                     **
		*    |                                   **
		*    |                                 **
		*    |                              ***
		*    |                           ***
		*    |                        ***
		*    |                     ***
		*    |                *****
		*    |         *******
		*  0 |*********
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInQuad
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuad(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t * t;
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quadratic curve, slow end: e = 1 - (1 - t)^2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2 次曲線。ゆっくり終わる: e = 1 - (1 - t)^2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                *********
		*    |                         *******
		*    |                    *****
		*    |                 ***
		*    |              ***
		*    |           ***
		*    |        ***
		*    |      **
		*    |    **
		*    |  **
		*  0 |**
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutQuad
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuad(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - ((1.0f - t) * (1.0f - t));
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quadratic curve, slow at both ends: e = 2t^2 for t < 0.5,
		* otherwise 1 - (-2t + 2)^2 / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2 次曲線。両端がゆっくり: t < 0.5 なら e = 2t^2、それ以外は
		* 1 - (-2t + 2)^2 / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                  *******
		*    |                              ****
		*    |                          ****
		*    |                        **
		*    |                      **
		*    |                   ***
		*    |                 **
		*    |               **
		*    |           ****
		*    |       ****
		*  0 |*******
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutQuad
		*/
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
		/**
		* [EN]
		* Cubic curve, slow start: e = t^3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 3 次曲線。ゆっくり始まる: e = t^3。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                      **
		*    |                                     *
		*    |                                   **
		*    |                                 **
		*    |                               **
		*    |                             **
		*    |                          ***
		*    |                      ****
		*    |               *******
		*  0 |***************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInCubic
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InCubic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = t * t * t;
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Cubic curve, slow end: e = 1 - (1 - t)^3.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 3 次曲線。ゆっくり終わる: e = 1 - (1 - t)^3。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                          ***************
		*    |                   *******
		*    |               ****
		*    |            ***
		*    |          **
		*    |        **
		*    |      **
		*    |    **
		*    |   *
		*    | **
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutCubic
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutCubic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 3);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Cubic curve, slow at both ends: e = 4t^3 for t < 0.5,
		* otherwise 1 - (-2t + 2)^3 / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 3 次曲線。両端がゆっくり: t < 0.5 なら e = 4t^3、それ以外は
		* 1 - (-2t + 2)^3 / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                               **********
		*    |                           ****
		*    |                         **
		*    |                       **
		*    |                     **
		*    |                    *
		*    |                  **
		*    |                **
		*    |              **
		*    |          ****
		*  0 |**********
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutCubic
		*/
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
		/**
		* [EN]
		* Quartic curve, slow start: e = t^4.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 次曲線。ゆっくり始まる: e = t^4。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                       *
		*    |                                      *
		*    |                                    **
		*    |                                   *
		*    |                                 **
		*    |                               **
		*    |                             **
		*    |                         ****
		*    |                   ******
		*  0 |*******************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInQuart
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuart(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, 4);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quartic curve, slow end: e = 1 - (1 - t)^4.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 次曲線。ゆっくり終わる: e = 1 - (1 - t)^4。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                      *******************
		*    |                ******
		*    |            ****
		*    |          **
		*    |        **
		*    |      **
		*    |     *
		*    |   **
		*    |  *
		*    | *
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutQuart
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuart(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 4);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quartic curve, slow at both ends: e = 8t^4 for t < 0.5,
		* otherwise 1 - (-2t + 2)^4 / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 次曲線。両端がゆっくり: t < 0.5 なら e = 8t^4、それ以外は
		* 1 - (-2t + 2)^4 / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                             ************
		*    |                          ***
		*    |                        **
		*    |                      **
		*    |                     *
		*    |                    *
		*    |                   *
		*    |                 **
		*    |               **
		*    |            ***
		*  0 |************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutQuart
		*/
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
		/**
		* [EN]
		* Quintic curve, slow start: e = t^5.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 5 次曲線。ゆっくり始まる: e = t^5。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                       *
		*    |                                      *
		*    |                                     *
		*    |                                    *
		*    |                                   *
		*    |                                 **
		*    |                               **
		*    |                            ***
		*    |                      ******
		*  0 |**********************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInQuint
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InQuint(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, 5);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quintic curve, slow end: e = 1 - (1 - t)^5.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 5 次曲線。ゆっくり終わる: e = 1 - (1 - t)^5。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                   **********************
		*    |             ******
		*    |          ***
		*    |        **
		*    |      **
		*    |     *
		*    |    *
		*    |   *
		*    |  *
		*    | *
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutQuint
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutQuint(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, 5);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quintic curve, slow at both ends: e = 16t^5 for t < 0.5,
		* otherwise 1 - (-2t + 2)^5 / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 5 次曲線。両端がゆっくり: t < 0.5 なら e = 16t^5、それ以外は
		* 1 - (-2t + 2)^5 / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                            *************
		*    |                         ***
		*    |                       **
		*    |                      *
		*    |                     *
		*    |                    *
		*    |                   *
		*    |                  *
		*    |                **
		*    |             ***
		*  0 |*************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutQuint
		*/
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
		/**
		* [EN]
		* Power curve with any exponent n, slow start: e = t^n. n = 2 to 5
		* matches InQuad to InQuint, and non-integer n gives the curves in
		* between.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 任意の指数 n の累乗曲線。ゆっくり始まる: e = t^n。n = 2〜5 は
		* InQuad〜InQuint と一致し、非整数の n でその中間の曲線になる。
		*
		* ---------------------------------------------------------------------
		*
		* n = 2.5
		*  1 |                                        *
		*    |                                      **
		*    |                                    **
		*    |                                  **
		*    |                                **
		*    |                              **
		*    |                           ***
		*    |                       ****
		*    |                   ****
		*    |             ******
		*  0 |*************
		*    +----------------------------------------- t
		*     0                                       1
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Pow(t, n);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Power curve with any exponent n, slow end: e = 1 - (1 - t)^n.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 任意の指数 n の累乗曲線。ゆっくり終わる: e = 1 - (1 - t)^n。
		*
		* ---------------------------------------------------------------------
		*
		* n = 2.5
		*  1 |                            *************
		*    |                      ******
		*    |                  ****
		*    |              ****
		*    |           ***
		*    |         **
		*    |       **
		*    |     **
		*    |   **
		*    | **
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Pow(1.0f - t, n);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Power curve with any exponent n, slow at both ends:
		* e = 2^(n - 1) * t^n for t < 0.5, otherwise 1 - (-2t + 2)^n / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 任意の指数 n の累乗曲線。両端がゆっくり: t < 0.5 なら
		* e = 2^(n - 1) * t^n、それ以外は 1 - (-2t + 2)^n / 2。
		*
		* ---------------------------------------------------------------------
		*
		* n = 2.5
		*  1 |                                 ********
		*    |                            *****
		*    |                         ***
		*    |                       **
		*    |                     **
		*    |                    *
		*    |                  **
		*    |                **
		*    |             ***
		*    |        *****
		*  0 |********
		*    +----------------------------------------- t
		*     0                                       1
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutPow(T start, T end, Float t, Float n)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;
			if (t < 0.5f)
			{
				/// [EN] 2^(n - 1) scales the first half so that it reaches exactly 0.5 at t = 0.5.
				/// [JP] 2^(n - 1) は、前半が t = 0.5 でちょうど 0.5 に達するように拡大する係数。
				e = Pow(2.0f, n - 1.0f) * Pow(t, n);
			}
			else
			{
				e = 1.0f - Pow(-2.0f * t + 2.0f, n) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		/**
		* [EN]
		* Exponential curve, slow start: e = 2^(10t - 10), pinned to 0 at
		* t = 0.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指数曲線。ゆっくり始まる: e = 2^(10t - 10)。t = 0 では 0 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                        .
		*    |                                       *
		*    |                                      *
		*    |                                     *
		*    |                                    *
		*    |                                  **
		*    |                                 *
		*    |                              ***
		*    |                       *******
		*  0 |***********************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInExpo
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			/// [EN] 2^(10t - 10) is 2^-10 rather than 0 at t = 0, so the endpoint is set explicitly.
			/// [JP] 2^(10t - 10) は t = 0 で 0 ではなく 2^-10 になるため、端点を明示的に決める。
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

		/**
		* [EN]
		* Exponential curve, slow end: e = 1 - 2^(-10t), pinned to 1 at
		* t = 1.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指数曲線。ゆっくり終わる: e = 1 - 2^(-10t)。t = 1 では 1 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                  ***********************
		*    |           *******
		*    |        ***
		*    |       *
		*    |     **
		*    |    *
		*    |   *
		*    |  *
		*    | *
		*    | .
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutExpo
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			/// [EN] 1 - 2^(-10t) is 1 - 2^-10 rather than 1 at t = 1, so the endpoint is set explicitly.
			/// [JP] 1 - 2^(-10t) は t = 1 で 1 ではなく 1 - 2^-10 になるため、端点を明示的に決める。
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

		/**
		* [EN]
		* Exponential curve, slow at both ends: e = 2^(20t - 10) / 2 for
		* t < 0.5, otherwise (2 - 2^(-20t + 10)) / 2, pinned to 0 and 1 at
		* the endpoints.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指数曲線。両端がゆっくり: t < 0.5 なら e = 2^(20t - 10) / 2、
		* それ以外は (2 - 2^(-20t + 10)) / 2。端点では 0 と 1 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                           **************
		*    |                        ***
		*    |                      **
		*    |                      .
		*    |                     *
		*    |                    *
		*    |                   *
		*    |                   .
		*    |                 **
		*    |              ***
		*  0 |**************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutExpo
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutExpo(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			/// [EN] The exponential halves never reach exactly 0 or 1, so both endpoints are set explicitly.
			/// [JP] 指数の各半分はちょうど 0 や 1 に達しないため、両端点を明示的に決める。
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
		/**
		* [EN]
		* Quarter-circle curve, slow start: e = 1 - sqrt(1 - t^2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 分の 1 円の曲線。ゆっくり始まる: e = 1 - sqrt(1 - t^2)。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                        .
		*    |                                       *
		*    |                                      *
		*    |                                    **
		*    |                                  **
		*    |                               ***
		*    |                           ****
		*    |                      *****
		*    |             *********
		*  0 |*************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInCirc
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InCirc(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = 1.0f - Sqrt(1.0f - t * t);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quarter-circle curve, slow end: e = sqrt(1 - (t - 1)^2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 分の 1 円の曲線。ゆっくり終わる: e = sqrt(1 - (t - 1)^2)。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                            *************
		*    |                   *********
		*    |              *****
		*    |          ****
		*    |       ***
		*    |     **
		*    |   **
		*    |  *
		*    | *
		*    | .
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutCirc
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutCirc(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e = Sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Quarter-circle curve, slow at both ends:
		* e = (1 - sqrt(1 - (2t)^2)) / 2 for t < 0.5,
		* otherwise (sqrt(1 - (-2t + 2)^2) + 1) / 2.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 4 分の 1 円の曲線。両端がゆっくり: t < 0.5 なら
		* e = (1 - sqrt(1 - (2t)^2)) / 2、それ以外は
		* (sqrt(1 - (-2t + 2)^2) + 1) / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                *********
		*    |                          ******
		*    |                       ***
		*    |                     **
		*    |                     .
		*    |                    *
		*    |                    .
		*    |                  **
		*    |               ***
		*    |         ******
		*  0 |*********
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutCirc
		*/
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
		/**
		* [EN]
		* Pulls back below start before heading to end (anticipation):
		* e = c3 * t^3 - c1 * t^2 with c1 = overshoot and c3 = c1 + 1.
		* The default overshoot of 1.70158 dips about 10%; larger values
		* pull back further.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* end へ向かう前に、いったん start より手前へ引く（予備動作）:
		* c1 = overshoot、c3 = c1 + 1 として e = c3 * t^3 - c1 * t^2。
		* 既定の overshoot 1.70158 で約 10% 沈み、大きくするほど深く引く。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                       *
		*    |                                      *
		*    |                                     *
		*    |                                   **
		*    |                                  *
		*    |                                **
		*    |                              **
		*    |                            **
		*  0 |********                ****
		*    |        ****************
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInBack
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] c3 = c1 + 1 makes the curve end at exactly 1 at t = 1.
			/// [JP] c3 = c1 + 1 により、曲線は t = 1 でちょうど 1 に終わる。
			Float c3 = overshoot + 1.0f;
			Float e = c3 * t * t * t - overshoot * t * t;
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Goes past end and settles back to it (pop-in):
		* e = 1 + c3 * (t - 1)^3 + c1 * (t - 1)^2 with c1 = overshoot and
		* c3 = c1 + 1. The default overshoot of 1.70158 peaks about 10% past
		* end; larger values peak higher.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* end を行き過ぎてから end へ戻る（ポップイン）:
		* c1 = overshoot、c3 = c1 + 1 として
		* e = 1 + c3 * (t - 1)^3 + c1 * (t - 1)^2。既定の overshoot 1.70158 で
		* end の約 10% 先が頂点になり、大きくするほど頂点が高くなる。
		*
		* ---------------------------------------------------------------------
		*
		*    |                 ****************
		*  1 |             ****                ********
		*    |           **
		*    |         **
		*    |       **
		*    |      *
		*    |    **
		*    |   *
		*    |  *
		*    | *
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutBack
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] c3 = c1 + 1 makes the curve start at exactly 0 at t = 0.
			/// [JP] c3 = c1 + 1 により、曲線は t = 0 でちょうど 0 から始まる。
			Float c3 = overshoot + 1.0f;

			/// [EN] Offset from the end of the curve, t - 1, in [-1, 0].
			/// [JP] 曲線の終端からの差 t - 1。[-1, 0] に入る。
			Float shifted = t - 1.0f;
			Float e = 1.0f + c3 * shifted * shifted * shifted + overshoot * shifted * shifted;
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* InBack for the first half and OutBack for the second half: pulls
		* back below start, then goes past end and settles. c2 = c1 * 1.525
		* rescales overshoot so each half dips about as far as the
		* single-sided curves.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前半が InBack、後半が OutBack: start より手前へ引いてから、end を
		* 行き過ぎて戻る。c2 = c1 * 1.525 は、各半分の行き過ぎ量が片側版の
		* 曲線とほぼ同じになるよう overshoot を補正する係数。
		*
		* ---------------------------------------------------------------------
		*
		*    |                           *********
		*  1 |                         **         *****
		*    |                       **
		*    |                      *
		*    |                     *
		*    |                    *
		*    |                   *
		*    |                  *
		*    |                **
		*  0 |*****         **
		*    |     *********
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutBack
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutBack(T start, T end, Float t, Float overshoot = 1.70158f)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] overshoot rescaled for curves compressed into half of the range.
			/// [JP] 範囲の半分に圧縮した曲線向けに補正した overshoot。
			Float c2 = overshoot * 1.525f;
			Float e;
			if (t < 0.5f)
			{
				/// [EN] Progress within the first half, 2t in [0, 1).
				/// [JP] 前半の中での進行度 2t。[0, 1) に入る。
				Float doubled = 2.0f * t;
				e = (doubled * doubled * ((c2 + 1.0f) * doubled - c2)) / 2.0f;
			}
			else
			{
				/// [EN] Offset from the end within the second half, 2t - 2 in [-1, 0].
				/// [JP] 後半の中での終端からの差 2t - 2。[-1, 0] に入る。
				Float shifted = 2.0f * t - 2.0f;
				e = (shifted * shifted * ((c2 + 1.0f) * shifted + c2) + 2.0f) / 2.0f;
			}
			return start + (end - start) * e;
		}

	public:
		/**
		* [EN]
		* Spring that oscillates around start with growing amplitude before
		* snapping to end: e = -2^(10t - 10) * sin((10t - 10.75) * c4) with
		* c4 = 2pi / 3, pinned to 0 and 1 at the endpoints.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* start の周りで振幅を大きくしながら振動し、最後に end へ跳ぶバネ:
		* c4 = 2pi / 3 として e = -2^(10t - 10) * sin((10t - 10.75) * c4)。
		* 端点では 0 と 1 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                        *
		*    |                                        .
		*    |                                       *
		*    |                                       .
		*    |                                       .
		*    |                                      *
		*    |                            **        .
		*  0 |*********************    ***  **     *
		*    |                     ****       *    .
		*    |                                 *  *
		*    |                                  **
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInElastic
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] Angular frequency of the oscillation: one period every 0.3 of t.
			/// [JP] 振動の角周波数。t が 0.3 進むごとに 1 周期。
			Float c4 = Pi<Float>::Two / 3.0f;
			Float e;

			/// [EN] The exponential envelope never reaches exactly 0 or 1, so both endpoints are set explicitly.
			/// [JP] 指数の包絡線はちょうど 0 や 1 に達しないため、両端点を明示的に決める。
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

		/**
		* [EN]
		* Spring that shoots to end and oscillates around it with shrinking
		* amplitude: e = 2^(-10t) * sin((10t - 0.75) * c4) + 1 with
		* c4 = 2pi / 3, pinned to 0 and 1 at the endpoints.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* end へ勢いよく到達し、その周りで振幅を小さくしながら振動するバネ:
		* c4 = 2pi / 3 として e = 2^(-10t) * sin((10t - 0.75) * c4) + 1。
		* 端点では 0 と 1 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*    |     **
		*    |    *  *
		*    |    .   *       ****
		*  1 |   *     **  ***    *********************
		*    |   .       **
		*    |  *
		*    |  .
		*    |  .
		*    | *
		*    | .
		*  0 |*
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutElastic
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] Angular frequency of the oscillation: one period every 0.3 of t.
			/// [JP] 振動の角周波数。t が 0.3 進むごとに 1 周期。
			Float c4 = Pi<Float>::Two / 3.0f;
			Float e;

			/// [EN] The exponential envelope never reaches exactly 0 or 1, so both endpoints are set explicitly.
			/// [JP] 指数の包絡線はちょうど 0 や 1 に達しないため、両端点を明示的に決める。
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

		/**
		* [EN]
		* InElastic for the first half and OutElastic for the second half:
		* e = -(2^(20t - 10) * sin((20t - 11.125) * c5)) / 2 for t < 0.5,
		* otherwise (2^(-20t + 10) * sin((20t - 11.125) * c5)) / 2 + 1 with
		* c5 = 2pi / 4.5, pinned to 0 and 1 at the endpoints.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前半が InElastic、後半が OutElastic: c5 = 2pi / 4.5 として、
		* t < 0.5 なら e = -(2^(20t - 10) * sin((20t - 11.125) * c5)) / 2、
		* それ以外は (2^(-20t + 10) * sin((20t - 11.125) * c5)) / 2 + 1。
		* 端点では 0 と 1 に固定する。
		*
		* ---------------------------------------------------------------------
		*
		*    |                       ***
		*  1 |                      *   ***************
		*    |                      .
		*    |                     *
		*    |                     .
		*    |                    *
		*    |                    .
		*    |                   *
		*    |                   .
		*  0 |***************   *
		*    |               ***
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutElastic
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutElastic(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] Angular frequency of the oscillation: one period every 0.225 of t.
			/// [JP] 振動の角周波数。t が 0.225 進むごとに 1 周期。
			Float c5 = Pi<Float>::Two / 4.5f;
			Float e;

			/// [EN] The exponential envelope never reaches exactly 0 or 1, so both endpoints are set explicitly.
			/// [JP] 指数の包絡線はちょうど 0 や 1 に達しないため、両端点を明示的に決める。
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
		/**
		* [EN]
		* Mirror of OutBounce: bounces with growing height before reaching
		* end. e = 1 - B(1 - t), where B is OutBounce's eased progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* OutBounce の鏡像: 高さを増しながら跳ねてから end に到達する。
		* B を OutBounce のイージング後の進行度として e = 1 - B(1 - t)。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                                     ****
		*    |                                   **
		*    |                                 **
		*    |                                *
		*    |                               *
		*    |                              *
		*    |                             *
		*    |                            *
		*    |              *********    *
		*    |      ***   **         ** *
		*  0 |******   ***             *
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInBounce
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] OutBounce from 0 to 1 returns its eased progress B directly.
			/// [JP] 0 から 1 への OutBounce は、イージング後の進行度 B をそのまま返す。
			Float e = 1.0f - OutBounce(0.0f, 1.0f, 1.0f - t);
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* Falls to end and bounces off it with shrinking height, like a
		* dropped ball; never goes past end. Built from four parabolas
		* n1 * (t - offset)^2 + floor with n1 = 7.5625 and d1 = 2.75, each
		* one a lower, shorter bounce than the last.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 落としたボールのように end へ落ち、高さを減らしながら跳ね返る。
		* end を行き過ぎることはない。n1 = 7.5625、d1 = 2.75 として、
		* n1 * (t - offset)^2 + floor の 4 本の放物線から成り、後の放物線ほど
		* 低く短い跳ね返りになる。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |               *             ***   ******
		*    |              * **         **   ***
		*    |             *    *********
		*    |            *
		*    |           *
		*    |          *
		*    |         *
		*    |        *
		*    |      **
		*    |    **
		*  0 |****
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeOutBounce
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T OutBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);

			/// [EN] Curvature of every parabola; n1 = d1^2 makes the first one rise from 0 to exactly 1 at t = 1 / d1.
			/// [JP] 全ての放物線の曲率。n1 = d1^2 により、最初の放物線は 0 から始まり t = 1 / d1 でちょうど 1 に達する。
			Float n1 = 7.5625f;

			/// [EN] Time scale of the bounces: the parabolas meet 1 at t = 1/d1, 2/d1 and 2.5/d1.
			/// [JP] 跳ね返りの時間の尺度。放物線は t = 1/d1、2/d1、2.5/d1 で 1 に接する。
			Float d1 = 2.75f;
			Float e;
			if (t < 1.0f / d1)
			{
				/// [EN] The initial fall.
				/// [JP] 最初の落下。
				e = n1 * t * t;
			}
			else if (t < 2.0f / d1)
			{
				/// [EN] First bounce, dipping to 0.75.
				/// [JP] 1 回目の跳ね返り。0.75 まで沈む。
				Float shifted = t - 1.5f / d1;
				e = n1 * shifted * shifted + 0.75f;
			}
			else if (t < 2.5f / d1)
			{
				/// [EN] Second bounce, dipping to 0.9375.
				/// [JP] 2 回目の跳ね返り。0.9375 まで沈む。
				Float shifted = t - 2.25f / d1;
				e = n1 * shifted * shifted + 0.9375f;
			}
			else
			{
				/// [EN] Last bounce, dipping to 0.984375.
				/// [JP] 最後の跳ね返り。0.984375 まで沈む。
				Float shifted = t - 2.625f / d1;
				e = n1 * shifted * shifted + 0.984375f;
			}
			return start + (end - start) * e;
		}

		/**
		* [EN]
		* InBounce for the first half and OutBounce for the second half:
		* e = (1 - B(1 - 2t)) / 2 for t < 0.5, otherwise (1 + B(2t - 1)) / 2,
		* where B is OutBounce's eased progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前半が InBounce、後半が OutBounce: B を OutBounce のイージング後の
		* 進行度として、t < 0.5 なら e = (1 - B(1 - 2t)) / 2、それ以外は
		* (1 + B(2t - 1)) / 2。
		*
		* ---------------------------------------------------------------------
		*
		*  1 |                           **     *******
		*    |                           . *****
		*    |                          *
		*    |                        **
		*    |                       *
		*    |                  *****
		*    |                 *
		*    |               **
		*    |              *
		*    |       *****  .
		*  0 |*******     **
		*    +----------------------------------------- t
		*     0                                       1
		*
		* https://easings.net/ja#easeInOutBounce
		*/
		template<typename T>
			requires IsInterpolatable<T>
		static T InOutBounce(T start, T end, Float t)
		{
			t = Clamp(t, 0.0f, 1.0f);
			Float e;

			/// [EN] OutBounce from 0 to 1 returns its eased progress B directly.
			/// [JP] 0 から 1 への OutBounce は、イージング後の進行度 B をそのまま返す。
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

#pragma once

namespace SeedCore
{
	struct Color
	{
		union
		{
			struct
			{
				Float r;
				Float g;
				Float b;
				Float a;
			};
			struct
			{
				Vector2 rg;
			};
			struct
			{
				Uint32 : 32;
				Vector2 gb;
			};
			struct
			{
				Uint32 : 32;
				Uint32 : 32;
				Vector2 ba;
			};
			struct
			{
				Vector3 rgb;
			};
			struct
			{
				Uint32 : 32;
				Vector3 gba;
			};
		};

		constexpr Color();
		constexpr Color(Float r, Float g, Float b);
		constexpr Color(Float r, Float g, Float b, Float a);
		constexpr explicit Color(const Vector3& rgb);
		constexpr explicit Color(const Vector3& rgb, Float a);
		constexpr explicit Color(const Vector4& vector);

		constexpr Bool operator==(const Color& color)const;
		constexpr Bool operator!=(const Color& color)const;

		constexpr Color& operator+=(const Color& color);
		constexpr Color& operator-=(const Color& color);
		constexpr Color& operator*=(const Color& color);
		constexpr Color& operator/=(const Color& color);

		constexpr Color& operator*=(Float scalar);
		constexpr Color& operator/=(Float scalar);

		constexpr Color operator+()const;
		constexpr Color operator-()const;

		constexpr Color operator+(const Color& color)const;
		constexpr Color operator-(const Color& color)const;
		constexpr Color operator*(const Color& color)const;
		constexpr Color operator/(const Color& color)const;

		constexpr Color operator*(Float scalar)const;
		constexpr Color operator/(Float scalar)const;

		friend constexpr Color operator*(Float scalar, const Color& color);

		NODISCARD constexpr Uint32 ToHex()const;
		NODISCARD static constexpr Color FromHex(Uint32 hex);

		NODISCARD constexpr Uint32 ToPacked()const;
		NODISCARD static constexpr Color FromPacked(Uint32 packed);

		NODISCARD constexpr Vector3 ToHSV()const;
		NODISCARD static constexpr Color FromHSV(Float hue, Float saturation, Float value, Float alpha);

		NODISCARD constexpr Color ToSRGB()const;
		NODISCARD constexpr Color FromSRGB()const;

		NODISCARD constexpr Float Luminance()const;

		constexpr void Saturate();
		constexpr void Negate();
		constexpr void Premultiply();

		constexpr void AdjustSaturation(Float saturation);
		constexpr void AdjustContrast(Float contrast);

		NODISCARD static constexpr Color Lerp(const Color& color1, const Color& color2, Float t);

		static const Color White;
		static const Color Black;
		static const Color Gray;

		static const Color Red;
		static const Color Green;
		static const Color Blue;

		static const Color Yellow;
		static const Color Cyan;
		static const Color Magenta;

		static const Color Clear;
	};

	constexpr Color::Color() :r(0.0f), g(0.0f), b(0.0f), a(1.0f)
	{
		/// No Code
	}

	constexpr Color::Color(Float r, Float g, Float b) : r(r), g(g), b(b), a(1.0f)
	{
		/// No Code
	}

	constexpr Color::Color(Float r, Float g, Float b, Float a) : r(r), g(g), b(b), a(a)
	{
		/// No Code
	}

	constexpr Color::Color(const Vector3& rgb) : r(rgb.x), g(rgb.y), b(rgb.z), a(1.0f)
	{
		/// No Code
	}

	constexpr Color::Color(const Vector3& rgb, Float a) : r(rgb.x), g(rgb.y), b(rgb.z), a(a)
	{
		/// No Code
	}

	constexpr Color::Color(const Vector4& vector) : r(vector.x), g(vector.y), b(vector.z), a(vector.w)
	{
		/// No Code
	}

	constexpr Vector4::Vector4(const Color& color) : x(color.r), y(color.g), z(color.b), w(color.a)
	{
		/// No Code
	}

	constexpr Bool Color::operator==(const Color& color)const
	{
		return r == color.r && g == color.g && b == color.b && a == color.a;
	}

	constexpr Bool Color::operator!=(const Color& color)const
	{
		return !(*this == color);
	}

	constexpr Color& Color::operator+=(const Color& color)
	{
		r += color.r;
		g += color.g;
		b += color.b;
		a += color.a;
		return *this;
	}

	constexpr Color& Color::operator-=(const Color& color)
	{
		r -= color.r;
		g -= color.g;
		b -= color.b;
		a -= color.a;
		return *this;
	}

	constexpr Color& Color::operator*=(const Color& color)
	{
		r *= color.r;
		g *= color.g;
		b *= color.b;
		a *= color.a;
		return *this;
	}

	constexpr Color& Color::operator/=(const Color& color)
	{
		r /= color.r;
		g /= color.g;
		b /= color.b;
		a /= color.a;
		return *this;
	}

	constexpr Color& Color::operator*=(Float scalar)
	{
		r *= scalar;
		g *= scalar;
		b *= scalar;
		a *= scalar;
		return *this;
	}

	constexpr Color& Color::operator/=(Float scalar)
	{
		Float inverse = 1.0f / scalar;
		r *= inverse;
		g *= inverse;
		b *= inverse;
		a *= inverse;
		return *this;
	}

	constexpr Color Color::operator+()const
	{
		return *this;
	}

	constexpr Color Color::operator-()const
	{
		return Color(-r, -g, -b, -a);
	}

	constexpr Color Color::operator+(const Color& color)const
	{
		return Color(r + color.r, g + color.g, b + color.b, a + color.a);
	}

	constexpr Color Color::operator-(const Color& color)const
	{
		return Color(r - color.r, g - color.g, b - color.b, a - color.a);
	}

	constexpr Color Color::operator*(const Color& color)const
	{
		return Color(r * color.r, g * color.g, b * color.b, a * color.a);
	}

	constexpr Color Color::operator/(const Color& color)const
	{
		return Color(r / color.r, g / color.g, b / color.b, a / color.a);
	}

	constexpr Color Color::operator*(Float scalar)const
	{
		return Color(r * scalar, g * scalar, b * scalar, a * scalar);
	}

	constexpr Color Color::operator/(Float scalar)const
	{
		Float inverse = 1.0f / scalar;
		return Color(r * inverse, g * inverse, b * inverse, a * inverse);
	}

	constexpr Color operator*(Float scalar, const Color& color)
	{
		return Color(scalar * color.r, scalar * color.g, scalar * color.b, scalar * color.a);
	}

	constexpr Uint32 Color::ToHex()const
	{
		Float channels[4] = { r, g, b, a };
		Uint32 hex = 0;
		for (Int index = 0;index < 4;++index)
		{
			Float clamped = channels[index] < 0.0f ? 0.0f : (channels[index] > 1.0f ? 1.0f : channels[index]);
			hex = (hex << 8) | static_cast<Uint32>(clamped * 255.0f + 0.5f);
		}
		return hex;
	}

	constexpr Color Color::FromHex(Uint32 hex)
	{
		Float inverse = 1.0f / 255.0f;
		return Color(static_cast<Float>((hex >> 24) & 0xFF) * inverse, static_cast<Float>((hex >> 16) & 0xFF) * inverse, static_cast<Float>((hex >> 8) & 0xFF) * inverse, static_cast<Float>(hex & 0xFF) * inverse);
	}

	constexpr Uint32 Color::ToPacked()const
	{
		Float channels[4] = { a, b, g, r };
		Uint32 packed = 0;
		for (Int index = 0;index < 4;++index)
		{
			Float clamped = channels[index] < 0.0f ? 0.0f : (channels[index] > 1.0f ? 1.0f : channels[index]);
			packed = (packed << 8) | static_cast<Uint32>(clamped * 255.0f + 0.5f);
		}
		return packed;
	}

	constexpr Color Color::FromPacked(Uint32 packed)
	{
		Float inverse = 1.0f / 255.0f;
		return Color(static_cast<Float>(packed & 0xFF) * inverse, static_cast<Float>((packed >> 8) & 0xFF) * inverse, static_cast<Float>((packed >> 16) & 0xFF) * inverse, static_cast<Float>((packed >> 24) & 0xFF) * inverse);
	}

	constexpr Vector3 Color::ToHSV()const
	{
		Float maximum = r > g ? (r > b ? r : b) : (g > b ? g : b);
		Float minimum = r < g ? (r < b ? r : b) : (g < b ? g : b);
		Float delta = maximum - minimum;

		Float hue = 0.0f;
		if (delta > 0.0f)
		{
			if (maximum == r)
			{
				hue = (g - b) / delta;
			}
			else if (maximum == g)
			{
				hue = (b - r) / delta + 2.0f;
			}
			else
			{
				hue = (r - g) / delta + 4.0f;
			}
			hue = SeedCore::Frac(hue / 6.0f);
		}

		Float saturation = maximum > 0.0f ? delta / maximum : 0.0f;
		return Vector3(hue, saturation, maximum);
	}

	constexpr Color Color::FromHSV(Float hue, Float saturation, Float value, Float alpha)
	{
		Float sector = SeedCore::Frac(hue) * 6.0f;
		Int index = static_cast<Int>(sector);
		Float fraction = sector - static_cast<Float>(index);
		Float p = value * (1.0f - saturation);
		Float q = value * (1.0f - saturation * fraction);
		Float t = value * (1.0f - saturation * (1.0f - fraction));
		switch (index)
		{
		case 0:
			return Color(value, t, p, alpha);
		case 1:
			return Color(q, value, p, alpha);
		case 2:
			return Color(p, value, t, alpha);
		case 3:
			return Color(p, q, value, alpha);
		case 4:
			return Color(t, p, value, alpha);
		default:
			return Color(value, p, q, alpha);
		}
	}

	constexpr Color Color::ToSRGB()const
	{
		Float channels[3] = { r, g, b };
		for (Int index = 0;index < 3;++index)
		{
			if (channels[index] <= 0.0031308f)
			{
				channels[index] = channels[index] * 12.92f;
			}
			else
			{
				if consteval
				{
					channels[index] = 1.055f * SeedCore::Pow(channels[index], 1.0f / 2.4f) - 0.055f;
				}
				else
				{
					channels[index] = 1.055f * std::pow(channels[index], 1.0f / 2.4f) - 0.055f;
				}
			}
		}
		return Color(channels[0], channels[1], channels[2], a);
	}

	constexpr Color Color::FromSRGB()const
	{
		Float channels[3] = { r, g, b };
		for (Int index = 0;index < 3;++index)
		{
			if (channels[index] <= 0.04045f)
			{
				channels[index] = channels[index] / 12.92f;
			}
			else
			{
				Float base = (channels[index] + 0.055f) / 1.055f;
				if consteval
				{
					channels[index] = SeedCore::Pow(base, 2.4f);
				}
				else
				{
					channels[index] = std::pow(base, 2.4f);
				}
			}
		}
		return Color(channels[0], channels[1], channels[2], a);
	}

	constexpr Float Color::Luminance()const
	{
		return r * 0.2126f + g * 0.7152f + b * 0.0722f;
	}

	constexpr void Color::Saturate()
	{
		r = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
		g = g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
		b = b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);
		a = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
	}

	constexpr void Color::Negate()
	{
		r = 1.0f - r;
		g = 1.0f - g;
		b = 1.0f - b;
	}

	constexpr void Color::Premultiply()
	{
		r *= a;
		g *= a;
		b *= a;
	}

	constexpr void Color::AdjustSaturation(Float saturation)
	{
		Float luminance = r * 0.2126f + g * 0.7152f + b * 0.0722f;
		r = luminance + (r - luminance) * saturation;
		g = luminance + (g - luminance) * saturation;
		b = luminance + (b - luminance) * saturation;
	}

	constexpr void Color::AdjustContrast(Float contrast)
	{
		r = (r - 0.5f) * contrast + 0.5f;
		g = (g - 0.5f) * contrast + 0.5f;
		b = (b - 0.5f) * contrast + 0.5f;
	}

	constexpr Color Color::Lerp(const Color& color1, const Color& color2, Float t)
	{
		return Color(color1.r + (color2.r - color1.r) * t, color1.g + (color2.g - color1.g) * t, color1.b + (color2.b - color1.b) * t, color1.a + (color2.a - color1.a) * t);
	}

	inline constexpr Color Color::White = { 1.0f, 1.0f, 1.0f, 1.0f };
	inline constexpr Color Color::Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	inline constexpr Color Color::Gray = { 0.5f, 0.5f, 0.5f, 1.0f };

	inline constexpr Color Color::Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	inline constexpr Color Color::Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	inline constexpr Color Color::Blue = { 0.0f, 0.0f, 1.0f, 1.0f };

	inline constexpr Color Color::Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	inline constexpr Color Color::Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	inline constexpr Color Color::Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	inline constexpr Color Color::Clear = { 0.0f, 0.0f, 0.0f, 0.0f };
}
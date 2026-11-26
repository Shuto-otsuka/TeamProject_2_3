#pragma once

namespace SeedCore
{
	class FrameRing
	{
	public:
		NODISCARD static Uint Index();

		static void Advance();

	public:
		SC_CONSTEX Uint frameCount = 2;

	private:
		static inline Uint index_ = 0;
	};
}

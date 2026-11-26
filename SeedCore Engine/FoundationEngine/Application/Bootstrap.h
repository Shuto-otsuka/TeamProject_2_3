#pragma once

namespace SeedCore
{
	struct SEEDCORE_API Bootstrap
	{
		Bootstrap();

		struct Window
		{
			Uint32 Width_ = 1920;

			Uint32 Height_ = 1080;

			Bool Fullscreen_ = false;

			const Wchar* Title_ = L"SeedCore Engine";
		};

		Window WindowDesc_;
	};
}
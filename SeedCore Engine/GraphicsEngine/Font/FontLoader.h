#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Font/Font.h>

namespace SeedCore
{
	class FontLoader :public AssetLoader<Font>
	{
	public:
		FontLoader() = default;
		~FontLoader() = default;

		Handle<Font> Load(String filePath, LoadThread thread);

		void Clear(Handle<Font>& handle)noexcept;
	};
}

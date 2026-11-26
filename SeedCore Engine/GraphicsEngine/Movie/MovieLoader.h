#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Movie/Video.h>

namespace SeedCore
{
	class MovieLoader :public AssetLoader<Video>
	{
	private:
		friend class MovieResource;

	public:
		MovieLoader();

		~MovieLoader();

		Handle<Video> Load(String filePath, LoadThread thread);

		void Clear(Handle<Video>& handle)noexcept;

	private:
		Bool Bake(String sourcePath, String cachePath);

	private:
		Bool ownsComInitialize_ = false;

		Bool mfStarted_ = false;
	};
}

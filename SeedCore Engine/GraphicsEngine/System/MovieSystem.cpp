#include <GraphicsEngine/System/MovieSystem.h>
#include <GraphicsEngine/Movie/Movie.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/Movie/Video.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void MovieSystem::Update(MovieResource& movieResource, World& world)
	{
		Query<Read<Active>, Read<Movie>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Movie& movie)
			{
				if (!active.active_)
				{
					return;
				}

				Video* video = movieResource.Resolve(movieResource.GetHandle(movie.movieID_));
				if (movie.movieID_ == 0 || !video)
				{
					return;
				}

				video->SetLoop(movie.loop_);

				if (movie.autoPlay_ && !video->HasAutoPlayStarted())
				{
					video->Play();
					video->MarkAutoPlayStarted();
				}

				video->Update();
			});
	}

	/**
	* [EN]
	* Uploads the frame each loaded video decoded since the last upload
	* into its GPU texture, recording the copies on cmdList. Runs after
	* Update and before the renderers consume the textures.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 読み込み済みの各動画が前回のアップロード以降にデコードした
	* フレームを、その GPU テクスチャへアップロードする。コピーは cmdList
	* に記録する。Update の後、レンダラーがテクスチャを使う前に動かす。
	*/
	void MovieSystem::Upload(MovieResource& movieResource, ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap)
	{
		/// [EN] Every loaded video is uploaded, not only those a Movie component names, so a video played from outside the world still shows its frames.
		/// [JP] Movie コンポーネントが指すものに限らず、読み込み済みの全動画をアップロードする。world の外から再生された動画でもフレームが出るようにするため。
		for (const Handle<Video>& handle : movieResource.assetHandleMap_ | std::ranges::views::values)
		{
			Video* video = movieResource.Resolve(handle);
			if (!video)
			{
				continue;
			}
			video->UploadFrame(device, cmdList, bindlessHeap);
		}
	}
}

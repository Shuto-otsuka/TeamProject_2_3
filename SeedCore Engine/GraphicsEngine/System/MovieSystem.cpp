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

	void MovieSystem::Upload(MovieResource& movieResource, ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap)
	{
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

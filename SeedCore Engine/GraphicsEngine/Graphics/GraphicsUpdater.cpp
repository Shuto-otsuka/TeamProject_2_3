#include <GraphicsEngine/Graphics/GraphicsUpdater.h>
#include <GraphicsEngine/Graphics/Graphics.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Font/FontResource.h>
#include <GraphicsEngine/Movie/MovieResource.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>

#include <FoundationEngine/Benchmark/Benchmark.h>

namespace SeedCore
{
	GraphicsUpdater::GraphicsUpdater(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	GraphicsUpdater::~GraphicsUpdater() = default;

	void GraphicsUpdater::Configure(const GraphicsSettings& settings)
	{
		shapeVisible_ = settings.shapeVisible_;
	}

	void GraphicsUpdater::Update(Float deltaTime, World& world, Float aspectRatio)
	{
		Benchmark::Begin("GraphicsUpdate");

		GraphicsResourceSystem& resources = module_.Resources();
		D3D12Context& context = device_.Context();
		BindlessHeap& resourceHeap = device_.ResourceHeap();

		Benchmark::Begin("Font");
		std::get<FontSystem>(systems_).Update(*resources.fontResource_, context.GetDevice(), *context.GetFrameTransferLane(), &resourceHeap);
		Benchmark::End("Font");

		Benchmark::Begin("Movie");
		std::get<MovieSystem>(systems_).Update(*resources.movieResource_, world);
		std::get<MovieSystem>(systems_).Upload(*resources.movieResource_, context.GetDevice(), context.GetDirectList()->Get(), &resourceHeap);
		Benchmark::End("Movie");

		Benchmark::Begin("Effect");
		std::get<EffectSystem>(systems_).Update(*resources.effectResource_, *resources.effekseerResource_, world, deltaTime);
		Benchmark::End("Effect");

		Benchmark::Begin("Celestial");
		std::get<CelestialSystem>(systems_).Execute(world, deltaTime);
		Benchmark::End("Celestial");

		Benchmark::Begin("Weather");
		std::get<WeatherSystem>(systems_).Execute(world, deltaTime, std::get<CelestialSystem>(systems_).MonthOfYear());
		Benchmark::End("Weather");

		Benchmark::Begin("Collider");
		std::get<ColliderSystem>(systems_).Update(world);
		Benchmark::End("Collider");

		Benchmark::Begin("Shape");
		std::get<ShapeSystem>(systems_).Update(world, shapeVisible_, aspectRatio);
		Benchmark::End("Shape");

		Benchmark::Begin("Animation");
		std::get<ConstraintSystem>(systems_).Execute(world);
		std::get<AnimationSystem>(systems_).Execute(world, *resources.animationResource_, *resources.modelResource_);
		std::get<ConstraintSystem>(systems_).Execute(world);
		Benchmark::End("Animation");

		Benchmark::Begin("Material");
		std::get<MaterialSystem>(systems_).Execute(world, *resources.materialResource_, *resources.modelResource_);
		Benchmark::End("Material");

		Benchmark::End("GraphicsUpdate");
	}

	const CelestialResult& GraphicsUpdater::Celestial()const
	{
		return std::get<CelestialSystem>(systems_).Result();
	}

	ShapeList GraphicsUpdater::Shapes()const
	{
		ShapeList list;
		list.shapes_ = std::get<ShapeSystem>(systems_).Shapes();
		list.colliders_ = std::get<ColliderSystem>(systems_).Colliders();
		return list;
	}
}

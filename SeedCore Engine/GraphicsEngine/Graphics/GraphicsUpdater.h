#pragma once
#include <GraphicsEngine/System/AnimationSystem.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/System/ColliderSystem.h>
#include <GraphicsEngine/System/ConstraintSystem.h>
#include <GraphicsEngine/System/EffectSystem.h>
#include <GraphicsEngine/System/FontSystem.h>
#include <GraphicsEngine/System/MaterialSystem.h>
#include <GraphicsEngine/System/MovieSystem.h>
#include <GraphicsEngine/System/ShapeSystem.h>
#include <GraphicsEngine/System/WeatherSystem.h>

namespace SeedCore
{
	struct GraphicsSettings;

	class GraphicsModule;
	class RenderDevice;
	class World;

	class GraphicsUpdater
	{
	public:
		GraphicsUpdater(RenderDevice& device, GraphicsModule& module);

		~GraphicsUpdater();

	public:
		void Configure(const GraphicsSettings& settings);

	public:
		void Update(Float deltaTime, World& world, Float aspectRatio);

		NODISCARD const CelestialResult& Celestial()const;

		NODISCARD ShapeList Shapes()const;

	private:
		using Systems = std::tuple
			<
			FontSystem,
			MovieSystem,
			EffectSystem,
			CelestialSystem,
			WeatherSystem,
			ColliderSystem,
			ShapeSystem,
			ConstraintSystem,
			AnimationSystem,
			MaterialSystem
			>;

	private:
		RenderDevice& device_;

		GraphicsModule& module_;

		Bool shapeVisible_ = false;

		Systems systems_;
	};
}

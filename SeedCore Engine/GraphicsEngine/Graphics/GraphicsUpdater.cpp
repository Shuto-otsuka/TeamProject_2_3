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
	/**
	* [EN]
	* Keeps the device and the asset module the systems work with.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* System が使うデバイスとアセットモジュールを受け取る。
	*/
	GraphicsUpdater::GraphicsUpdater(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the updater.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 更新器を破棄する。
	*/
	GraphicsUpdater::~GraphicsUpdater() = default;

	/**
	* [EN]
	* Records the shape debug display setting.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形のデバッグ表示の設定を記録する。
	*/
	void GraphicsUpdater::Configure(const GraphicsSettings& settings)
	{
		shapeVisible_ = settings.shapeVisible_;
	}

	/**
	* [EN]
	* Advances the per-frame state shared by every view.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全ビューで共有するフレームごとの状態を進める。
	*/
	void GraphicsUpdater::Update(Float deltaTime, World& world, Float aspectRatio)
	{
		Benchmark::Begin("GraphicsUpdate");

		GraphicsResourceSystem& resources = module_.Resources();
		D3D12Context& context = device_.Context();
		BindlessHeap& resourceHeap = device_.ResourceHeap();

		/// [EN] Asset state (fonts, movies, effects) is brought up to date before any Feature gathers it.
		/// [JP] アセットの状態（フォント、ムービー、エフェクト）は、Feature が集める前に最新にする。
		Benchmark::Begin("Font");
		std::get<FontSystem>(systems_).Update(*resources.fontResource_, context.GetDevice(), *context.GetFrameTransferLane(), &resourceHeap);
		Benchmark::End("Font");

		Benchmark::Begin("Movie");
		std::get<MovieSystem>(systems_).Update(*resources.movieResource_, world);
		std::get<MovieSystem>(systems_).Upload(*resources.movieResource_, context.GetDevice(), context.GetDirectList()->Get(), &resourceHeap);
		Benchmark::End("Movie");

		Benchmark::Begin("Effect");
		std::get<EffectSystem>(systems_).Update(world, deltaTime);
		Benchmark::End("Effect");

		/// [EN] The celestial clock advances by the frame time and drives the sun's DirectionalLight before Light gathers the lights.
		/// [JP] 天体の時計をフレーム時間だけ進め、Light がライトを集める前に太陽の DirectionalLight を動かす。
		Benchmark::Begin("Celestial");
		std::get<CelestialSystem>(systems_).Execute(world, deltaTime);
		Benchmark::End("Celestial");

		/// [EN] Colliders and shapes are gathered once per frame; the debug display follows the Selected tag and the visibility flag, and camera frustums use the render aspect ratio.
		/// [JP] コライダーと形はフレームに1回集める。デバッグ表示は Selected タグと表示のフラグに従い、カメラの視錐台は描画の縦横比を使う。
		Benchmark::Begin("Collider");
		std::get<ColliderSystem>(systems_).Update(world);
		Benchmark::End("Collider");

		Benchmark::Begin("Shape");
		std::get<ShapeSystem>(systems_).Update(world, shapeVisible_, aspectRatio);
		Benchmark::End("Shape");

		/// [EN] Constraints are resolved before and after animation, so constrained bones follow the posed skeleton; materials are resolved last, onto the posed actors.
		/// [JP] コンストレイントはアニメーションの前後で解決し、拘束されたボーンがポーズの付いた骨格に従うようにする。マテリアルは最後に、ポーズの付いたアクターに対して解決する。
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

	/**
	* [EN]
	* Returns this frame's sun, moon and night factor from the celestial system.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 天体の System から、このフレームの太陽・月・夜の度合いを返す。
	*/
	const CelestialResult& GraphicsUpdater::Celestial()const
	{
		return std::get<CelestialSystem>(systems_).Result();
	}
}

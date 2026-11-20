#include <GraphicsEngine/Effect/Zephyr/ParticleFeature.h>
#include <GraphicsEngine/Effect/Zephyr/ParticleStage.h>
#include <GraphicsEngine/Effect/Effect.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ParticleFeature, UpdateStage, DrawStage);

	void ParticleFeature::Create(RenderDevice& device)
	{
		resetPipeline_.Create(device);
		beginPipeline_.Create(device);
		drivenPipeline_.Create(device);

		String filePath = assembler_.GetOrCreate(DynamicArray<String>{ String("Gravity"), String("Drag") });
		spawnPipeline_.Create(device, DynamicComputeDescription{ filePath, String("Spawn"), 64, 1, 1 });
		simulatePipeline_.Create(device, DynamicComputeDescription{ filePath, String("Simulate"), 64, 1, 1, &SimulateArguments });

		std::filesystem::path folder = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr";
		DynamicGraphicsDescription quadDescription{};
		quadDescription.meshFile_ = String((folder / "Quad/ParticleQuadMS.hlsl").string());
		quadDescription.vertexFile_ = String((folder / "Quad/ParticleQuadVS.hlsl").string());
		quadDescription.pixelFile_ = String((folder / "Quad/ParticleQuadPS.hlsl").string());
		quadDescription.fallback_ = GraphicsFallback::Indirect;
		quadDescription.targetFormat_ = { RenderFormat::RGBA16Float };
		quadDescription.depthFormat_ = RenderFormat::D32Float;
		quadDescription.blend_ = BlendStateType::Additive;
		quadDescription.depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;
		quadDescription.rasterizer_ = RasterizerStateType::SolidNoneLHS;
		quadDescription.arguments_ = &QuadArguments;
		quadDescription.fallbackArguments_ = &QuadFallbackArguments;
		quadPipeline_.Create(device, quadDescription);
	}

	void ParticleFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		World& world = *scene.world_;
		flip_ = !flip_;

		DynamicArray<ParticleEmitterStructuredBuffer> emitters;
		DynamicArray<EntityID> entities;
		DynamicArray<Uint32> seedCounts = { 0 };
		parameters_.clear();
		Uint32 sharedUsed = 0;

		Query<Read<Active>, Read<Effect>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Effect& effect)
			{
				if (!active.active_ || effect.capacity_ == 0)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}

				ParticleEmitterStructuredBuffer emitter{};
				emitter.capacity_ = effect.capacity_;
				if (effect.capacity_ <= sharedThreshold_ && sharedUsed + effect.capacity_ <= sharedCapacity_)
				{
					emitter.particles_ = 0;
					emitter.base_ = sharedUsed;
					sharedUsed += effect.capacity_;
				}
				else
				{
					emitter.particles_ = static_cast<Uint32>(seedCounts.size());
					emitter.base_ = 0;
					seedCounts.push_back(effect.capacity_);
				}
				emitter.aliveRead_ = emitter.base_ * 2 + (flip_ ? emitter.capacity_ : 0);
				emitter.aliveWrite_ = emitter.base_ * 2 + (flip_ ? 0 : emitter.capacity_);
				emitter.parameters_ = static_cast<Uint32>(parameters_.size());
				emitter.spawnCount_ = Min(effect.spawnCount_, effect.capacity_);
				emitter.emitter_.delta_ = effect.delta_;
				emitter.emitter_.age_ = effect.age_;
				emitter.emitter_.emitterToWorld_ = actor.WorldMatrix();
				emitter.emitter_.worldToEmitter_ = emitter.emitter_.emitterToWorld_.Invert();
				emitters.push_back(emitter);
				entities.push_back(entityID);

				parameters_.push_back(Vector4(0.0f, -9.8f, 0.0f, 0.0f));
				parameters_.push_back(Vector4(0.5f, 0.0f, 0.0f, 0.0f));
			});

		seedCounts[0] = sharedUsed > 0 ? sharedCapacity_ : 0;

		Bool changed = entities != entities_ || emitters.size() != emitters_.size();
		for (Size emitterIndex = 0;!changed && emitterIndex < emitters.size();++emitterIndex)
		{
			changed = emitters[emitterIndex].particles_ != emitters_[emitterIndex].particles_ || emitters[emitterIndex].base_ != emitters_[emitterIndex].base_ || emitters[emitterIndex].capacity_ != emitters_[emitterIndex].capacity_;
		}

		emitters_ = std::move(emitters);
		entities_ = std::move(entities);
		seedCounts_ = std::move(seedCounts);
		aliveCounts_.clear();
		for (Uint32 count : seedCounts_)
		{
			aliveCounts_.push_back(count * 2);
		}

		active_ = !emitters_.empty();
		reset_ = active_ && changed;
	}

	void ParticleFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!active_)
		{
			return;
		}

		upload.Write(Emitters, RenderViewKind::Game, std::span<const ParticleEmitterStructuredBuffer>(emitters_));
		upload.Write(Parameters, RenderViewKind::Game, std::span<const Vector4>(parameters_));

		upload.Resize(Seeds, seedCounts_);
		upload.Resize(Dead, seedCounts_);
		upload.Resize(Alive, aliveCounts_);

		Uint32 emitterCount = static_cast<Uint32>(emitters_.size());
		upload.Resize(Counters, std::span<const Uint32>(&emitterCount, 1));
		upload.Resize(Offsets, std::span<const Uint32>(&emitterCount, 1));
		upload.Resize(QuadOffsets, std::span<const Uint32>(&emitterCount, 1));
	}
}

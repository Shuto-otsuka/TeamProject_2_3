#include <GraphicsEngine/Effect/Zephyr/ParticleFeature.h>
#include <GraphicsEngine/Effect/Zephyr/ParticleStage.h>
#include <GraphicsEngine/Effect/Effect.h>
#include <GraphicsEngine/Effect/EffectAsset.h>
#include <GraphicsEngine/Effect/EffectResource.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
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

		std::filesystem::path engineRoot = FileLocator::EngineRoot();
		DynamicGraphicsDescription quadDescription{};
		quadDescription.meshFile_ = String((engineRoot / "GraphicsEngine/Effect/Zephyr/Quad/ParticleQuadMS.hlsl").string());
		quadDescription.vertexFile_ = String((engineRoot / "GraphicsEngine/Effect/Zephyr/Quad/ParticleQuadVS.hlsl").string());
		quadDescription.pixelFile_ = String((engineRoot / "GraphicsEngine/Effect/Zephyr/Quad/ParticleQuadPS.hlsl").string());
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
		EffectResource& effectResource = *scene.resource_->effectResource_;
		flip_ = !flip_;

		struct Candidate
		{
			EntityID entity_;

			Uint32 source_ = 0;

			Uint32 program_ = 0;

			Uint32 capacity_ = 0;

			Uint32 spawnCount_ = 0;

			ParticleEmitter emitter_;

			DynamicArray<Vector4> parameters_;
		};
		DynamicArray<Candidate> candidates;

		Query<Read<Active>, Read<Effect>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, const Effect& effect)
			{
				const EffectAsset* asset = effectResource.Resolve(effectResource.GetHandle(effect.effectID_));
				if (!active.active_ || !asset)
				{
					return;
				}
				Actor actor = WorldAccessor::Make(world, entityID);
				if (!actor)
				{
					return;
				}
				Matrix emitterToWorld = actor.WorldMatrix();

				for (Size sourceIndex = 0;sourceIndex < asset->emitters_.size();++sourceIndex)
				{
					const EffectEmitter& source = asset->emitters_[sourceIndex];
					const EffectNode* receiver = source.Find(String("Emitter"));
					Float maxParticles = receiver ? receiver->Value(String("MaxParticles")).x : 0.0f;
					Uint32 capacity = maxParticles > 0.0f ? static_cast<Uint32>(maxParticles) : 0;
					if (!source.enabled_ || source.kind_ != EffectEmitterKind::Zephyr || capacity == 0)
					{
						continue;
					}

					ZephyrAssembly assembly = assembler_.Assemble(source);
					auto found = std::ranges::find_if(programs_, [&assembly](const Program& program) { return program.file_ == assembly.filePath_; });
					Size programIndex = static_cast<Size>(std::distance(programs_.begin(), found));
					if (found == programs_.end())
					{
						programs_.emplace_back().file_ = assembly.filePath_;
					}
					if (programs_[programIndex].created_ && !programs_[programIndex].valid_)
					{
						continue;
					}

					Candidate& candidate = candidates.emplace_back();
					candidate.entity_ = entityID;
					candidate.source_ = static_cast<Uint32>(sourceIndex);
					candidate.program_ = static_cast<Uint32>(programIndex);
					candidate.capacity_ = capacity;
					candidate.spawnCount_ = sourceIndex < effect.spawnCounts_.size() ? Min(effect.spawnCounts_[sourceIndex], capacity) : 0;
					candidate.emitter_.delta_ = effect.delta_;
					candidate.emitter_.age_ = effect.age_;
					candidate.emitter_.emitterToWorld_ = emitterToWorld;
					candidate.emitter_.worldToEmitter_ = emitterToWorld.Invert();
					for (const ZephyrParameterRow& row : assembly.rows_)
					{
						auto node = std::ranges::find_if(source.nodes_, [&row](const EffectNode& candidateNode) { return candidateNode.id_ == row.node_; });
						candidate.parameters_.push_back(node != source.nodes_.end() ? node->Value(row.input_) : row.default_);
					}
				}
			});

		std::ranges::stable_sort(candidates, {}, &Candidate::program_);

		DynamicArray<ParticleEmitterStructuredBuffer> emitters;
		DynamicArray<EntityID> entities;
		DynamicArray<Uint32> sources;
		DynamicArray<Uint32> emitterPrograms;
		DynamicArray<Uint32> seedCounts = { 0 };
		parameters_.clear();
		groups_.clear();
		groupPrograms_.clear();
		Uint32 sharedUsed = 0;

		for (const Candidate& candidate : candidates)
		{
			if (groupPrograms_.empty() || groupPrograms_.back() != candidate.program_)
			{
				if (groups_.size() == groupCapacity_)
				{
					break;
				}
				ParticleGroupStructuredBuffer& group = groups_.emplace_back();
				group.first_ = static_cast<Uint32>(emitters.size());
				groupPrograms_.push_back(candidate.program_);
			}
			++groups_.back().count_;

			ParticleEmitterStructuredBuffer emitter{};
			emitter.capacity_ = candidate.capacity_;
			if (candidate.capacity_ <= sharedThreshold_ && sharedUsed + candidate.capacity_ <= sharedCapacity_)
			{
				emitter.particles_ = 0;
				emitter.base_ = sharedUsed;
				sharedUsed += candidate.capacity_;
			}
			else
			{
				emitter.particles_ = static_cast<Uint32>(seedCounts.size());
				emitter.base_ = 0;
				seedCounts.push_back(candidate.capacity_);
			}
			emitter.aliveRead_ = emitter.base_ * 2 + (flip_ ? emitter.capacity_ : 0);
			emitter.aliveWrite_ = emitter.base_ * 2 + (flip_ ? 0 : emitter.capacity_);
			emitter.parameters_ = static_cast<Uint32>(parameters_.size());
			emitter.spawnCount_ = candidate.spawnCount_;
			emitter.emitter_ = candidate.emitter_;
			emitters.push_back(emitter);
			entities.push_back(candidate.entity_);
			sources.push_back(candidate.source_);
			emitterPrograms.push_back(candidate.program_);
			parameters_.insert(parameters_.end(), candidate.parameters_.begin(), candidate.parameters_.end());
		}

		seedCounts[0] = sharedUsed > 0 ? sharedCapacity_ : 0;

		Bool changed = entities != entities_ || sources != sources_ || emitterPrograms != emitterPrograms_ || emitters.size() != emitters_.size();
		for (Size emitterIndex = 0;!changed && emitterIndex < emitters.size();++emitterIndex)
		{
			changed = emitters[emitterIndex].particles_ != emitters_[emitterIndex].particles_ || emitters[emitterIndex].base_ != emitters_[emitterIndex].base_ || emitters[emitterIndex].capacity_ != emitters_[emitterIndex].capacity_;
		}

		emitters_ = std::move(emitters);
		entities_ = std::move(entities);
		sources_ = std::move(sources);
		emitterPrograms_ = std::move(emitterPrograms);
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
		upload.Write(Groups, RenderViewKind::Game, std::span<const ParticleGroupStructuredBuffer>(groups_));

		upload.Resize(Seeds, seedCounts_);
		upload.Resize(Dead, seedCounts_);
		upload.Resize(Alive, aliveCounts_);

		Uint32 emitterCount = static_cast<Uint32>(emitters_.size());
		upload.Resize(Counters, std::span<const Uint32>(&emitterCount, 1));
		upload.Resize(Offsets, std::span<const Uint32>(&emitterCount, 1));
		upload.Resize(QuadOffsets, std::span<const Uint32>(&emitterCount, 1));
	}
}

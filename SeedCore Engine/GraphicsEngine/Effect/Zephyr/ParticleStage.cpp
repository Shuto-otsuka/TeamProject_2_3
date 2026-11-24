#include <GraphicsEngine/Effect/Zephyr/ParticleStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void ParticleFeature::UpdateStage::ResetStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		for (Size emitterIndex = 0;emitterIndex < feature.emitters_.size();++emitterIndex)
		{
			feature.resetPipeline_.Dispatch(pass, feature.emitters_[emitterIndex].capacity_, 1, 1, static_cast<Uint32>(emitterIndex));
		}
	}

	void ParticleFeature::UpdateStage::BeginStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		feature.beginPipeline_.Dispatch(pass, 1, 1, 1, static_cast<Uint32>(feature.groups_.size()));
	}

	void ParticleFeature::UpdateStage::SpawnStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		for (Program& program : feature.programs_)
		{
			if (program.created_)
			{
				continue;
			}
			program.created_ = true;
			program.valid_ = program.spawn_.Create(pass.Device(), DynamicComputeDescription{ program.file_, String("Spawn"), 64, 1, 1 }) && program.simulate_.Create(pass.Device(), DynamicComputeDescription{ program.file_, String("Simulate"), 64, 1, 1, &SimulateArguments });
		}

		for (Size emitterIndex = 0;emitterIndex < feature.emitters_.size();++emitterIndex)
		{
			Program& program = feature.programs_[feature.emitterPrograms_[emitterIndex]];
			if (!program.valid_ || feature.emitters_[emitterIndex].spawnCount_ == 0)
			{
				continue;
			}
			program.spawn_.Dispatch(pass, feature.emitters_[emitterIndex].spawnCount_, 1, 1, static_cast<Uint32>(emitterIndex));
		}
	}

	void ParticleFeature::UpdateStage::SimulateStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		for (Size groupIndex = 0;groupIndex < feature.groups_.size();++groupIndex)
		{
			Program& program = feature.programs_[feature.groupPrograms_[groupIndex]];
			if (!program.valid_)
			{
				continue;
			}
			program.simulate_.DispatchIndirect(pass, static_cast<Uint32>(groupIndex), static_cast<Uint32>(groupIndex));
		}
	}

	void ParticleFeature::UpdateStage::DrivenStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		feature.drivenPipeline_.Dispatch(pass, 1, 1);
	}

	void ParticleFeature::DrawStage::QuadStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		feature.quadPipeline_.DrawIndirect(pass, 1);
	}
}

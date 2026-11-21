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
		feature.beginPipeline_.Dispatch(pass, 1, 1);
	}

	void ParticleFeature::UpdateStage::SpawnStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		for (Size emitterIndex = 0;emitterIndex < feature.emitters_.size();++emitterIndex)
		{
			if (feature.emitters_[emitterIndex].spawnCount_ == 0)
			{
				continue;
			}
			feature.spawnPipeline_.Dispatch(pass, feature.emitters_[emitterIndex].spawnCount_, 1, 1, static_cast<Uint32>(emitterIndex));
		}
	}

	void ParticleFeature::UpdateStage::SimulateStep::Execute(ParticleFeature& feature, RenderPass& pass)
	{
		feature.simulatePipeline_.DispatchIndirect(pass);
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

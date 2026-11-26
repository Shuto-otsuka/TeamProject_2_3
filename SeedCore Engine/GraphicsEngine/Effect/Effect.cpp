#include <GraphicsEngine/Effect/Effect.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	void Effect::OnAwake()
	{
		if (autoPlay_)
		{
			Play();
		}
	}

	void Effect::OnDestroy()
	{
		Stop(EffectStopFlag::Immediate);
	}

	void Effect::Play()
	{
		EffekseerManager& effekseerManager = Gateway::GetEffekseerManager();
		for (const EffectPlayback& playback : playbacks_)
		{
			if (playback.current_ >= 0)
			{
				effekseerManager.Stop(playback.current_, EffectStopFlag::Immediate);
			}
			if (playback.previous_ >= 0)
			{
				effekseerManager.Stop(playback.previous_, EffectStopFlag::Immediate);
			}
		}
		playbacks_.clear();

		age_ = 0.0f;
		std::ranges::fill(spawnAccumulators_, 0.0f);
		playing_ = true;
	}

	void Effect::Stop(EffectStopFlag flag)
	{
		EffekseerManager& effekseerManager = Gateway::GetEffekseerManager();
		for (EffectPlayback& playback : playbacks_)
		{
			if (playback.current_ >= 0)
			{
				effekseerManager.Stop(playback.current_, flag);
			}
			if (playback.previous_ >= 0)
			{
				effekseerManager.Stop(playback.previous_, flag);
			}
			playback.finished_ = true;
		}
		if (flag == EffectStopFlag::Immediate)
		{
			playbacks_.clear();
		}
		playing_ = false;
	}

	Bool Effect::Playing()const
	{
		return playing_;
	}
}
#include <GraphicsEngine/System/SplashSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>

namespace SeedCore
{
	void SplashSystem::Initialize(Bool showWarning, Bool showFiction)
	{
		showWarning_ = showWarning;
		showFiction_ = showFiction;

		config_.Load();
	}

	void SplashSystem::Update(const ResourceCache& resourceCache)
	{
		if (phase_ == SplashPhase::Complete)
		{
			return;
		}

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (!started_)
		{
			splashStartTime_ = now;
			started_ = true;
		}

		Float elapsed = std::chrono::duration<Float>(now - splashStartTime_).count();

		Float warningEnd = showWarning_ ? warningDuration_ : 0.0f;
		Float fictionEnd = warningEnd + (showFiction_ ? fictionDuration_ : 0.0f);
		Float criLogoEnd = fictionEnd + criLogoDuration_;
		Float engineLogoEnd = criLogoEnd + engineLogoDuration_;

		if (elapsed < engineLogoEnd)
		{
			Float phaseStart = criLogoEnd;
			Float phaseEnd = engineLogoEnd;
			phase_ = SplashPhase::EngineLogo;

			if (elapsed < warningEnd)
			{
				phaseStart = 0.0f;
				phaseEnd = warningEnd;
				phase_ = SplashPhase::Warning;
			}
			else if (elapsed < fictionEnd)
			{
				phaseStart = warningEnd;
				phaseEnd = fictionEnd;
				phase_ = SplashPhase::Fiction;
			}
			else if (elapsed < criLogoEnd)
			{
				phaseStart = fictionEnd;
				phaseEnd = criLogoEnd;
				phase_ = SplashPhase::CriLogo;
			}

			Float phaseElapsed = elapsed - phaseStart;
			Float phaseDuration = phaseEnd - phaseStart;

			alpha_ = 1.0f;
			if (phaseElapsed < fadeInDuration_)
			{
				alpha_ = phaseElapsed / fadeInDuration_;
			}
			else if (phaseElapsed > phaseDuration - fadeOutDuration_)
			{
				alpha_ = (phaseDuration - phaseElapsed) / fadeOutDuration_;
			}
			return;
		}

		if (resourceCache.Complete())
		{
			phase_ = SplashPhase::Complete;
			return;
		}

		if (phase_ != SplashPhase::Boot)
		{
			phase_ = SplashPhase::Boot;
			bootStartTime_ = now;
		}

		alpha_ = 1.0f;
		progress_ = resourceCache.Progress();
		time_ = std::chrono::duration<Float>(now - bootStartTime_).count();
	}

	Bool SplashSystem::Complete()const
	{
		return phase_ == SplashPhase::Complete;
	}

	SplashPhase SplashSystem::Phase()const
	{
		return phase_;
	}

	Float SplashSystem::Alpha()const
	{
		return alpha_;
	}

	Float SplashSystem::Progress()const
	{
		return progress_;
	}

	Float SplashSystem::Time()const
	{
		return time_;
	}

	const BootConfig& SplashSystem::Config()const
	{
		return config_;
	}
}

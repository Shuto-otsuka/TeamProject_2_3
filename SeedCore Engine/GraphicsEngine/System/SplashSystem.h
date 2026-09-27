#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Config/BootConfig.h>

namespace SeedCore
{
	class ResourceCache;

	enum class SplashPhase
	{
		Warning,
		Fiction,
		CriLogo,
		EngineLogo,
		Boot,
		Complete,
	};

	class SEEDCORE_API SplashSystem
	{
	public:
		void Initialize(Bool showWarning, Bool showFiction);

		void Update(const ResourceCache& resourceCache);

		Bool Complete()const;

	public:
		SplashPhase Phase()const;

		Float Alpha()const;

		Float Progress()const;

		Float Time()const;

		const BootConfig& Config()const;

	private:
		BootConfig config_;

		SplashPhase phase_ = SplashPhase::Warning;

		std::chrono::steady_clock::time_point splashStartTime_;
		std::chrono::steady_clock::time_point bootStartTime_;

		Float warningDuration_ = 2.5f;
		Float fictionDuration_ = 2.5f;
		Float criLogoDuration_ = 2.5f;
		Float engineLogoDuration_ = 3.0f;

		Float fadeInDuration_ = 0.5f;
		Float fadeOutDuration_ = 0.5f;

		Float alpha_ = 0.0f;
		Float progress_ = 0.0f;
		Float time_ = 0.0f;

		Bool showWarning_ = false;
		Bool showFiction_ = false;
		Bool started_ = false;
	};
}

#pragma once

namespace SeedCore
{
	struct CelestialResult
	{
		Vector3 sunDirection_ = { 0.0f, -1.0f, 0.0f };
		Color sunColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Float sunIntensity_ = 0.0f;

		Bool sunEnabled_ = false;

		Float sunAngularRadius_ = 0.03f;

		Vector3 moonDirection_ = { 0.0f, 1.0f, 0.0f };
		Color moonColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Float moonIntensity_ = 0.0f;

		Float moonPhase_ = 0.0f;

		Float moonAngularRadius_ = 0.02f;

		Float nightFactor_ = 0.0f;

		Float skyZenithColor_[3] = { 0.15f, 0.35f, 0.75f };
		Float skyHorizonColor_[3] = { 0.65f, 0.75f, 0.9f };
	};

	class World;
	struct Celestial;

	class SEEDCORE_API CelestialSystem
	{
	public:
		void Execute(World& world, Float deltaTime);

		NODISCARD const CelestialResult& Result()const;

		NODISCARD Bool Enabled()const;

		NODISCARD Bool SunEnabled()const;

		NODISCARD Bool SunLightDriven()const;

		NODISCARD Bool MoonEnabled()const;

		NODISCARD Uint32 MonthOfYear()const;

		NODISCARD Float SunAngularRadius()const;

	private:
		void Advance(Float deltaTime, Celestial& celestial);

		void Compute(const Celestial& celestial);

	private:
		CelestialResult result_;

		Bool enabled_ = false;

		Bool sunEnabled_ = false;

		Bool sunLightDriven_ = false;

		Bool moonEnabled_ = false;

		Uint32 monthOfYear_ = 1;

		Float sunAngularRadius_ = 0.03f;
	};
}

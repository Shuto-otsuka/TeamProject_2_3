#pragma once

namespace SeedCore
{
	/// [EN] Output of CelestialSystem::Compute: everything downstream (LightSystem,
	///      VolumetricStarRenderer) needs for this frame's sun/moon/night state.
	/// [JP] CelestialSystem::Compute の出力: 下流(LightSystem、
	///      VolumetricStarRenderer)がこのフレームの太陽/月/夜状態のために必要な
	///      全てを持つ。
	struct CelestialResult
	{
		Vector3 sunDirection_ = { 0.0f, -1.0f, 0.0f };
		Color sunColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Float sunIntensity_ = 0.0f;

		Vector3 moonDirection_ = { 0.0f, 1.0f, 0.0f };
		Color moonColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Float moonIntensity_ = 0.0f;

		/// [EN] 0 = new moon, 0.5 = full moon, 1 = new moon again.
		/// [JP] 0=新月、0.5=満月、1=次の新月。
		Float moonPhase_ = 0.0f;

		Float moonAngularRadius_ = 0.02f;

		/// [EN] 0 = full day, 1 = full night. Drives star visibility/shooting-star chance.
		/// [JP] 0=完全な昼、1=完全な夜。星の見え方/流れ星の発生確率を駆動する。
		Float nightFactor_ = 0.0f;

		/// [EN] Realistic time-of-day background sky gradient (night -> twilight ->
		///      sunrise/sunset -> day), independent of the sun's light color.
		///      Drives VolumetricCloudScapesRayConstantBuffer's
		///      skyZenithColor_/skyHorizonColor_ through VolumetricCloudScapesSystem.
		/// [JP] 現実的な時刻依存の空グラデーション(夜→薄明→朝焼け/夕焼け→昼)。
		///      太陽の光源色とは独立。VolumetricCloudScapesSystem を通して
		///      VolumetricCloudScapesRayConstantBuffer の skyZenithColor_/skyHorizonColor_ を駆動する。
		Float skyZenithColor_[3] = { 0.15f, 0.35f, 0.75f };
		Float skyHorizonColor_[3] = { 0.65f, 0.75f, 0.9f };
	};

	class World;
	struct Celestial;

	/**
	* [EN]
	* Drives the day/night cycle from the scene's Celestial component. Execute
	* advances the component's clock by the game's scaled delta time, derives
	* this frame's sun/moon direction, color and intensity, and writes the sun
	* onto the DirectionalLight of the same actor. Without a Celestial
	* component the result is derived from the component's defaults.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンの Celestial コンポーネントから昼夜サイクルを駆動する。Execute は
	* コンポーネントの時計をゲームの倍率付き経過時間で進め、このフレームの
	* 太陽と月の方向・色・強さを求め、太陽を同じアクターの DirectionalLight へ
	* 書き込む。Celestial コンポーネントが無いときは、コンポーネントの既定値から
	* 結果を求める。
	*/
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

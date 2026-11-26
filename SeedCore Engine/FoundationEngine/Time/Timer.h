#pragma once

namespace SeedCore
{
	class SEEDCORE_API Timer
	{
	public:
		void Start();

		void Stop();

		void Reset();

		void Tick();

		Float Delta()const;

		Float Total()const;

	private:
		std::chrono::high_resolution_clock::time_point start_;

		std::chrono::high_resolution_clock::time_point last_;

		std::chrono::high_resolution_clock::time_point stop_;

		std::chrono::duration<Double> paused_ = std::chrono::duration<Double>::zero();

		Double delta_ = 0.0f;

		Bool stopped_ = false;
	};
}
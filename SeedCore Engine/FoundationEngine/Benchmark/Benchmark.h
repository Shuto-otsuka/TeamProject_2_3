#pragma once

namespace SeedCore
{
	class BenchmarkThread;

	class SEEDCORE_API Benchmark
	{
	private:
		friend class BenchmarkSystem;

	public:
		template<Size Length>
		static void Begin(const Char(&name)[Length])
		{
			BeginLiteral(name);
		}

		template<Size Length>
		static void End(const Char(&name)[Length])
		{
			EndLiteral(name);
		}

	private:
		static void BeginLiteral(const Char* name);

		static void EndLiteral(const Char* name);

		static void Bind(BenchmarkThread* thread);
	};
}
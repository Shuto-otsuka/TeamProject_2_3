#pragma once

namespace SeedCore
{
	class ResourceCache;
	class World;

	struct BenchmarkContext
	{
		World& world_;
		ResourceCache& cache_;
	};

	enum class BenchmarkUnit :Uint8
	{
		Nanoseconds,
		Microseconds,
	};

	struct BenchmarkSample
	{
		String name_;
		BenchmarkUnit unit_ = BenchmarkUnit::Nanoseconds;
		Double median_ = 0.0;
		Double minimum_ = 0.0;
		Double deviation_ = 0.0;
		Size iterationCount_ = 0;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			Int unitValue = static_cast<Int>(unit_);
			archive.Field("name", name_);
			archive.Field("unit", unitValue);
			archive.Field("median", median_);
			archive.Field("minimum", minimum_);
			archive.Field("deviation", deviation_);
			archive.Field("iteration_count", iterationCount_);
			unit_ = static_cast<BenchmarkUnit>(unitValue);
		}
	};

	struct BenchmarkRecord
	{
		String date_;
		String configuration_;
		DynamicArray<BenchmarkSample> samples_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("date", date_);
			archive.Field("configuration", configuration_);
			archive.Field("samples", samples_);
		}
	};

	class SEEDCORE_API BenchmarkRecorder
	{
	public:
		template<std::invocable Body>
		void Measure(String name, BenchmarkUnit unit, Body&& body)
		{
			body();

			Size iterationCount = 1;
			while (iterationCount < (Size(1) << 24))
			{
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				for (Size iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex)
				{
					body();
				}
				if (std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(5))
				{
					break;
				}
				iterationCount *= 2;
			}

			Measure(name, unit, iterationCount, body, []() {});
		}

		template<std::invocable Body, std::invocable Reset>
		void Measure(String name, BenchmarkUnit unit, Size iterationCount, Body&& body, Reset&& reset)
		{
			DynamicArray<Double> perCallSeconds;
			for (Size sampleIndex = 0;sampleIndex < 9;++sampleIndex)
			{
				reset();
				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				for (Size iterationIndex = 0;iterationIndex < iterationCount;++iterationIndex)
				{
					body();
				}
				perCallSeconds.push_back(std::chrono::duration<Double>(std::chrono::steady_clock::now() - start).count() / static_cast<Double>(iterationCount));
			}
			reset();

			std::ranges::sort(perCallSeconds);
			Double median = perCallSeconds[perCallSeconds.size() / 2];

			DynamicArray<Double> distances;
			for (Double seconds : perCallSeconds)
			{
				distances.push_back(Abs(seconds - median));
			}
			std::ranges::sort(distances);

			Double scale = unit == BenchmarkUnit::Nanoseconds ? 1.0e9 : 1.0e6;

			BenchmarkSample sample;
			sample.name_ = name;
			sample.unit_ = unit;
			sample.median_ = median * scale;
			sample.minimum_ = perCallSeconds.front() * scale;
			sample.deviation_ = distances[distances.size() / 2] * scale;
			sample.iterationCount_ = iterationCount;
			samples_.push_back(std::move(sample));
		}

		template<typename T>
		static void Keep(const T& value)
		{
			const volatile Byte* pointer = reinterpret_cast<const volatile Byte*>(&value);
			static_cast<void>(*pointer);
		}

		template<typename Self>
		auto Samples(this Self&& self)
		{
			if constexpr (std::is_lvalue_reference_v<Self>)
			{
				return std::span<const BenchmarkSample>(self.samples_);
			}
			else
			{
				return DynamicArray<BenchmarkSample>(std::move(self.samples_));
			}
		}

	private:
		DynamicArray<BenchmarkSample> samples_;
	};

	template<typename T>
	concept RunnableBenchmark = std::default_initializable<T> && requires(T & benchmark, BenchmarkContext & context, BenchmarkRecorder & recorder)
	{
		{ T::Name() } -> std::convertible_to<const Char*>;
		benchmark.Run(context, recorder);
	};

	class SEEDCORE_API BenchmarkSuite :public NonTransferable
	{
	public:
		template<RunnableBenchmark T>
		void Register()
		{
			entries_.push_back(Entry{ String(T::Name()),&Entry::template RunAs<T> });
		}

		template<typename... Modules>
			requires(requires(BenchmarkSuite& suite) { Modules::Register(suite); } && ...)
		static void Register(BenchmarkSuite& suite)
		{
			(Modules::Register(suite), ...);
		}

	public:
		BenchmarkRecord Run(BenchmarkContext& context)const;

		Bool Save(const std::filesystem::path& path, const BenchmarkRecord& record)const;

		Bool Load(const std::filesystem::path& path, BenchmarkRecord& record)const;

	private:
		struct Entry
		{
			String name_;

			void (*run_)(BenchmarkContext& context, BenchmarkRecorder& recorder) = nullptr;

			template<RunnableBenchmark T>
			static void RunAs(BenchmarkContext& context, BenchmarkRecorder& recorder)
			{
				T benchmark;
				if constexpr (requires{benchmark.Setup(context); })
				{
					benchmark.Setup(context);
				}
				benchmark.Run(context, recorder);
				if constexpr (requires{benchmark.Teardown(context); })
				{
					benchmark.Teardown(context);
				}
			}
		};

		DynamicArray<Entry> entries_;
	};
}
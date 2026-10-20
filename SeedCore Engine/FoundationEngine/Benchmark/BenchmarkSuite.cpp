#include <FoundationEngine/Benchmark/BenchmarkSuite.h>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	BenchmarkRecord BenchmarkSuite::Run(BenchmarkContext& context)const
	{
		HANDLE thread = GetCurrentThread();
		Int previousPriority = GetThreadPriority(thread);
		SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);

		BenchmarkRecorder recorder;
		for (const Entry& entry : entries_)
		{
			entry.run_(context, recorder);
		}

		SetThreadPriority(thread, previousPriority);

		BenchmarkRecord record;
		record.date_ = String(std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()))));
#if SC_DEBUG
		record.configuration_ = String("Debug");
#else
		record.configuration_ = String("Release");
#endif
		record.samples_ = std::move(recorder).Samples();
		return record;
	}

	Bool BenchmarkSuite::Save(const std::filesystem::path& path, const BenchmarkRecord& record)const
	{
		std::error_code errorCode;
		std::filesystem::create_directories(path.parent_path(), errorCode);

		JsonOutputArchive archive;
		archive.Field("record", record);
		return archive.Write(String(path.string()));
	}

	Bool BenchmarkSuite::Load(const std::filesystem::path& path, BenchmarkRecord& record)const
	{
		JsonInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			return false;
		}
		return archive.TryField("record", record);
	}
}
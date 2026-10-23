#pragma once

namespace SeedCore
{
	enum class RuntimeBuildStage
	{
		Build,
		Package,
	};

	class RuntimeBuilder
	{
	public:
		~RuntimeBuilder();

		void BuildAsync(const std::filesystem::path& projectRoot);

		NODISCARD Bool IsBuilding()const;

		NODISCARD RuntimeBuildStage GetStage()const;

		NODISCARD Float GetProgress()const;

		NODISCARD std::string GetStatusText()const;

		NODISCARD std::string GetDetailText()const;

		Bool ConsumeResult(Bool& outSuccess, std::string& outLog);

	private:
		Bool Build(const std::filesystem::path& projectRoot, std::string& outLog);

		Bool Package(const std::filesystem::path& projectRoot, const std::filesystem::path& outputDir, std::string& outLog);

		Bool RunProcessCaptureOutput(const std::wstring& commandLine, std::string& outResult, const std::function<void(const std::string&)>& onChunk = nullptr);

		std::optional<std::filesystem::path> FindMSBuild(const std::filesystem::path& projectRoot);

	private:
		std::thread thread_;
		std::atomic<Bool> building_{ false };

		std::mutex resultMutex_;
		Bool hasResult_ = false;
		Bool lastSuccess_ = false;
		std::string lastLog_;

		std::atomic<RuntimeBuildStage> stage_{ RuntimeBuildStage::Build };
		std::atomic<Float> progress_{ 0.0f };

		mutable std::mutex statusMutex_;
		std::string statusText_;
		std::string detailText_;
	};
}

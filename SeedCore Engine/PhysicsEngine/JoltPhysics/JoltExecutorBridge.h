#pragma once

namespace JPH
{
	class JobSystem;
}

namespace SeedCore
{
	class JobExecutor;

	/**
	* [EN]
	* Adapts Jolt's job-system interface to SeedCore's JobExecutor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のジョブシステムインターフェースを SeedCore の JobExecutor へ適合させる。
	*/
	class JoltExecutorBridge final
	{
	public:
		explicit JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers = 8);
		~JoltExecutorBridge();

		JoltExecutorBridge(const JoltExecutorBridge&) = delete;
		JoltExecutorBridge& operator=(const JoltExecutorBridge&) = delete;

	public:
		JPH::JobSystem* Native();

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}


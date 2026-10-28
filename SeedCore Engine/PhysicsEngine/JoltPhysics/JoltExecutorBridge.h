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
		/**
		* [EN]
		* Creates a bridge backed by the supplied executor and barrier capacity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定されたエグゼキューターとバリア容量を使うブリッジを生成する。
		*/
		explicit JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers = 8);

		/**
		* [EN]
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~JoltExecutorBridge();

		/// [EN] Jolt holds a raw pointer to the Impl as its job system, so the bridge is neither copied nor reassigned.
		/// [JP] Jolt が Impl の生ポインタをジョブシステムとして持つので、ブリッジはコピーも代入もしない。
		JoltExecutorBridge(const JoltExecutorBridge&) = delete;
		JoltExecutorBridge& operator=(const JoltExecutorBridge&) = delete;

	public:
		/**
		* [EN]
		* Returns the bridge as the Jolt job system to hand to the physics
		* update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理の更新へ渡す Jolt のジョブシステムとして、ブリッジを返す。
		*/
		JPH::JobSystem* Native();

	private:
		class Impl;

		/// [EN] The Jolt job system itself, owned through a pointer so Jolt's headers stay out of this one.
		/// [JP] Jolt のジョブシステム本体。Jolt のヘッダをこのヘッダから外すため、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};
}


#pragma once

namespace JPH
{
	class JobSystem;
}

namespace SeedCore
{
	class JobExecutor;

	/**
	* [JP]
	* 物理エンジンの仕事を、エンジンの仕事の仕組み（JobExecutor）で動かすためのつなぎ役。
	*
	* @brief 物理エンジンの仕事を流すつなぎ役
	*
	* [EN]
	* The go-between that runs the physics engine's jobs on the engine's job system (JobExecutor).
	*
	* @brief Runs physics jobs on the job system
	*/
	class JoltExecutorBridge final
	{
	public:
		/**
		* [JP]
		* 仕事を流す先を受け取って準備する。
		*
		* @brief 準備する
		* @param executor    仕事を流す先
		* @param maxBarriers 同時に待てる区切りの数
		*
		* [EN]
		* Takes where to send jobs and gets ready.
		*
		* @brief Gets ready
		* @param executor    Where jobs are sent
		* @param maxBarriers How many wait points can be used at once
		*/
		explicit JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers = 8);

		/**
		* [JP]
		* 中身を片付ける。
		*
		* @brief 片付ける
		*
		* [EN]
		* Cleans up the inner parts.
		*
		* @brief Cleans up
		*/
		~JoltExecutorBridge();

		JoltExecutorBridge(const JoltExecutorBridge&) = delete;
		JoltExecutorBridge& operator=(const JoltExecutorBridge&) = delete;

	public:
		/**
		* [JP]
		* 物理エンジンに渡す、仕事の仕組みを返す。
		*
		* @brief  物理エンジン用の仕組みを返す
		* @return 物理エンジンに渡す仕組み
		*
		* [EN]
		* Returns the job system to hand to the physics engine.
		*
		* @brief  Returns the physics job system
		* @return The job system for the physics engine
		*/
		JPH::JobSystem* Native();

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};
}


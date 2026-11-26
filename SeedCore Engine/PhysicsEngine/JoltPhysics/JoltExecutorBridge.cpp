#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Core/Color.h>
#include <External/JoltPhysics/Jolt/Core/JobSystemWithBarrier.h>
#include <PhysicsEngine/JoltPhysics/JoltExecutorBridge.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	/**
	* [JP]
	* JoltExecutorBridge の中身。物理エンジンの仕事を受け取り、エンジンの仕事として流す。
	*
	* @brief JoltExecutorBridge の中身
	*
	* [EN]
	* The inside of JoltExecutorBridge; it takes physics jobs and runs them as engine jobs.
	*
	* @brief The inside of JoltExecutorBridge
	*/
	class JoltExecutorBridge::Impl final :public JPH::JobSystemWithBarrier
	{
	public:
		/**
		* [JP]
		* 仕事を流す先を覚えて準備する。
		*
		* @brief 準備する
		* @param executor    仕事を流す先
		* @param maxBarriers 同時に待てる区切りの数
		*
		* [EN]
		* Remembers where to send jobs and gets ready.
		*
		* @brief Gets ready
		* @param executor    Where jobs are sent
		* @param maxBarriers How many wait points can be used at once
		*/
		explicit Impl(JobExecutor& executor, JPH::uint maxBarriers);

		/**
		* [JP]
		* 同時に動かせる仕事の数を返す。
		*
		* @brief  同時に動かせる数を返す
		* @return CPU のスレッドの数
		*
		* [EN]
		* Returns how many jobs can run at once.
		*
		* @brief  Returns how many can run at once
		* @return The number of CPU threads
		*/
		Int GetMaxConcurrency()const override;

		/**
		* [JP]
		* 仕事を作る。待つものがなければすぐ流す。
		*
		* @brief  仕事を作る
		* @param  name            仕事の名前
		* @param  color           調べるときの色
		* @param  function        やること
		* @param  dependencyCount 先に終わるのを待つ仕事の数
		* @return 作った仕事
		*
		* [EN]
		* Makes a job, and sends it at once if it has nothing to wait for.
		*
		* @brief  Makes a job
		* @param  name            The job's name
		* @param  color           The color used when debugging
		* @param  function        What to do
		* @param  dependencyCount How many jobs it waits for
		* @return The new job
		*/
		JobHandle CreateJob(const Char* name, JPH::ColorArg color, const JobFunction& function, JPH::uint32 dependencyCount)override;

	protected:
		/**
		* [JP]
		* 仕事を1つ流す。
		*
		* @brief 仕事を1つ流す
		* @param job 流す仕事
		*
		* [EN]
		* Sends one job.
		*
		* @brief Sends one job
		* @param job The job to send
		*/
		void QueueJob(Job* job)override;

		/**
		* [JP]
		* 仕事をまとめて流す。
		*
		* @brief 仕事をまとめて流す
		* @param jobs     流す仕事
		* @param jobCount 仕事の数
		*
		* [EN]
		* Sends several jobs together.
		*
		* @brief Sends several jobs
		* @param jobs     The jobs to send
		* @param jobCount How many there are
		*/
		void QueueJobs(Job** jobs, JPH::uint jobCount)override;

		/**
		* [JP]
		* 終わった仕事を片付ける。
		*
		* @brief 仕事を片付ける
		* @param job 片付ける仕事
		*
		* [EN]
		* Cleans up a finished job.
		*
		* @brief Frees a job
		* @param job The job to free
		*/
		void FreeJob(Job* job)override;

	private:
		/// [JP] 仕事を流す先
		/// [EN] Where jobs are sent
		JobExecutor& executor_;
	};

	/**
	* [JP]
	* 仕事を流す先を覚えて準備する。
	*
	* @brief 準備する
	* @param executor    仕事を流す先
	* @param maxBarriers 同時に待てる区切りの数
	*
	* [EN]
	* Remembers where to send jobs and gets ready.
	*
	* @brief Gets ready
	* @param executor    Where jobs are sent
	* @param maxBarriers How many wait points can be used at once
	*/
	JoltExecutorBridge::Impl::Impl(JobExecutor& executor, JPH::uint maxBarriers) :JPH::JobSystemWithBarrier(maxBarriers), executor_(executor)
	{
		/// No Code
	}

	/**
	* [JP]
	* 同時に動かせる仕事の数を返す。
	*
	* @brief  同時に動かせる数を返す
	* @return CPU のスレッドの数
	*
	* [EN]
	* Returns how many jobs can run at once.
	*
	* @brief  Returns how many can run at once
	* @return The number of CPU threads
	*/
	Int JoltExecutorBridge::Impl::GetMaxConcurrency()const
	{
		return static_cast<Int>(std::thread::hardware_concurrency());
	}

	/**
	* [JP]
	* 仕事を作る。待つものがなければすぐ流す。
	*
	* @brief  仕事を作る
	* @param  name            仕事の名前
	* @param  color           調べるときの色
	* @param  function        やること
	* @param  dependencyCount 先に終わるのを待つ仕事の数
	* @return 作った仕事
	*
	* [EN]
	* Makes a job, and sends it at once if it has nothing to wait for.
	*
	* @brief  Makes a job
	* @param  name            The job's name
	* @param  color           The color used when debugging
	* @param  function        What to do
	* @param  dependencyCount How many jobs it waits for
	* @return The new job
	*/
	JoltExecutorBridge::Impl::JobHandle JoltExecutorBridge::Impl::CreateJob(const Char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies)
	{
		Job* job = new Job(inName, inColor, this, inJobFunction, inNumDependencies);
		JobHandle handle(job);

		if (inNumDependencies == 0)
		{
			QueueJob(job);
		}

		return handle;
	}

	/**
	* [JP]
	* 仕事を1つ流す。
	*
	* @brief 仕事を1つ流す
	* @param job 流す仕事
	*
	* [EN]
	* Sends one job.
	*
	* @brief Sends one job
	* @param job The job to send
	*/
	void JoltExecutorBridge::Impl::QueueJob(Job* inJob)
	{
		/// [JP] 終わるまで消えないように持っておき、1つの流れにして流す
		/// [EN] Hold on to the job until it finishes and send it as one flow
		inJob->AddRef();

		ResourceRef<JobTaskflow> flow = MakeRef<JobTaskflow>();
		flow->emplace([inJob]()
			{
				inJob->Execute();
				inJob->Release();
			});

		executor_.Run(*flow, [flow]()
			{
				/// No Code
			});
	}

	/**
	* [JP]
	* 仕事をまとめて流す。
	*
	* @brief 仕事をまとめて流す
	* @param jobs     流す仕事
	* @param jobCount 仕事の数
	*
	* [EN]
	* Sends several jobs together.
	*
	* @brief Sends several jobs
	* @param jobs     The jobs to send
	* @param jobCount How many there are
	*/
	void JoltExecutorBridge::Impl::QueueJobs(Job** inJobs, JPH::uint inNumJobs)
	{
		/// [JP] 全部の仕事を1つの流れに入れて、まとめて流す
		/// [EN] Put every job into one flow and send them together
		ResourceRef<JobTaskflow> flow = MakeRef<JobTaskflow>();

		for (JPH::uint index = 0; index < inNumJobs; ++index)
		{
			Job* inJob = inJobs[index];
			inJob->AddRef();
			flow->emplace([inJob]()
				{
					inJob->Execute();
					inJob->Release();
				});
		}

		executor_.Run(*flow, [flow]()
			{
				/// No Code
			});
	}

	/**
	* [JP]
	* 終わった仕事を片付ける。
	*
	* @brief 仕事を片付ける
	* @param job 片付ける仕事
	*
	* [EN]
	* Cleans up a finished job.
	*
	* @brief Frees a job
	* @param job The job to free
	*/
	void JoltExecutorBridge::Impl::FreeJob(Job* inJob)
	{
		delete inJob;
	}

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
	JoltExecutorBridge::JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers) :impl_(MakePtr<Impl>(executor, static_cast<JPH::uint>(maxBarriers)))
	{
		/// No Code
	}

	JoltExecutorBridge::~JoltExecutorBridge() = default;

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
	JPH::JobSystem* JoltExecutorBridge::Native()
	{
		return impl_.get();
	}
}

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
	* [EN]
	* The Jolt job system behind JoltExecutorBridge: submits Jolt jobs to
	* SeedCore's JobExecutor as JobTaskflow work.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JoltExecutorBridge の奥にある Jolt のジョブシステム。Jolt ジョブを
	* JobTaskflow の処理として SeedCore の JobExecutor へ投入する。
	*/
	class JoltExecutorBridge::Impl final :public JPH::JobSystemWithBarrier
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
		explicit Impl(JobExecutor& executor, JPH::uint maxBarriers);

		/**
		* [EN]
		* Returns the hardware concurrency exposed to Jolt's job scheduler.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Jolt のジョブスケジューラーへ公開するハードウェア並行数を返す。
		*/
		Int GetMaxConcurrency()const override;

		/**
		* [EN]
		* Allocates a Jolt job and queues it immediately when it has no dependencies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Jolt ジョブを確保し、依存関係がなければ即座にキューへ投入する。
		*/
		JobHandle CreateJob(const Char* name, JPH::ColorArg color, const JobFunction& function, JPH::uint32 dependencyCount)override;

	protected:
		/**
		* [EN]
		* Submits one Jolt job to the SeedCore executor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1つの Jolt ジョブを SeedCore のエグゼキューターへ投入する。
		*/
		void QueueJob(Job* job)override;

		/**
		* [EN]
		* Submits multiple Jolt jobs in one taskflow.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 複数の Jolt ジョブを1つのタスクフローへ投入する。
		*/
		void QueueJobs(Job** jobs, JPH::uint jobCount)override;

		/**
		* [EN]
		* Deletes a Jolt job whose reference count reached zero.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 参照カウントがゼロになった Jolt ジョブを削除する。
		*/
		void FreeJob(Job* job)override;

	private:
		/// [EN] SeedCore executor that runs submitted Jolt work.
		/// [JP] 投入された Jolt 処理を実行する SeedCore エグゼキューター。
		JobExecutor& executor_;
	};

	/**
	* [EN]
	* Creates a bridge backed by the supplied executor and barrier capacity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定されたエグゼキューターとバリア容量を使うブリッジを生成する。
	*/
	JoltExecutorBridge::Impl::Impl(JobExecutor& executor, JPH::uint maxBarriers) :JPH::JobSystemWithBarrier(maxBarriers), executor_(executor)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns the hardware concurrency exposed to Jolt's job scheduler.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のジョブスケジューラーへ公開するハードウェア並行数を返す。
	*/
	Int JoltExecutorBridge::Impl::GetMaxConcurrency()const
	{
		return static_cast<Int>(std::thread::hardware_concurrency());
	}

	/**
	* [EN]
	* Allocates a Jolt job and queues it immediately when it has no dependencies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt ジョブを確保し、依存関係がなければ即座にキューへ投入する。
	*/
	JoltExecutorBridge::Impl::JobHandle JoltExecutorBridge::Impl::CreateJob(const Char* inName, JPH::ColorArg inColor, const JobFunction& inJobFunction, JPH::uint32 inNumDependencies)
	{
		Job* job = new Job(inName, inColor, this, inJobFunction, inNumDependencies);
		JobHandle handle(job);

		/// [EN] A job with dependencies is queued later by Jolt, once the last one finishes.
		/// [JP] 依存のあるジョブは、最後の依存が終わったときに Jolt が後から投入する。
		if (inNumDependencies == 0)
		{
			QueueJob(job);
		}

		return handle;
	}

	/**
	* [EN]
	* Submits one Jolt job to the SeedCore executor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つの Jolt ジョブを SeedCore のエグゼキューターへ投入する。
	*/
	void JoltExecutorBridge::Impl::QueueJob(Job* inJob)
	{
		/// [EN] The task holds its own reference, released after the job runs, so the job outlives Jolt's handle if needed.
		/// [JP] タスクが自分の参照を持ち、ジョブを実行した後で解放する。これで Jolt 側のハンドルが先に消えてもジョブは残る。
		inJob->AddRef();

		ResourceRef<JobTaskflow> flow = MakeRef<JobTaskflow>();
		flow->emplace([inJob]()
			{
				inJob->Execute();
				inJob->Release();
			});

		/// [EN] The completion callback captures the flow, keeping it alive until the executor is done with it.
		/// [JP] 完了コールバックがフローを捕まえ、エグゼキューターが使い終わるまでフローを生かしておく。
		executor_.Run(*flow, [flow]()
			{
				/// No Code
			});
	}

	/**
	* [EN]
	* Submits multiple Jolt jobs in one taskflow.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 複数の Jolt ジョブを1つのタスクフローへ投入する。
	*/
	void JoltExecutorBridge::Impl::QueueJobs(Job** inJobs, JPH::uint inNumJobs)
	{
		/// [EN] The jobs have no order between them, so they go into one flow as independent tasks.
		/// [JP] ジョブ同士に順序は無いので、独立したタスクとして1つのフローへ入れる。
		ResourceRef<JobTaskflow> flow = MakeRef<JobTaskflow>();

		for (JPH::uint index = 0; index < inNumJobs; ++index)
		{
			/// [EN] Each task holds its own reference, released after its job runs.
			/// [JP] 各タスクが自分の参照を持ち、ジョブを実行した後で解放する。
			Job* inJob = inJobs[index];
			inJob->AddRef();
			flow->emplace([inJob]()
				{
					inJob->Execute();
					inJob->Release();
				});
		}

		/// [EN] The completion callback captures the flow, keeping it alive until the executor is done with it.
		/// [JP] 完了コールバックがフローを捕まえ、エグゼキューターが使い終わるまでフローを生かしておく。
		executor_.Run(*flow, [flow]()
			{
				/// No Code
			});
	}

	/**
	* [EN]
	* Deletes a Jolt job whose reference count reached zero.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 参照カウントがゼロになった Jolt ジョブを削除する。
	*/
	void JoltExecutorBridge::Impl::FreeJob(Job* inJob)
	{
		delete inJob;
	}

	/**
	* [EN]
	* Creates a bridge backed by the supplied executor and barrier capacity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定されたエグゼキューターとバリア容量を使うブリッジを生成する。
	*/
	JoltExecutorBridge::JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers) :impl_(MakePtr<Impl>(executor, static_cast<JPH::uint>(maxBarriers)))
	{
		/// No Code
	}

	/**
	* [EN]
	* Defined here, where Impl is a complete type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Impl が完全型になるここで定義する。
	*/
	JoltExecutorBridge::~JoltExecutorBridge() = default;

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
	JPH::JobSystem* JoltExecutorBridge::Native()
	{
		return impl_.get();
	}
}

#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Core/Color.h>
#include <External/JoltPhysics/Jolt/Core/JobSystemWithBarrier.h>
#include <PhysicsEngine/JoltPhysics/JoltExecutorBridge.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	class JoltExecutorBridge::Impl final :public JPH::JobSystemWithBarrier
	{
	public:
		explicit Impl(JobExecutor& executor, JPH::uint maxBarriers);

		Int GetMaxConcurrency()const override;

		JobHandle CreateJob(const Char* name, JPH::ColorArg color, const JobFunction& function, JPH::uint32 dependencyCount)override;

	protected:
		void QueueJob(Job* job)override;

		void QueueJobs(Job** jobs, JPH::uint jobCount)override;

		void FreeJob(Job* job)override;

	private:
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

	JoltExecutorBridge::JoltExecutorBridge(JobExecutor& executor, Uint32 maxBarriers) :impl_(MakePtr<Impl>(executor, static_cast<JPH::uint>(maxBarriers)))
	{
		/// No Code
	}

	JoltExecutorBridge::~JoltExecutorBridge() = default;

	JPH::JobSystem* JoltExecutorBridge::Native()
	{
		return impl_.get();
	}
}

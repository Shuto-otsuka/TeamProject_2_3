#pragma once
#include <FoundationEngine/JobSystem/JobCommon.h>
#include <FoundationEngine/JobSystem/JobNodeBase.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobTopology.h>
#include <FoundationEngine/JobSystem/Semaphore.h>
#include <FoundationEngine/JobSystem/WorkerCommon.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	class SEEDCORE_API JobNode :public JobNodeBase
	{
	private:
		friend class JobGraph;
		friend class JobTask;
		friend class JobTaskView;
		friend class JobTaskflow;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;

	private:
		using Placeholder = std::monostate;

		struct Static
		{
			template<typename C>
			Static(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<void()> work_;
		};

		struct PreemptiveRuntime
		{
			template<typename C>
			PreemptiveRuntime(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<void(JobPreemptiveRuntime&)> work_;
		};

		struct NonpreemptiveRuntime
		{
			template<typename C>
			NonpreemptiveRuntime(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<void(JobNonpreemptiveRuntime&)> work_;
		};

		struct Subflow
		{
			template<typename C>
			Subflow(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<void(JobSubflow&)> work_;

			JobGraph subgraph_;
		};

		struct SingleCondition
		{
			template <typename C>
			SingleCondition(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<Int()> work_;
		};

		struct MultiCondition
		{
			template <typename C>
			MultiCondition(C&& c) :work_(std::forward<C>(c))
			{
				/// No Code
			}

			std::function<HybridArray<Int>()> work_;
		};

		struct OwnedModule
		{
			OwnedModule(JobGraph& graph);

			JobGraph& graph_;
		};

		struct AdoptedModule
		{
			AdoptedModule(JobGraph&& graph);

			JobGraph graph_;
		};

		using NodeHandle = std::variant
			<
			Placeholder,
			Static,
			PreemptiveRuntime,
			NonpreemptiveRuntime,
			Subflow,
			SingleCondition,
			MultiCondition,
			OwnedModule,
			AdoptedModule
			>;

		struct Semaphores
		{
			HybridArray<Semaphore*> acquire_;

			HybridArray<Semaphore*> release_;
		};

	public:
		SC_CONSTEX auto PLACEHOLDER = GetIndexValue<Placeholder, NodeHandle>;

		SC_CONSTEX auto STATIC = GetIndexValue<Static, NodeHandle>;

		SC_CONSTEX auto PREEMPTIVE_RUNTIME = GetIndexValue<PreemptiveRuntime, NodeHandle>;

		SC_CONSTEX auto NONPREEMPTIVE_RUNTIME = GetIndexValue<NonpreemptiveRuntime, NodeHandle>;

		SC_CONSTEX auto SUBFLOW = GetIndexValue<Subflow, NodeHandle>;

		SC_CONSTEX auto SINGLE_CONDITION = GetIndexValue<SingleCondition, NodeHandle>;

		SC_CONSTEX auto MULTI_CONDITION = GetIndexValue<MultiCondition, NodeHandle>;

		SC_CONSTEX auto OWNED_MODULE = GetIndexValue<OwnedModule, NodeHandle>;

		SC_CONSTEX auto ADOPTED_MODULE = GetIndexValue<AdoptedModule, NodeHandle>;

		JobNode() = default;

		template<typename... Args>
		JobNode(NState nstate, EState estate, const TaskParams& params, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), name_(params.name_), data_(params.data_), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

		template<typename... Args>
		JobNode(NState nstate, EState estate, const DefaultTaskParams& defaultTaskParams, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

		template<StringLike S, typename... Args>
		JobNode(NState nstate, EState estate, S&& name, JobTopology* topology, JobNodeBase* parent, Size joinCounter, Args&&... args) : JobNodeBase(nstate, estate, parent, joinCounter), name_(std::forward<S>(name)), topology_(topology), handle_(std::forward<Args>(args)...)
		{
			/// No Code
		}

		Size NumberSuccessors()const;

		Size NumberPredecessors()const;

		Size NumberStrongDependencies()const;

		Size NumberWeakDependencies()const;

		const String& Name()const;

	private:
		String name_;

		void* data_ = nullptr;

		JobTopology* topology_ = nullptr;

		Size numberSuccessors_ = 0;

		HybridArray<JobNode*, 4> edges_;

		NodeHandle handle_;

		std::unique_ptr<Semaphores> semaphores_;

		Bool ParentCancelled()const;

		Bool Conditioner()const;

		Bool AcquireAll(HybridArray<JobNode*>& nodes);

		void ReleaseAll(HybridArray<JobNode*>& nodes);

		void Precede(JobNode* node);

		void SetUpJoinCounter();

		void RemoveSuccessors(JobNode* node);

		void RemovePredecessors(JobNode* node);
	};

#if SC_ENABLE_TASK_POOL
	using NodePool = std::conditional_t
		<
		std::atomic<SynchronizedPointer>::is_always_lock_free,
		ObjectPool<JobNode, SynchronizedPointer>,
		ObjectPool<JobNode, PackedSynchronizedPointer<>>
		>;

	inline NodePool nodePool_;
#endif

	template<typename... Args>
	__forceinline JobNode* animate(Args&&... args)
	{
#if SC_ENABLE_TASK_POOL
		return nodePool_.Create(std::forward<Args>(args)...);
#else
		return new JobNode(std::forward<Args>(args)...);
#endif
	}

	__forceinline void recycle(JobNode* node)
	{
#if SC_ENABLE_TASK_POOL
		nodePool_.Recycle(node);
#else
		delete node;
#endif
	}
}
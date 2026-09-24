#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/JobSystem/JobNodeBase.h>

namespace SeedCore
{
	/**
	* [EN]
	* Represents a single submitted run of a JobTaskflow's graph. Holds
	* the completion predicate (checked after each pass to decide
	* whether to loop/repeat the graph), the on-finish callback, and the
	* promise that JobFuture<void> observers wait on.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JobTaskflow のグラフに対する、投入済みの単一の実行を表すクラス。
	* 完了述語（各パス後にチェックされ、グラフをループ/再実行するかを
	* 判断する）、完了時コールバック、および JobFuture<void>
	* オブザーバーが待機する promise を保持する。
	*/
	class JobTopology :public JobNodeBase
	{
	private:
		/// [EN] The executor drives the run, and the runtimes and futures read or change its state.
		/// [JP] 実行を進めるのはエグゼキュータで、ランタイムや future がその状態を読み書きする。
		friend class JobExecutor;
		friend class Subflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobNode;

		template<typename T>
		friend class JobFuture;

	public:
		/**
		* [EN]
		* Constructs a topology bound to taskflow, storing predicate
		* (re-run condition) and onFinish (completion callback). The
		* topology starts EXPLICITLY_ANCHORED, since its future is where
		* exceptions from the run are delivered.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* taskflow に紐づいたトポロジーを構築し、predicate（再実行条件）と
		* onFinish（完了時コールバック）を保持する。実行中の例外は future へ
		* 届けるので、トポロジーは最初から EXPLICITLY_ANCHORED にしておく。
		*/
		template<typename Predicate, typename OnFinish>
		JobTopology(JobTaskflow& taskflow, Predicate&& predicate, OnFinish&& onFinish) :JobNodeBase(JobNodeState::NONE, JobExceptionState::EXPLICITLY_ANCHORED, nullptr, 0), taskflow_(taskflow), predicate_(std::forward<Predicate>(predicate)), onFinish_(std::forward<OnFinish>(onFinish))
		{
			/// No Code
		}

		/**
		* [EN]
		* Destroys the topology, and with it the taskflow it owns, if the
		* run was submitted with a moved-in taskflow. Defined in the .cpp,
		* where JobTaskflow is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* トポロジーを破棄する。ムーブで渡されたタスクフローを持っていれば、
		* それも一緒に破棄する。JobTaskflow が完全型になる .cpp で定義する。
		*/
		~JobTopology();

		/**
		* [EN]
		* Returns whether this run has been cancelled or has failed with an
		* exception.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この実行がキャンセルされたか、例外で失敗しているかを返す。
		*/
		Bool Cancelled()const;

	private:
		/// [EN] The taskflow whose graph this topology is a run of.
		/// [JP] このトポロジーがその実行インスタンスとなっている、対象のタスクフロー。
		JobTaskflow& taskflow_;

		/// [EN] Owns the taskflow when it was moved into the run, so it lives exactly as long as the run; empty otherwise.
		/// [JP] ムーブで実行に渡されたタスクフローを所有し、実行とちょうど同じだけ生かす。それ以外では空。
		ResourcePtr<JobTaskflow> ownedTaskflow_;

		/// [EN] Fulfilled when this run finishes, allowing JobFuture<void> observers to unblock.
		/// [JP] この実行が完了した時点で満たされ、JobFuture<void> オブザーバーのブロックを解除する。
		std::promise<void> promise_;

		/// [EN] Evaluated after each pass over the graph; returning true stops the run, false re-runs it.
		/// [JP] グラフの各パス後に評価される。true を返すと実行を停止し、false であれば再実行する。
		std::function<Bool()> predicate_;

		/// [EN] Invoked once this run has fully finished (predicate_ returned true or the run was cancelled), before the promise is fulfilled.
		/// [JP] この実行が完全に終了した時点（predicate_ が true を返した、またはキャンセルされた場合）で、promise を満たす前に呼び出される。
		std::function<void()> onFinish_;

		/**
		* [EN]
		* Fulfills promise_, with the stored exception if the run failed,
		* unblocking any JobFuture<void> observers waiting on this run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* promise_ を満たし（実行が失敗していれば格納された例外で）、この
		* 実行を待機している JobFuture<void> オブザーバーのブロックを解除する。
		*/
		void CarryOutPromise();
	};
}

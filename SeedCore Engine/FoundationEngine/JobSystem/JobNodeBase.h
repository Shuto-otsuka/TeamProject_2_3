#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Represents the execution state of a job node.
	* The upper 4 bits encode control flags, while the lower 28 bits represent
	* the count of strong (unresolved) dependencies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブノードの実行状態を表す構造体。
	* 上位4ビットが制御フラグを、下位28ビットが未解決の強依存関係の数を表す。
	*/
	struct JobNodeState
	{
		/// [EN] Underlying integer type for node state bitfield.
		/// [JP] ノード状態ビットフィールドの基底整数型。
		using NodeStateType = Uint32;

		/// [EN] No flags set; initial default state.
		/// [JP] フラグなし：初期デフォルト状態。
		static constexpr NodeStateType NONE = 0x00000000;

		/// [EN] Set while a runtime task's callable runs; exceptions thrown by tasks it spawned are stored on this node.
		/// [JP] ランタイムタスクの処理の実行中に立つ。そこで生成したタスクが投げた例外は、このノードに格納される。
		static constexpr NodeStateType IMPLICITLY_ANCHORED = 0x10000000;
		
		/// [EN] Node suspended itself to wait for child work, and is run again by the last child to finish.
		/// [JP] ノードが子の処理を待つために自分を中断している。最後に終わった子によって再び実行される。
		static constexpr NodeStateType PREEMPTED = 0x20000000;
		
		/// [EN] Keeps the subflow's graph after the node finishes instead of clearing it.
		/// [JP] ノードが終わった後も、サブフローのグラフを消さずに残す。
		static constexpr NodeStateType RETAIN_SUBFLOW = 0x40000000;
		
		/// [EN] The subflow was already joined inside the callable, so it is not scheduled again afterwards.
		/// [JP] サブフローは処理の中で既に合流済みなので、その後に改めてスケジュールしない。
		static constexpr NodeStateType JOINED_SUBFLOW = 0x80000000;

		/// [EN] Bitmask to extract the strong dependency counter from the state value (lower 28 bits).
		/// [JP] 状態値から強依存関係カウンタを取り出すためのビットマスク（下位28ビット）。
		static constexpr NodeStateType STRONG_DEPENDENCIES_MASK = 0x0FFFFFFF;
	};

	/// [EN] Convenient alias for the node state type.
	/// [JP] ノード状態型の短縮エイリアス。
	using NState = JobNodeState::NodeStateType;

	/**
	* [EN]
	* Represents the exception and cancellation state of a job node or
	* topology, as flags in the upper byte. It is atomic on the node
	* because several workers may set flags at once.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ジョブノードやトポロジーの例外状態とキャンセル状態を、上位バイトの
	* フラグで表す構造体。複数のワーカーが同時にフラグを立てうるため、
	* ノード側ではアトミックに持つ。
	*/
	struct JobExceptionState
	{
		/// [EN] Underlying integer type for exception state bitfield.
		/// [JP] 例外状態ビットフィールドの基底整数型。
		using ExceptionStateType = Uint32;

		/// [EN] No exception has occurred; initial default state.
		/// [JP] 例外なし：初期デフォルト状態。
		static constexpr ExceptionStateType NONE = 0x00000000;

		/// [EN] An exception was thrown by this node or a descendant; remaining work under it is skipped.
		/// [JP] このノードか子孫が例外を投げた。その下の残りの処理は飛ばされる。
		static constexpr ExceptionStateType EXCEPTION = 0x10000000;

		/// [EN] This node already stores an exception, so later ones are not stored here.
		/// [JP] このノードは既に例外を格納しているので、後から来たものはここに格納しない。
		static constexpr ExceptionStateType CAUGHT = 0x20000000;

		/// [EN] The run was cancelled; nodes not yet started are skipped.
		/// [JP] 実行がキャンセルされた。まだ始まっていないノードは飛ばされる。
		static constexpr ExceptionStateType CANCELLED = 0x40000000;

		/// [EN] A caller is blocked on this node (corun/join, or a topology's future), so exceptions from below are stored here.
		/// [JP] 呼び出し側がこのノードで待っている（corun/join、あるいはトポロジーの future）ので、下から来た例外はここに格納する。
		static constexpr ExceptionStateType EXPLICITLY_ANCHORED = 0x80000000;

		/// [EN] Reserved flag; nothing in the job system sets or reads it.
		/// [JP] 予約済みのフラグ。ジョブシステムの中で立てたり読んだりしている箇所は無い。
		static constexpr ExceptionStateType LOCKED = 0x01000000;

		/// [EN] Reserved flag; nothing in the job system sets or reads it.
		/// [JP] 予約済みのフラグ。ジョブシステムの中で立てたり読んだりしている箇所は無い。
		static constexpr ExceptionStateType FINISHED = 0x02000000;

		/// [EN] Bitmask covering all exception state flag bits (upper byte).
		/// [JP] 例外状態フラグビット全体を覆うビットマスク（上位バイト）。
		static constexpr ExceptionStateType MASK = 0xFF000000;
	};

	/// [EN] Convenient alias for the exception state type.
	/// [JP] 例外状態型の短縮エイリアス。
	using EState = JobExceptionState::ExceptionStateType;

	/**
	* [EN]
	* Base class for all job nodes in the execution graph.
	* Holds the core scheduling state, exception state, parent linkage,
	* join counter, and any propagated exception pointer.
	* All graph-related classes are declared as friends so they can
	* manipulate internal state directly without exposing public setters.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 実行グラフ上のすべてのジョブノードの基底クラス。
	* スケジューリング状態・例外状態・親ノードへのリンク・
	* 合流カウンタ・伝播例外ポインタを保持する。
	* グラフ関連クラスはすべてフレンド宣言されており、
	* パブリックなセッターを介さずに内部状態を直接操作できる。
	*/
	class SEEDCORE_API JobNodeBase
	{
	private:
		/// [EN] Graph-related classes manipulate this state directly instead of through public setters.
		/// [JP] グラフ関連のクラスは、public なセッターを通さずにこの状態を直接操作する。
		friend class JobNode;
		friend class JobGraph;
		friend class JobTask;
		friend class JobTaskView;
		friend class JobTaskflow;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class JobSubflow;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobExplicitAnchorGuard;

	protected:
		/// [EN] Scheduling state bitfield (control flags + strong dependency counter).
		/// [JP] スケジューリング状態ビットフィールド（制御フラグ＋強依存関係カウンタ）。
		NState nstate_ = JobNodeState::NONE;

		/// [EN] Exception and cancellation flags; atomic because several workers may set them at once.
		/// [JP] 例外とキャンセルのフラグ。複数のワーカーが同時に立てうるためアトミック。
		std::atomic<EState> estate_ = JobExceptionState::NONE;

		/// [EN] Pointer to the parent node; nullptr if this node is a root.
		/// [JP] 親ノードへのポインタ。ルートノードの場合は nullptr。
		JobNodeBase* parent_ = nullptr;

		/// [EN] For a node waiting to run: predecessors not yet finished. For a parent: children still in flight.
		/// [JP] 実行を待つノードでは、まだ終わっていない先行ノードの数。親としては、まだ実行中の子の数。
		std::atomic<Size> joinCounter_ = 0;

		/// [EN] The first exception thrown by this node or one of its children; nullptr if none.
		/// [JP] このノードかその子が最初に投げた例外。無ければ nullptr。
		std::exception_ptr exceptionPtr_ = nullptr;

	protected:
		/**
		* [EN]
		* Default constructor: leaves every field at its in-class default
		* (unscheduled, no exception, no parent, zero join counter).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: 全フィールドをクラス内デフォルト値の
		* ままにする（未スケジュール、例外なし、親なし、合流カウンタ0）。
		*/
		JobNodeBase() = default;

		/**
		* [EN]
		* Explicit constructor for initializing all fields at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全フィールドを一括初期化するための明示的コンストラクタ。
		*/
		JobNodeBase(NState nstate, EState estate, JobNodeBase* parent, Size joinCounter);

		/**
		* [EN]
		* If an exception has been stored on this node, clears it (and the
		* EXCEPTION/CAUGHT flags) and rethrows it to the caller.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードに例外が格納されていれば、それ（と EXCEPTION/CAUGHT の
		* フラグ）を消してから、呼び出し側へ投げ直す。
		*/
		void RethrowException();
	};

	/**
	* [EN]
	* Scope guard that marks a node as EXPLICITLY_ANCHORED while a caller
	* blocks on it (corun, subflow join), so exceptions thrown by the work
	* it waits for are stored on that node and can be rethrown afterwards.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 呼び出し側がノードで待っている間（corun、サブフローの合流）、その
	* ノードに EXPLICITLY_ANCHORED を立てておくスコープガード。待っている
	* 処理が投げた例外はそのノードに格納され、後で投げ直せる。
	*/
	class SEEDCORE_API JobExplicitAnchorGuard
	{
	public:
		/**
		* [EN]
		* Sets EXPLICITLY_ANCHORED on node.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node に EXPLICITLY_ANCHORED を立てる。
		*/
		explicit JobExplicitAnchorGuard(JobNodeBase* node);

		/**
		* [EN]
		* Clears EXPLICITLY_ANCHORED from the node again.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* node から EXPLICITLY_ANCHORED を再び消す。
		*/
		~JobExplicitAnchorGuard();

		/// [EN] The guard is tied to one scope and one node.
		/// [JP] ガードは1つのスコープと1つのノードに結びついている。
		JobExplicitAnchorGuard(const JobExplicitAnchorGuard&) = delete;
		JobExplicitAnchorGuard& operator=(const JobExplicitAnchorGuard&) = delete;

	private:
		/// [EN] The node anchored for the lifetime of this guard.
		/// [JP] このガードが生きている間、アンカーにしているノード。
		JobNodeBase* node_ = nullptr;
	};
}
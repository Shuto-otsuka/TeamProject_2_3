#pragma once
#include <FoundationEngine/JobSystem/WorkerCommon.h>

/**
* [EN]
* Forward declarations for every job-system type, shared so headers can
* reference each other by pointer/reference without pulling in the
* whole job-system include graph. See each type's own header for its
* actual documentation.
*
* ---------------------------------------------------------------------
*
* [JP]
* ジョブシステムの全型に対する前方宣言。各ヘッダがジョブシステムの
* include グラフ全体を引き込まずに、ポインタ/参照で互いを参照できる
* ようにするために共有される。実際のドキュメントは各型自身のヘッダを参照。
*/
namespace SeedCore
{
	/// [EN] The graph itself: nodes, the graph that owns them, and the builder that adds them.
	/// [JP] グラフそのもの。ノード、それを所有するグラフ、ノードを足していくビルダー。
	class JobNode;
	class JobGraph;
	class FlowBuilder;

	/// [EN] Limits how many tasks of one kind run at once.
	/// [JP] ある種類のタスクが同時に走る数を制限する。
	class Semaphore;

	/// [EN] Graphs spawned or run from inside a running task.
	/// [JP] 実行中のタスクの中から生成・実行されるグラフ。
	class JobSubflow;
	class JobPreemptiveRuntime;
	class JobNonpreemptiveRuntime;

	/// [EN] User-facing handles to nodes and whole taskflows, and one run of a taskflow.
	/// [JP] ノードやタスクフロー全体を扱う利用者向けのハンドルと、タスクフローの1回分の実行。
	class JobTask;
	class JobTaskView;
	class JobTaskflow;
	class JobTopology;

	/// [EN] The thread pool that runs everything, and its worker threads.
	/// [JP] 全てを実行するスレッドプールと、そのワーカースレッド。
	class JobExecutor;
	class JobWorker;
	class JobWorkerView;

	/// [EN] Result handle returned when a taskflow is run.
	/// [JP] タスクフローを実行したときに返る、結果を待つためのハンドル。
	template <typename T>
	class JobFuture;

	/// [EN] Per-task parameters passed when creating async tasks.
	/// [JP] 非同期タスクを作るときに渡すタスクごとのパラメータ。
	class TaskParams;
	class DefaultTaskParams;
}
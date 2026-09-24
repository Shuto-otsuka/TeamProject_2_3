#pragma once

namespace SeedCore
{
	/// [EN] Non-zero allocates JobNode instances from a pool instead of plain new/delete.
	/// [JP] 非ゼロなら、JobNode を new/delete ではなくプールから確保する。
#define SC_ENABLE_TASK_POOL 0

	/// [EN] Non-zero makes worker threads park and wake through AtomicNotifier instead of NonblockingNotifier.
	/// [JP] 非ゼロなら、ワーカースレッドの待機と起床に NonblockingNotifier ではなく AtomicNotifier を使う。
#define SC_ENABLE_ATOMIC_NOTIFIER 0

	/// [EN] Non-zero skips the try/catch around task execution: less overhead, but an exception escaping a task calls std::terminate.
	/// [JP] 非ゼロならタスク実行時の try/catch を省く。負荷は下がるが、タスクから漏れた例外は std::terminate になる。
#define SC_DISABLE_EXCEPTION_HANDLING 0
}
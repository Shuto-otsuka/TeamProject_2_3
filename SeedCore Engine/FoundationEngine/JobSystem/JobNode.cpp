#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs, storing a reference to the externally-owned graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 外部が所有するグラフへの参照を保持して構築する。
	*/
	JobNode::OwnedModule::OwnedModule(JobGraph& graph) :graph_(graph)
	{
		/// No Code
	}

	/**
	* [EN]
	* Constructs, moving graph into this module's ownership.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* graph をこのモジュールの所有権へムーブして構築する。
	*/
	JobNode::AdoptedModule::AdoptedModule(JobGraph&& graph) :graph_(std::move(graph))
	{
		/// No Code
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the number of successor nodes connected to this node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードに接続されている後続ノードの数を返す。
	*/
	Size JobNode::NumberSuccessors()const
	{
		return numberSuccessors_;
	}

	/**
	* [EN]
	* Returns the number of predecessor nodes connected to this node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードに接続されている先行ノードの数を返す。
	*/
	Size JobNode::NumberPredecessors()const
	{
		/// [EN] edges_ stores successors in the front portion and predecessors in the remainder, so predecessor count is the total minus the successor count.
		/// [JP] edges_ は前方に後続、残りに先行を格納しているため、先行ノード数は全体数から後続ノード数を引いた値になる。
		return edges_.size() - numberSuccessors_;
	}

	/**
	* [EN]
	* Returns the number of strong dependencies: predecessors that are
	* not condition nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 強い依存関係の数を返す。条件ノードではない先行ノードのこと。
	*/
	Size JobNode::NumberStrongDependencies()const
	{
		/// [EN] Walks the predecessor part of edges_ and counts the predecessors that are not condition nodes.
		/// [JP] edges_ の先行ノード部分を辿り、条件ノードではない先行ノードを数える。
		Size n = 0;
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			n += !edges_[index]->Conditioner();
		}
		return n;
	}

	/**
	* [EN]
	* Returns the number of weak dependencies: predecessors that are
	* condition nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 弱い依存関係の数を返す。条件ノードである先行ノードのこと。
	*/
	Size JobNode::NumberWeakDependencies()const
	{
		/// [EN] Walks the predecessor part of edges_ and counts the predecessors that are condition nodes.
		/// [JP] edges_ の先行ノード部分を辿り、条件ノードである先行ノードを数える。
		Size n = 0;
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			n += edges_[index]->Conditioner();
		}
		return n;
	}

	/**
	* [EN]
	* Returns this node's display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードの表示名を返す。
	*/
	const String& JobNode::Name()const
	{
		return name_;
	}

	/**
	* [EN]
	* Returns whether this node's topology or parent node has been
	* cancelled or has failed with an exception, in which case this
	* node is skipped instead of run.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードのトポロジーか親ノードが、キャンセルされたか例外で失敗
	* しているかを返す。その場合、このノードは実行されずに飛ばされる。
	*/
	Bool JobNode::ParentCancelled()const
	{
		/// [EN] An exception counts as cancellation too, so the rest of a failed run is skipped rather than executed.
		/// [JP] 例外もキャンセルとして扱う。失敗した実行の残りは、実行されずに飛ばされる。
		return (topology_ && topology_->estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION)) || (parent_ && (parent_->estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION)));
	}

	/**
	* [EN]
	* Returns whether this node represents a conditional branch (i.e.
	* its handle is SingleCondition or MultiCondition).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードが条件分岐を表すかどうか（すなわちハンドルが
	* SingleCondition または MultiCondition であるか）を返す。
	*/
	Bool JobNode::Conditioner()const
	{
		return handle_.index() == JobNode::SINGLE_CONDITION || handle_.index() == JobNode::MULTI_CONDITION;
	}

	/**
	* [EN]
	* Takes every semaphore this node must hold before it runs, in
	* order. If one is not free, this node is parked on it, the ones
	* already taken are given back and false is returned.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードが実行前に持つべきセマフォを、順に全て取る。どれかに
	* 空きが無ければ、このノードをそこで待機させ、既に取った分を返し、
	* false を返す。
	*/
	Bool JobNode::AcquireAll(HybridArray<JobNode*>& nodes)
	{
		/// [EN] All or nothing: holding some semaphores while waiting for another could deadlock two nodes against each other.
		/// [JP] 全部取るか、何も持たないか。一部を持ったまま別のものを待つと、2つのノードが互いを待って止まりうる。
		auto& acquire = semaphores_->acquire_;
		for (Size index = 0;index < acquire.size();++index)
		{
			if (!acquire[index]->try_acquire_or_wait(this))
			{
				/// [EN] Acquisition failed partway through: roll back by releasing every semaphore successfully acquired so far (in reverse order), then report overall failure.
				/// [JP] 途中で獲得に失敗したため、それまでに獲得済みのセマフォを（逆順に）すべて解放してロールバックし、全体としての失敗を報告する。
				for (Size rollbackIndex = 1;rollbackIndex <= index;++rollbackIndex)
				{
					acquire[index - rollbackIndex]->release(nodes);
				}
				return false;
			}
		}
		return true;
	}

	/**
	* [EN]
	* Gives back every semaphore this node releases after running,
	* collecting the tasks that were waiting on them into nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードが実行後に解放するセマフォを全て返し、それらを待って
	* いたタスクを nodes へ集める。
	*/
	void JobNode::ReleaseAll(HybridArray<JobNode*>& nodes)
	{
		/// [EN] The waiters of every semaphore are gathered into one list, so the caller schedules them in a single batch.
		/// [JP] 全セマフォの待機者を1つのリストへ集め、呼び出し側が1回でまとめてスケジュールできるようにする。
		auto& release = semaphores_->release_;
		for (Semaphore* semaphore : release)
		{
			semaphore->release(nodes);
		}
	}

	/**
	* [EN]
	* Adds an edge from this node to node: node becomes a successor
	* here, and this node becomes a predecessor there.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードから node へのエッジを足す。こちらでは node が後続に、
	* あちらではこのノードが先行ノードになる。
	*/
	void JobNode::Precede(JobNode* node)
	{
		/// [EN] Add node to the end of edges_, then swap it into position at index numberSuccessors_ (extending the successor region by one) so successors stay contiguous at the front.
		/// [JP] node を edges_ の末尾に追加し、それをインデックス numberSuccessors_ の位置にスワップする（後続領域を 1 つ拡張する）ことで、後続ノードが先頭に連続して並ぶようにする。
		edges_.push_back(node);
		std::swap(edges_[numberSuccessors_++], edges_[edges_.size() - 1]);

		/// [EN] Register this as a predecessor of node (its reciprocal edge).
		/// [JP] this を node の先行ノードとして登録する（相互エッジ）。
		node->edges_.push_back(this);
	}

	/**
	* [EN]
	* Adds the number of strong dependencies to nstate_ and sets the
	* join counter to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 強い依存関係の数を nstate_ に足し、join カウンタをその値にする。
	*/
	void JobNode::SetUpJoinCounter()
	{
		/// [EN] The count lives in the low bits of nstate_, so Invoke can restore the counter after each run without recounting.
		/// [JP] 数は nstate_ の下位ビットに置く。Invoke は実行のたびに数え直さずにカウンタを戻せる。
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			nstate_ += !edges_[index]->Conditioner();
		}

		/// [EN] Initialize the join counter from the strong-dependency bits of nstate_, so the node becomes runnable once that many strong predecessors have completed.
		/// [JP] nstate_ の強い依存関係を表すビットから join カウンタを初期化し、その数の強い先行ノードが完了した時点でこのノードが実行可能になるようにする。
		joinCounter_.store(nstate_ & JobNodeState::STRONG_DEPENDENCIES_MASK, std::memory_order_relaxed);
	}

	/**
	* [EN]
	* Removes every edge to node from this node's successors.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードの後続から、node へのエッジを全て取り除く。
	*/
	void JobNode::RemoveSuccessors(JobNode* node)
	{
		/// [EN] Remove node from the successor region [begin, begin + numberSuccessors_) using erase-remove, tracking the new successor count.
		/// [JP] erase-remove を用いて後続領域 [begin, begin + numberSuccessors_) から node を削除し、新しい後続ノード数を記録する。
		auto sit = std::remove(edges_.begin(), edges_.begin() + numberSuccessors_, node);
		Size newNumberSuccessor = std::distance(edges_.begin(), sit);

		/// [EN] Shift the predecessor region left to close the gap left by the removed successor(s), then shrink the container and update the successor count.
		/// [JP] 削除された後続ノード分の隙間を詰めるため、先行ノード領域を前方へ詰め、コンテナを縮小して後続ノード数を更新する。
		std::move(edges_.begin() + numberSuccessors_, edges_.end(), sit);
		edges_.resize(edges_.size() - (numberSuccessors_ - newNumberSuccessor));
		numberSuccessors_ = newNumberSuccessor;
	}

	/**
	* [EN]
	* Removes every edge from node out of this node's predecessors.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードの先行ノードから、node からのエッジを全て取り除く。
	*/
	void JobNode::RemovePredecessors(JobNode* node)
	{
		/// [EN] Remove node from the predecessor region [begin + numberSuccessors_, end) using erase-remove; the successor region is untouched.
		/// [JP] erase-remove を用いて先行ノード領域 [begin + numberSuccessors_, end) から node を削除する。後続ノード領域には影響しない。
		edges_.erase(std::remove(edges_.begin() + numberSuccessors_, edges_.end(), node), edges_.end());
	}
}

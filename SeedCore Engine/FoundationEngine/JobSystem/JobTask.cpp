#include <FoundationEngine/JobSystem/JobTask.h>
#include <FoundationEngine/JobSystem/Semaphore.h>

namespace SeedCore
{
	/**
	* [EN]
	* Copy-constructs, pointing at the same underlying node as rhs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* rhs と同じ内部ノードを指すようにコピー構築する。
	*/
	JobTask::JobTask(const JobTask& rhs) :node_(rhs.node_)
	{
		/// No Code
	}

	/**
	* [EN]
	* Constructs a handle wrapping node directly.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node を直接包むハンドルを構築する。
	*/
	JobTask::JobTask(JobNode* node) :node_(node)
	{
		/// No Code
	}

	/**
	* [EN]
	* Copy-assigns, pointing at the same underlying node as rhs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* rhs と同じ内部ノードを指すようにコピー代入する。
	*/
	JobTask& JobTask::operator=(const JobTask& rhs)
	{
		node_ = rhs.node_;
		return *this;
	}

	/**
	* [EN]
	* Resets this handle to point at no node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このハンドルをどのノードも指さない状態にリセットする。
	*/
	JobTask& JobTask::operator=(std::nullptr_t null)
	{
		/// [EN] Only the handle is cleared; the node stays in its graph.
		/// [JP] 消すのはハンドルだけで、ノードはグラフに残る。
		node_ = null;
		return *this;
	}

	/**
	* [EN]
	* Returns whether this and rhs refer to the same underlying node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* this と rhs が同じ内部ノードを指しているかどうかを返す。
	*/
	Bool JobTask::operator==(const JobTask& rhs)const
	{
		return node_ == rhs.node_;
	}

	/**
	* [EN]
	* Negation of operator==.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* operator== の否定。
	*/
	Bool JobTask::operator!=(const JobTask& rhs)const
	{
		return node_ != rhs.node_;
	}

	/**
	* [EN]
	* Returns the underlying node's display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードの表示名を返す。
	*/
	const String& JobTask::Name()const
	{
		return node_->name_;
	}

	/**
	* [EN]
	* Returns the number of successor tasks connected to this task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクに接続されている後続タスクの数を返す。
	*/
	Size JobTask::NumberSuccessors()const
	{
		return node_->NumberSuccessors();
	}

	/**
	* [EN]
	* Returns the number of predecessor tasks connected to this task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクに接続されている先行タスクの数を返す。
	*/
	Size JobTask::NumberPredecessors()const
	{
		return node_->NumberPredecessors();
	}

	/**
	* [EN]
	* Returns the number of strong (unconditional) dependencies this task has.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクが持つ強い（無条件の）依存関係の数を返す。
	*/
	Size JobTask::NumberStrongDependencies()const
	{
		return node_->NumberStrongDependencies();
	}

	/**
	* [EN]
	* Returns the number of weak (conditional-branch-only) dependencies this task has.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクが持つ弱い（条件分岐経由のみの）依存関係の数を返す。
	*/
	Size JobTask::NumberWeakDependencies()const
	{
		return node_->NumberWeakDependencies();
	}

	/**
	* [EN]
	* Sets the underlying node's display name and returns *this for chaining.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードの表示名を設定し、メソッドチェーン用に *this を返す。
	*/
	JobTask& JobTask::Name(const String& name)
	{
		node_->name_ = name;
		return *this;
	}

	/**
	* [EN]
	* Turns this task into a module that takes ownership of graph (moved
	* in). Returns *this for chaining.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* graph の所有権を（ムーブで）引き受けるモジュールへこのタスクを
	* 変換する。メソッドチェーン用に *this を返す。
	*/
	JobTask& JobTask::Adopt(JobGraph&& graph)
	{
		/// [EN] Replacing the handle alternative destroys the previous work, including a previously adopted graph.
		/// [JP] ハンドルの選択肢を置き換えると、以前に引き受けたグラフも含めて前の処理が破棄される。
		node_->handle_.emplace<JobNode::AdoptedModule>(std::move(graph));
		return *this;
	}

	/**
	* [EN]
	* Sets the underlying node's user-data pointer and returns *this for chaining.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードのユーザーデータポインタを設定し、メソッドチェーン用に
	* *this を返す。
	*/
	JobTask& JobTask::Data(void* data)
	{
		node_->data_ = data;
		return *this;
	}

	/**
	* [EN]
	* Resets this handle to point at no node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このハンドルをどのノードも指さない状態にリセットする。
	*/
	void JobTask::Reset()
	{
		/// [EN] Same as assigning nullptr: the node keeps its work, edges and semaphores.
		/// [JP] nullptr の代入と同じ。ノードは処理・エッジ・セマフォをそのまま持つ。
		node_ = nullptr;
	}

	/**
	* [EN]
	* Clears only the assigned work (NodeHandle), leaving dependencies
	* and semaphores intact.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 割り当てられた処理（NodeHandle）のみをクリアし、依存関係と
	* セマフォはそのまま維持する。
	*/
	void JobTask::ResetWork()
	{
		/// [EN] std::monostate is the Placeholder alternative, so the node runs no work from now on.
		/// [JP] std::monostate は Placeholder の選択肢なので、ノードはこれ以降何も実行しない。
		node_->handle_.emplace<std::monostate>();
	}

	/**
	* [EN]
	* Returns whether this handle points at no node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このハンドルがどのノードも指していないかどうかを返す。
	*/
	Bool JobTask::Empty()const
	{
		return node_ == nullptr;
	}

	/**
	* [EN]
	* Returns whether this task has been assigned any work (its
	* NodeHandle is not the Placeholder alternative).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクに何らかの処理が割り当てられているか（NodeHandle が
	* Placeholder 以外であるか）を返す。
	*/
	Bool JobTask::HasWork()const
	{
		/// [EN] Index 0 is the Placeholder alternative; an empty handle has no work either.
		/// [JP] インデックス 0 は Placeholder の選択肢。空のハンドルも処理を持たないとみなす。
		return node_ ? node_->handle_.index() != 0 : false;
	}

	/**
	* [EN]
	* Returns a hash value identifying the underlying node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードを識別するハッシュ値を返す。
	*/
	Size JobTask::HashValue()const
	{
		/// [EN] Hashing the address makes two handles to the same node hash alike.
		/// [JP] アドレスをハッシュするので、同じノードを指す2つのハンドルは同じ値になる。
		return std::hash<JobNode*>{}(node_);
	}

	/**
	* [EN]
	* Returns the kind of work this task currently performs, derived
	* from the underlying node's active NodeHandle alternative.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードの現在有効な NodeHandle の選択肢から導出される、この
	* タスクが現在行っている処理の種類を返す。
	*/
	JobTaskType JobTask::Type()const
	{
		/// [EN] Preemptive/non-preemptive, single/multi condition and owned/adopted module each collapse into one public kind.
		/// [JP] プリエンプティブ/非プリエンプティブ、単一/複数の条件、所有/養子のモジュールは、それぞれ公開用の1種類にまとめる。
		switch (node_->handle_.index())
		{
		case JobNode::PLACEHOLDER:
			return JobTaskType::PLACEHOLDER;
		case JobNode::STATIC:
			return JobTaskType::STATIC;
		case JobNode::PREEMPTIVE_RUNTIME:
			[[fallthrough]];
		case JobNode::NONPREEMPTIVE_RUNTIME:
			return JobTaskType::RUNTIME;
		case JobNode::SUBFLOW:
			return JobTaskType::SUBFLOW;
		case JobNode::SINGLE_CONDITION:
			[[fallthrough]];
		case JobNode::MULTI_CONDITION:
			return JobTaskType::CONDITION;
		case JobNode::OWNED_MODULE:
			[[fallthrough]];
		case JobNode::ADOPTED_MODULE:
			return JobTaskType::MODULE;
		default:
			return JobTaskType::UNDEFINED;
		}
	}

	/**
	* [EN]
	* Registers semaphore to be released once this task finishes
	* executing. Returns *this for chaining.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクの実行完了時に解放されるセマフォとして semaphore を
	* 登録する。メソッドチェーン用に *this を返す。
	*/
	JobTask& JobTask::Release(Semaphore& semaphore)
	{
		/// [EN] The semaphore block is created only once a task actually uses one.
		/// [JP] セマフォ用のまとまりは、実際に使うタスクで初めて作る。
		if (!node_->semaphores_)
		{
			node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
		}
		node_->semaphores_->release_.push_back(&semaphore);
		return *this;
	}

	/**
	* [EN]
	* Registers semaphore to be acquired before this task may execute.
	* Returns *this for chaining.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクが実行可能になる前に獲得すべきセマフォとして
	* semaphore を登録する。メソッドチェーン用に *this を返す。
	*/
	JobTask& JobTask::Acquire(Semaphore& semaphore)
	{
		/// [EN] The semaphore block is created only once a task actually uses one.
		/// [JP] セマフォ用のまとまりは、実際に使うタスクで初めて作る。
		if (!node_->semaphores_)
		{
			node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
		}
		node_->semaphores_->acquire_.push_back(&semaphore);
		return *this;
	}

	/**
	* [EN]
	* Returns the underlying node's user-data pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードのユーザーデータポインタを返す。
	*/
	void* JobTask::Data()const
	{
		return node_->data_;
	}

	/**
	* [EN]
	* Returns the exception stored on this task's node, if any; nullptr
	* for an empty handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクのノードに格納された例外があれば、それを返す。空の
	* ハンドルでは nullptr。
	*/
	std::exception_ptr JobTask::ExceptionPtr()const
	{
		return node_ ? node_->exceptionPtr_ : nullptr;
	}

	/**
	* [EN]
	* Returns whether an exception has been propagated to this task.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このタスクに例外が伝播しているかどうかを返す。
	*/
	Bool JobTask::HasExceptionPtr()const
	{
		return node_ ? (node_->exceptionPtr_ != nullptr) : false;
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a view referring to node directly.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* node を直接参照するビューを構築する。
	*/
	JobTaskView::JobTaskView(const JobNode& node) :node_(node)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns the underlying node's display name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードの表示名を返す。
	*/
	const String& JobTaskView::Name()const
	{
		return node_.name_;
	}

	/**
	* [EN]
	* Returns the number of successor nodes connected to this node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードに接続されている後続ノードの数を返す。
	*/
	Size JobTaskView::NumberSuccessors()const
	{
		return node_.NumberSuccessors();
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
	Size JobTaskView::NumberPredecessors()const
	{
		return node_.NumberPredecessors();
	}

	/**
	* [EN]
	* Returns the number of strong (unconditional) dependencies this node has.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードが持つ強い（無条件の）依存関係の数を返す。
	*/
	Size JobTaskView::NumberStrongDependencies()const
	{
		return node_.NumberStrongDependencies();
	}

	/**
	* [EN]
	* Returns the number of weak (conditional-branch-only) dependencies this node has.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードが持つ弱い（条件分岐経由のみの）依存関係の数を返す。
	*/
	Size JobTaskView::NumberWeakDependencies()const
	{
		return node_.NumberWeakDependencies();
	}

	/**
	* [EN]
	* Returns the kind of work this node currently performs, derived
	* from the underlying node's active NodeHandle alternative.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードの現在有効な NodeHandle の選択肢から導出される、この
	* ノードが現在行っている処理の種類を返す。
	*/
	JobTaskType JobTaskView::Type()const
	{
		/// [EN] Preemptive/non-preemptive, single/multi condition and owned/adopted module each collapse into one public kind.
		/// [JP] プリエンプティブ/非プリエンプティブ、単一/複数の条件、所有/養子のモジュールは、それぞれ公開用の1種類にまとめる。
		switch (node_.handle_.index())
		{
		case JobNode::PLACEHOLDER:
			return JobTaskType::PLACEHOLDER;
		case JobNode::STATIC:
			return JobTaskType::STATIC;
		case JobNode::PREEMPTIVE_RUNTIME:
			[[fallthrough]];
		case JobNode::NONPREEMPTIVE_RUNTIME:
			return JobTaskType::RUNTIME;
		case JobNode::SUBFLOW:
			return JobTaskType::SUBFLOW;
		case JobNode::SINGLE_CONDITION:
			[[fallthrough]];
		case JobNode::MULTI_CONDITION:
			return JobTaskType::CONDITION;
		case JobNode::OWNED_MODULE:
			[[fallthrough]];
		case JobNode::ADOPTED_MODULE:
			return JobTaskType::MODULE;
		default:
			return JobTaskType::UNDEFINED;
		}
	}

	/**
	* [EN]
	* Returns a hash value identifying the underlying node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 内部ノードを識別するハッシュ値を返す。
	*/
	Size JobTaskView::HashValue()const
	{
		return std::hash<const JobNode*>{}(&node_);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the display name of type as a null-terminated string;
	* UNDEFINED and any unknown value give "Undefined".
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* type の表示名をヌル終端文字列として返す。UNDEFINED や未知の値では
	* "Undefined" を返す。
	*/
	const Char* ToString(JobTaskType type)
	{
		/// [EN] A switch keeps each name tied to its enumerator, independent of the enumerators' numeric order.
		/// [JP] switch にすることで、各名前が列挙子と直接結びつき、列挙子の数値の並びに左右されない。
		switch (type)
		{
		case JobTaskType::PLACEHOLDER:
			return "Placeholder";
		case JobTaskType::STATIC:
			return "Static";
		case JobTaskType::RUNTIME:
			return "Runtime";
		case JobTaskType::SUBFLOW:
			return "Subflow";
		case JobTaskType::CONDITION:
			return "Condition";
		case JobTaskType::MODULE:
			return "Module";
		default:
			return "Undefined";
		}
	}
}

#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	/**
	* [EN]
	* Destroys the graph, releasing all owned nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフを破棄し、所有しているすべてのノードを解放する。
	*/
	JobGraph::~JobGraph()
	{
		clear();
	}

	/**
	* [EN]
	* Move-constructs the graph, transferring ownership of the nodes
	* from other.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフをムーブ構築し、other からノードの所有権を移譲する。
	*/
	JobGraph::JobGraph(JobGraph&& other) :nodes_(std::move(other.nodes_))
	{
		/// No Code
	}

	/**
	* [EN]
	* Move-assigns the graph, transferring ownership of the nodes from
	* other and releasing any previously owned nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフをムーブ代入し、other からノードの所有権を移譲し、
	* それ以前に所有していたノードを解放する。
	*/
	JobGraph& JobGraph::operator=(JobGraph&& other)
	{
		/// [EN] Release any nodes currently owned by *this before taking ownership of other's nodes.
		/// [JP] other のノードの所有権を受け取る前に、*this が現在所有しているノードを解放する。
		clear();
		nodes_ = std::move(other.nodes_);
		return *this;
	}

	/**
	* [EN]
	* Removes and destroys all nodes currently owned by the graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフが現在所有しているすべてのノードを削除し、破棄する。
	*/
	void JobGraph::clear()
	{
		/// [EN] Every node is released (to the pool or with delete) before the pointers themselves are dropped.
		/// [JP] ポインタそのものを捨てる前に、全ノードを（プールへ、あるいは delete で）解放する。
		for (JobNode* node : nodes_)
		{
			recycle(node);
		}
		nodes_.clear();
	}

	/**
	* [EN]
	* Returns the number of nodes currently in the graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフ内に現在存在するノードの数を返す。
	*/
	Size JobGraph::size()const
	{
		return nodes_.size();
	}

	/**
	* [EN]
	* Returns whether the graph currently has no nodes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフが現在ノードを持たないかどうかを返す。
	*/
	Bool JobGraph::empty()const
	{
		return nodes_.empty();
	}

	/**
	* [EN]
	* Returns a mutable iterator to the first node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 先頭ノードを指す、変更可能なイテレータを返す。
	*/
	JobGraph::Iterator JobGraph::begin()
	{
		return nodes_.begin();
	}

	/**
	* [EN]
	* Returns a mutable iterator to one past the last node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 末尾ノードの次を指す、変更可能なイテレータを返す。
	*/
	JobGraph::Iterator JobGraph::end()
	{
		return nodes_.end();
	}

	/**
	* [EN]
	* Returns a const iterator to the first node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 先頭ノードを指す、読み取り専用のイテレータを返す。
	*/
	JobGraph::ConstIterator JobGraph::begin()const
	{
		return nodes_.begin();
	}

	/**
	* [EN]
	* Returns a const iterator to one past the last node.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 末尾ノードの次を指す、読み取り専用のイテレータを返す。
	*/
	JobGraph::ConstIterator JobGraph::end()const
	{
		return nodes_.end();
	}

	/**
	* [EN]
	* Removes and destroys a single node from the graph.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフから単一の node を削除し、破棄する。
	*/
	void JobGraph::erase(JobNode* node)
	{
		/// [EN] The predicate releases the matching node as it goes, and erase_if then drops its pointer from the list.
		/// [JP] 述語が一致したノードをその場で解放し、その後 erase_if がリストからそのポインタを取り除く。
		SeedCore::erase_if(nodes_, [&](auto& p)
			{
				if (p == node)
				{
					recycle(p);
					return true;
				}
				return false;
			});
	}
}

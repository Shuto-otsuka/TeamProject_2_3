#include <FoundationEngine/Interop/QueryInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* Records one query.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* クエリを1つ記録する。
	*/
	void QueryInstance::Add(const QueryDesc& query)
	{
		queries_.push_back(query);
	}

	/**
	* [EN]
	* Returns the queries recorded since the last Clear.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の Clear から記録したクエリを返す。
	*/
	std::span<const QueryDesc> QueryInstance::Queries()const
	{
		return queries_;
	}

	/**
	* [EN]
	* Forgets every recorded query.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 記録したクエリをすべて捨てる。
	*/
	void QueryInstance::Clear()
	{
		queries_.clear();
	}
}

#include <FoundationEngine/World/Tag/TagRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns tag's existing bit index, registering a new one if tag has
	* not been seen before. An empty tag is never registered, since an
	* empty name marks a removed slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* tag の既存のビットインデックスを返す。tag がまだ登録されていなければ、
	* 新しく登録する。空の名前は削除済みの位置を表すので、空の tag は
	* 登録しない。
	*/
	Size TagRegistry::GetOrCreate(String tag)
	{
		if (tag == String{})
		{
			return static_cast<Size>(-1);
		}

		auto it = indices_.find(tag);
		if (it != indices_.end())
		{
			return it->second;
		}

		/// [EN] New tag: append it to the end of names_, assigning the next bit index; the slots of removed tags are never reused.
		/// [JP] 新規タグ: names_ の末尾へ追加し、次のビットインデックスを割り当てる。削除済みタグの位置は使い回さない。
		Size index = names_.size();
		names_.push_back(tag);
		indices_[tag] = index;
		return index;
	}

	/**
	* [EN]
	* Returns tag's bit index, or the largest Size value if tag has not
	* been registered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* tag のビットインデックスを返す。tag が未登録であれば Size の
	* 最大値を返す。
	*/
	Size TagRegistry::Find(String tag)
	{
		auto it = indices_.find(tag);
		if (it != indices_.end())
		{
			return it->second;
		}
		return static_cast<Size>(-1);
	}

	/**
	* [EN]
	* Removes tag by emptying its slot's name; its bit index is not
	* reassigned or reused, so existing actors' bits remain valid.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* tag の位置の名前を空にして削除する。そのビットインデックスは
	* 再割り当て・再利用されないため、既存の Actor のビットは有効な
	* まま維持される。
	*/
	void TagRegistry::Remove(String tag)
	{
		auto it = indices_.find(tag);
		if (it == indices_.end())
		{
			return;
		}

		names_[it->second] = String{};
		indices_.erase(it);
	}

	/**
	* [EN]
	* Returns every registered tag name, indexed by bit index; a removed
	* tag's slot holds an empty String.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 登録済みの全タグ名を、ビットインデックスでアクセスできる形で返す。
	* 削除済みタグの位置は空の String になる。
	*/
	std::span<const String> TagRegistry::Names()
	{
		return std::span<const String>(names_.data(), names_.size());
	}
}

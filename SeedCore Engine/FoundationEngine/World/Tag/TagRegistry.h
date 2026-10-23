#pragma once
#include <FoundationEngine/World/Actor/ActorRegistry.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	/**
	* [EN]
	* Global registry mapping tag names to stable bit indices. Any Actor
	* can hold any number of tags by setting the corresponding bit in its
	* ActorRecord (the first 64 inline, the rest in an extra Bitset).
	* Removing a tag never reassigns or shifts existing bit indices; its
	* slot's name just becomes empty, so other actors' bits stay valid.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* タグ名とビットインデックスを対応付けるグローバルレジストリ。
	* 各 Actor は ActorRecord の対応するビットを立てることで（先頭 64 個は
	* レコードの中、それ以降は追加の Bitset）、個数・種類を問わず任意の
	* タグを保持できる。タグを削除しても既存のビットインデックスはずらさず、
	* その位置の名前を空にするだけなので、他の Actor のビットは壊れない。
	*/
	class SEEDCORE_API TagRegistry
	{
	public:
		/**
		* [EN]
		* Returns tag's existing bit index, registering a new one if tag
		* has not been seen before. An empty tag is never registered, since
		* an empty name marks a removed slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* tag の既存のビットインデックスを返す。tag がまだ登録されて
		* いなければ、新しく登録する。空の名前は削除済みの位置を表すので、
		* 空の tag は登録しない。
		*/
		static Size GetOrCreate(String tag);

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
		static Size Find(String tag);

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
		static void Remove(String tag);

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
		static std::span<const String> Names();

		/**
		* [EN]
		* Calls function with the name of every tag record holds, skipping
		* removed tags. Only the set bits are visited, so the cost follows how
		* many tags the actor has, not how many exist, and no name is looked up.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* record が持つ全タグの名前で function を呼ぶ。削除済みのタグは飛ばす。
		* 立っているビットだけを見るので、費用は存在するタグの数ではなく、その
		* actor が持つタグの数に比例し、名前を引くことも無い。
		*/
		template<typename Function>
		static void ForEach(const ActorRecord& record, Function function)
		{
			std::span<const String> names = Names();

			/// [EN] The first 64 tags: each pass takes the lowest set bit's position, then clears that bit with bits & (bits - 1). An empty name marks a removed tag.
			/// [JP] 先頭 64 個: 毎回、一番下の立ったビットの位置を取り、bits & (bits - 1) でそのビットを消す。名前が空なら削除済みのタグ。
			for (Uint64 bits = record.tags_;bits != 0;bits &= bits - 1)
			{
				Size index = static_cast<Size>(std::countr_zero(bits));
				if (names[index] != String{})
				{
					function(names[index]);
				}
			}

			/// [EN] Tags beyond the first 64 exist only when such a tag was added; their bit positions are offset by 64.
			/// [JP] 64 個目より後のタグは、そのようなタグを付けたときだけある。ビットの位置は 64 ずれている。
			if (record.extraTags_)
			{
				for (Size bit = record.extraTags_->find_first_set();bit != Bitset::InvalidIndex;bit = record.extraTags_->find_next_set(bit + 1))
				{
					Size index = bit + 64;
					if (names[index] != String{})
					{
						function(names[index]);
					}
				}
			}
		}

	private:
		/// [EN] Maps registered tag names to their bit index, the counterpart of names_; removed tags are no longer in it.
		/// [JP] 登録済みのタグ名をそのビットインデックスへ対応付ける。names_ と対になる。削除済みのタグはもう含まない。
		static inline FlatMap<String, Size> indices_;

		/// [EN] Tag names indexed by bit index; a removed tag's slot holds an empty String.
		/// [JP] ビットインデックスでアクセスできるタグ名一覧。削除済みタグの位置は空の String。
		static inline DynamicArray<String> names_;
	};
}

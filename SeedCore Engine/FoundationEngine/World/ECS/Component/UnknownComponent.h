#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/World/Actor/Blueprint.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Holds the components of an actor whose type this build does not
	* know - typically a script whose source has not been pulled or
	* built on this machine yet. Each one is kept exactly as it was
	* saved, name and fields alike, so saving or publishing the actor
	* carries it through unchanged. The Editor shows each one as
	* "Unknown Component", and once its script is registered it is
	* turned back into the real component with its saved values.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この実行ファイルが型を知らない、actor のコンポーネントを保持する。
	* 典型的には、この PC ではまだソースを pull・ビルドしていないスクリプト。
	* 名前もフィールドも保存されたとおりのまま持つため、actor を保存・
	* Publish してもそのまま運ばれる。Editor では各々を
	* 「Unknown Component」として表示し、そのスクリプトが登録された時点で、
	* 保存済みの値を持った本来のコンポーネントへ戻す。
	*/
	struct SEEDCORE_API UnknownComponent
	{
		/// [EN] Every component of this actor whose type is not registered, as it was saved.
		/// [JP] この actor のコンポーネントのうち、型が登録されていないもの全て。保存されたとおりの形で持つ。
		DynamicArray<BlueprintComponent> components_;

		/**
		* [EN]
		* Keeps component on actor as an unknown one, adding the holder
		* when the actor has none yet. A component already kept under the
		* same name is replaced, since one actor carries one of each type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* component を、型の分からないものとして actor に保持させる。actor
		* がまだ保持用のコンポーネントを持っていなければ追加する。同じ名前で
		* 既に保持しているものは置き換える。1つの actor が持つのは型ごとに
		* 1つであるため。
		*/
		static void Keep(Actor actor, const BlueprintComponent& component);

		/**
		* [EN]
		* Turns every kept component whose type has since been registered
		* back into the real component, restoring its saved values, and
		* removes the holder from actors left with nothing unknown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 保持しているコンポーネントのうち、その後に型が登録されたものを
		* 本来のコンポーネントへ戻し、保存済みの値を復元する。型の分からない
		* ものが残らなかった actor からは、保持用のコンポーネントを外す。
		*/
		static void Resolve(World& world);
	};
	REGISTER_COMPONENT(UnknownComponent, "Core", ComponentStorage::SparseSet);
}

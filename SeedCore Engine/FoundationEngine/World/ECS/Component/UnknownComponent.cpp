#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
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
	void UnknownComponent::Keep(Actor actor, const BlueprintComponent& component)
	{
		ComponentID id = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!actor || !id)
		{
			return;
		}
		if (!actor.HasComponent(id))
		{
			actor.AddComponent(id);
		}

		/// [EN] The holder is reached through the world because the actor only hands out read-only access to a plain component.
		/// [JP] 保持用のコンポーネントには world を通して触れる。actor が素のコンポーネントに渡すのは読み取り専用のアクセスだけであるため。
		UnknownComponent* unknown = static_cast<UnknownComponent*>(actor.GetWorld().GetComponent(actor.GetEntity(), id));
		if (!unknown)
		{
			return;
		}
		erase_if(unknown->components_, [&component](const BlueprintComponent& kept) { return kept.componentName_ == component.componentName_; });
		unknown->components_.push_back(component);
	}

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
	void UnknownComponent::Resolve(World& world)
	{
		ComponentID id = ComponentRegistry::GetComponentID<UnknownComponent>();
		if (!id)
		{
			return;
		}

		for (Actor actor : world.GetActors())
		{
			UnknownComponent* unknown = static_cast<UnknownComponent*>(world.GetComponent(actor.GetEntity(), id));
			if (!unknown)
			{
				continue;
			}

			/// [EN] The list is copied out first, since adding the real components may move this actor's storage and leave the holder pointer stale.
			/// [JP] 先に一覧を写し取る。本来のコンポーネントを足すと actor の格納場所が動き、保持用のコンポーネントへのポインタが古くなり得るため。
			DynamicArray<BlueprintComponent> kept = unknown->components_;
			DynamicArray<BlueprintComponent> remaining;
			for (const BlueprintComponent& component : kept)
			{
				ComponentID resolved = ComponentRegistry::GetComponentID(component.componentName_);
				if (!resolved)
				{
					remaining.push_back(component);
					continue;
				}

				if (!actor.HasComponent(resolved))
				{
					actor.AddComponent(resolved);
				}
				void* data = world.GetComponent(actor.GetEntity(), resolved);
				if (data)
				{
					ApplyComponent(component, data);
				}
			}

			/// [EN] Nothing changed when every kept component is still unknown, so the holder is left exactly as it was.
			/// [JP] 保持しているものが全て未だ不明なら何も変わっていないので、保持用のコンポーネントはそのままにする。
			if (remaining.size() == kept.size())
			{
				continue;
			}
			if (remaining.empty())
			{
				actor.RemoveComponent(id);
				continue;
			}

			unknown = static_cast<UnknownComponent*>(world.GetComponent(actor.GetEntity(), id));
			if (unknown)
			{
				unknown->components_ = remaining;
			}
		}
	}
}

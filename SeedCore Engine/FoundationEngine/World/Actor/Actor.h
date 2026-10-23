#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Service.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Lightweight value handle to a GameObject-style actor, and the face of
	* an actor that scripts see: it pairs the owning World with a
	* generation-checked Entity, and forwards every operation (components,
	* services, hierarchy, transform, active state, tags, layers) to World,
	* where the actor's data actually lives. Only what scripts use is here;
	* bookkeeping that only the engine and the editor need (prefab links,
	* persistent IDs, per-actor component lists) is kept by the systems that
	* own it. Copy it freely; it is 16 bytes and never dangles - a call on
	* an actor whose entity has been destroyed resolves to nothing (nullptr
	* / default) rather than aliasing a recycled slot. A
	* default-constructed Actor is invalid (converts to false).
	*
	* Every template member is a thin wrapper that turns T into its
	* registered ID (or its service name) and calls a type-erased member
	* defined in Actor.cpp, so this header never needs World's definition.
	*
	* Individual non-template members carry SEEDCORE_API rather than the
	* class itself, because a class-level dllexport/dllimport propagates
	* to member function templates under Clang (unlike MSVC).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GameObject 風の actor への軽量な値ハンドルで、スクリプトから見える
	* actor の面。所有元 World と世代チェック付き Entity のペアで、全操作
	* （コンポーネント、サービス、階層、トランスフォーム、アクティブ状態、
	* タグ、レイヤー）を、actor のデータが実際に存在する World へ転送する。
	* ここに置くのはスクリプトが使うものだけで、エンジンとエディタだけが
	* 要る管理情報（プレハブとのつながり、永続ID、actor ごとの
	* コンポーネント一覧）は、それを持つ仕組みの側が抱える。自由にコピー
	* してよい。16 バイトで、ダングリングしない - エンティティが破棄された
	* actor への呼び出しは、再利用スロットにエイリアスするのではなく、
	* 何も無い結果（nullptr / デフォルト）に解決される。デフォルト構築
	* された Actor は無効（false に変換される）。
	*
	* テンプレートのメンバーはどれも、T を登録済みの ID（またはサービスの
	* 名前）に変えて、Actor.cpp にある型消去版を呼ぶだけの薄い包みなので、
	* このヘッダは World の定義を必要としない。
	*
	* テンプレートでないメンバーには、クラス全体ではなく個別に
	* SEEDCORE_API を付ける。Clang では（MSVC と違い）クラス単位の
	* dllexport/dllimport がメンバー関数テンプレートにまで伝わるため。
	*/
	class Actor
	{
	private:
		friend class ActorAccessor;

		/**
		* [EN]
		* Constructs a handle to the actor identified by entity within world.
		* Private, because scripts never build an Actor from an entity: they
		* receive actors from World or from their own component. Inside the
		* engine, ActorAccessor is the one place that builds them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world 内で entity が識別する actor へのハンドルを構築する。
		* スクリプトがエンティティから Actor を作ることは無く、World か自分の
		* コンポーネントから受け取るので private にする。エンジンの中では、
		* ActorAccessor が作る唯一の場所になる。
		*/
		SEEDCORE_API Actor(World& world, Entity entity);

	public:
		/**
		* [EN]
		* Default constructor: an invalid actor (no World, null entity).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デフォルトコンストラクタ: 無効な actor（World 無し、null エンティティ）。
		*/
		Actor() = default;

		/**
		* [EN]
		* Returns whether this handle refers to a live actor, letting an
		* Actor be used directly in a boolean context (if, &&, ?:).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このハンドルが生存中の actor を指しているかどうかを返す。Actor を
		* 真偽値コンテキスト（if、&&、?:）でそのまま使えるようにする。
		*/
		SEEDCORE_API explicit operator Bool()const;

		/**
		* [EN]
		* Returns whether this and other are handles to the same actor:
		* they refer to the same World and the same Entity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* this と other が同じ actor へのハンドルかどうかを返す: 同じ
		* World と同じ Entity を指しているか。
		*/
		SEEDCORE_API Bool operator==(const Actor& other)const;

	public:
		/**
		* [EN]
		* Default-constructs and adds a T component to this actor and returns
		* a pointer to it, or nullptr if the actor is invalid or T is not
		* registered. A ComponentBehaviour-derived T is also wired to this
		* actor (see the type-erased overload).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T コンポーネントをデフォルト構築してこの actor へ追加し、その
		* ポインタを返す。actor が無効か T が未登録なら nullptr。
		* ComponentBehaviour 派生の T は、この actor への配線も行う（型消去版を
		* 参照）。
		*/
		template<typename T>
		T* AddComponent()
		{
			return static_cast<T*>(AddComponent(ComponentRegistry::GetComponentID<T>()));
		}

		/**
		* [EN]
		* Adds a copy of value as this actor's T component and returns a
		* pointer to it. Only for plain-data T: a ComponentBehaviour is wired
		* to its actor when added, which a copied value cannot carry.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* value のコピーをこの actor の T コンポーネントとして追加し、その
		* ポインタを返す。ただのデータの T に限る: ComponentBehaviour は追加
		* されたときに actor へ配線されるもので、コピーした値ではそれを
		* 持ち込めないため。
		*/
		template<typename T>
			requires (!std::derived_from<T, ComponentBehaviour>)
		T* AddComponent(const T& value)
		{
			T* component = AddComponent<T>();
			if (component)
			{
				*component = value;
			}
			return component;
		}

		/**
		* [EN]
		* Type-erased overload of AddComponent: adds a default-constructed
		* instance of the component registered under id and returns its
		* address, or returns the existing one if this actor already has it.
		* If the type has a hook table (it derives from ComponentBehaviour),
		* a newly added instance is wired to this actor's world, entity and
		* registration, and then handed to the type's attach_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* AddComponent の型消去版: id に登録されているコンポーネントの
		* デフォルト構築されたインスタンスを追加し、そのアドレスを返す。この
		* actor が既に持っていれば、既存のものを返す。型がフックの表を持つ
		* （ComponentBehaviour から派生している）なら、新しく追加した
		* インスタンスをこの actor の World・エンティティ・登録へ結び付けて
		* から、型の attach_ に渡す。
		*/
		SEEDCORE_API void* AddComponent(ComponentID id);

		/**
		* [EN]
		* Returns this actor's T component, or nullptr if absent.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の T コンポーネントを返す。無ければ nullptr を返す。
		*/
		template<typename T>
		T* GetComponent()const
		{
			return static_cast<T*>(GetComponent(ComponentRegistry::GetComponentID<T>()));
		}

		/**
		* [EN]
		* Type-erased overload of GetComponent: returns the address of this
		* actor's component registered under id, or nullptr if absent.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GetComponent の型消去版: この actor の、id に登録されている
		* コンポーネントのアドレスを返す。無ければ nullptr を返す。
		*/
		SEEDCORE_API void* GetComponent(ComponentID id)const;

		/**
		* [EN]
		* Returns whether this actor currently has a T component.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor が現在 T コンポーネントを持っているかどうかを返す。
		*/
		template<typename T>
		Bool HasComponent()const
		{
			return HasComponent(ComponentRegistry::GetComponentID<T>());
		}

		/**
		* [EN]
		* Type-erased overload of HasComponent: returns whether this actor
		* currently has the component registered under id.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* HasComponent の型消去版: この actor が現在、id に登録されている
		* コンポーネントを持っているかどうかを返す。
		*/
		SEEDCORE_API Bool HasComponent(ComponentID id)const;

		/**
		* [EN]
		* Removes this actor's T component, invoking its OnDestroy and then its OnDetach first if
		* it derives from ComponentBehaviour.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の T コンポーネントを削除する。ComponentBehaviour から
		* 派生していれば、先に OnDestroy、続いて OnDetach を呼び出す。
		*/
		template<typename T>
		void RemoveComponent()
		{
			RemoveComponent(ComponentRegistry::GetComponentID<T>());
		}

		/**
		* [EN]
		* Type-erased overload of RemoveComponent: removes this actor's
		* component registered under id, invoking its OnDestroy and then its OnDetach first if it
		* derives from ComponentBehaviour.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* RemoveComponent の型消去版: この actor の、id に登録されている
		* コンポーネントを削除する。ComponentBehaviour から派生していれば、
		* 先に OnDestroy、続いて OnDetach を呼び出す。
		*/
		SEEDCORE_API void RemoveComponent(ComponentID id);

		/**
		* [EN]
		* Returns a reference to the reflected field named fieldName (its
		* display name) of this actor's component named componentName,
		* whichever language the component is written in. Reading and
		* writing both go through the returned reference. Use it right
		* away rather than keeping it, since the component can be removed
		* or moved. If the component or a field of type T with that name
		* is missing, logs a warning and returns a reference to a
		* default-initialized dummy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の componentName という名前のコンポーネントにある、
		* fieldName（表示名）という名前のリフレクション対象フィールドへの
		* 参照を返す。コンポーネントがどちらの言語で書かれていても同じ。
		* 読むのも書くのも返った参照で行う。コンポーネントは外されたり
		* 移動したりしうるので、保持せずその場で使うこと。コンポーネント、
		* またはその名前で型が T のフィールドが無ければ、警告ログを出し、
		* デフォルト初期化したダミーへの参照を返す。
		*/
		template<typename T>
		T& Field(const String& componentName, const String& fieldName)const
		{
			/// [EN] Returned on a miss so the caller always gets a valid reference; reset each call so a write to it never leaks into the next miss.
			/// [JP] 見つからなかったときに返し、呼び出し側が常に有効な参照を受け取れるようにする。書き込まれた値が次に見つからなかったときへ持ち越されないよう、呼ぶたびに初期値へ戻す。
			static T fallback{};
			fallback = T{};

			/// [EN] Resolve the component instance and the function that lists its reflected fields.
			/// [JP] コンポーネントの実体と、そのリフレクション対象フィールドを列挙する関数を解決する。
			ComponentID id = ComponentRegistry::GetComponentID(componentName);
			void* data = id ? GetComponent(id) : nullptr;
			auto reflection = ReflectionRegistry::GetRegistry().find(componentName);
			if (data == nullptr || reflection == ReflectionRegistry::GetRegistry().end())
			{
				SC_LOG_WARNING("Field: この actor は {} を持っていません", componentName.str());
				return fallback;
			}

			/// [EN] Match on both the display name and the value kind, so a reference of the wrong type is never handed out.
			/// [JP] 表示名と値種別の両方で照合し、型の違う参照を渡すことがないようにする。
			DynamicArray<FieldInfo> fields;
			reflection->second(data, fields);
			for (const FieldInfo& field : fields)
			{
				if (field.name_ == fieldName && field.type_ == AttributeTraits<T>::type)
				{
					return *static_cast<T*>(field.directPtr_ ? field.directPtr_ : static_cast<Uint8*>(data) + field.offset_);
				}
			}

			SC_LOG_WARNING("Field: {} に {} という名前でこの型のフィールドはありません", componentName.str(), fieldName.str());
			return fallback;
		}

		/**
		* [EN]
		* Calls the function named functionName of this actor's component
		* named componentName with arguments, whichever language the
		* component is written in, and returns its result as Result (void
		* for no result). The function must be registered in
		* FunctionRegistry (SC_FUNCTION in C++, [SeedFunction] in C#). If
		* it is missing, or the argument or result types do not match,
		* logs a warning, calls nothing and returns a default-initialized
		* Result.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の componentName という名前のコンポーネントにある、
		* functionName という名前の関数を arguments を渡して呼び、戻り値を
		* Result として返す（戻り値が無い場合は void）。コンポーネントが
		* どちらの言語で書かれていても同じ。関数は FunctionRegistry に
		* 登録されている必要がある（C++ は SC_FUNCTION、C# は
		* [SeedFunction]）。見つからないか、引数や戻り値の型が合わなければ、
		* 警告ログを出して何も呼ばず、デフォルト初期化した Result を返す。
		*/
		template<typename Result = void, typename... Arguments>
		Result Function(const String& componentName, const String& functionName, const Arguments&... arguments)const
		{
			/// [EN] Storage the invoker writes the result into; a function returning nothing gets an unused Int so the code below stays the same for both cases.
			/// [JP] 呼び出し処理が戻り値を書き込む先。戻り値の無い関数では使われない Int にして、以下の処理を両方の場合で共通にする。
			std::conditional_t<std::is_void_v<Result>, Int, Result> result{};

			/// [EN] Resolve the component instance and the functions registered for its type.
			/// [JP] コンポーネントの実体と、その型に登録されている関数の一覧を解決する。
			ComponentID id = ComponentRegistry::GetComponentID(componentName);
			void* data = id ? GetComponent(id) : nullptr;
			auto functions = FunctionRegistry::GetRegistry().find(componentName);
			if (data == nullptr || functions == FunctionRegistry::GetRegistry().end())
			{
				SC_LOG_WARNING("Function: この actor は {} を持っていないか、{} に呼べる関数がありません", componentName.str(), componentName.str());
			}
			else
			{
				/// [EN] The value kinds of the arguments as passed, compared against the registered parameters, since the invoker trusts that they match.
				/// [JP] 渡された引数の値種別。呼び出し処理は一致している前提で読むので、登録されている引数と照らし合わせる。
				StaticArray<AttributeType, sizeof...(Arguments)> parameters{ AttributeTraits<Arguments>::type... };
				auto function = std::ranges::find_if(functions->second, [&functionName](const FunctionInfo& info) { return info.name_ == functionName; });
				if (function == functions->second.end())
				{
					SC_LOG_WARNING("Function: {} に {} という関数はありません", componentName.str(), functionName.str());
				}
				else if (function->result_ != AttributeTraits<Result>::type || !std::ranges::equal(function->parameters_, parameters))
				{
					SC_LOG_WARNING("Function: {}.{} の引数か戻り値の型が合いません", componentName.str(), functionName.str());
				}
				else
				{
					/// [EN] The address of each argument, in order. One extra slot keeps the array valid for a function that takes no arguments.
					/// [JP] 各引数のアドレスを順に並べたもの。引数の無い関数でも配列が成り立つよう、1 つ余分に確保する。
					void* pointers[sizeof...(Arguments) + 1]{ const_cast<void*>(static_cast<const void*>(&arguments))... };
					function->invoke_(data, pointers, std::is_void_v<Result> ? nullptr : &result);
				}
			}

			if constexpr (!std::is_void_v<Result>)
			{
				return result;
			}
		}

	public:
		/**
		* [EN]
		* Returns the World that owns this actor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor を所有する World を返す。
		*/
		SEEDCORE_API World& GetWorld()const;

		/**
		* [EN]
		* Returns the T service of the World that owns this actor (for
		* example Physics, Audio or DebugDraw), or nullptr if that World does
		* not provide one. Services are reached the same way as components,
		* only per World rather than per actor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor を所有する World の T サービス（例えば Physics、Audio、
		* DebugDraw）を返す。その World が持っていなければ nullptr。
		* サービスはコンポーネントと同じ形で引き、actor ごとではなく World
		* ごとにあるという点だけが違う。
		*/
		template<typename T>
		T* GetService()const
		{
			return static_cast<T*>(GetService(ServiceType<T>::name_));
		}

	public:
		/**
		* [EN]
		* Reparents this actor under parent (or to the scene root if parent
		* is an invalid Actor) as its last child. An inactive new parent makes
		* this actor inactive; moving to the scene root makes it active again.
		* Does nothing if parent is this actor or one of its descendants.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor を parent の末尾の子として付け替える（parent が無効な
		* Actor であればシーンのルートへ）。新しい親が非アクティブならこの
		* actor も非アクティブになり、シーンのルートへ移るとアクティブに
		* 戻る。parent がこの actor 自身かその子孫なら何もしない。
		*/
		SEEDCORE_API void Parent(Actor parent);

		/**
		* [EN]
		* Returns this actor's current parent, or an invalid Actor if it
		* has none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の現在の親を返す。親が無ければ無効な Actor を返す。
		*/
		SEEDCORE_API Actor Parent()const;

		/**
		* [EN]
		* Returns this actor's direct children, in their current order.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の直接の子一覧を、現在の順序で返す。
		*/
		SEEDCORE_API DynamicArray<Actor> Children()const;

		/**
		* [EN]
		* Moves this actor right after after among its siblings (the children
		* of its parent, or the scene roots), or to the front if after is
		* invalid. Does nothing if after is not one of its siblings.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 兄弟（親の子の並び、またはシーンのルート群）の中で、この actor を
		* after の直後へ移す。after が無効なら先頭へ移す。after が兄弟で
		* なければ何もしない。
		*/
		SEEDCORE_API void Move(Actor after);

	public:
		/**
		* [EN]
		* Returns this actor's world-space transform matrix, read from its
		* WorldMatrix component.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor のワールド空間変換行列を、WorldMatrix コンポーネントから
		* 読んで返す。
		*/
		SEEDCORE_API const Matrix& WorldMatrix()const;

		/**
		* [EN]
		* Sets this actor's world-space transform matrix in its WorldMatrix
		* component.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor のワールド空間変換行列を、WorldMatrix コンポーネントに
		* 設定する。
		*/
		SEEDCORE_API void WorldMatrix(const Matrix& matrix);

	public:
		/**
		* [EN]
		* Returns whether this actor is currently active, read from its
		* Active component.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor が現在アクティブかどうかを、Active コンポーネントから
		* 読んで返す。
		*/
		SEEDCORE_API Bool Active()const;

		/**
		* [EN]
		* Sets this actor's active state in its Active component, and
		* propagates the same state to every descendant.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor のアクティブ状態を Active コンポーネントに設定し、同じ
		* 状態を全ての子孫へ伝播する。
		*/
		SEEDCORE_API void Active(Bool active);

	public:
		/**
		* [EN]
		* Adds tag to this actor's tag set, registering it in TagRegistry
		* if it's new. An empty tag is ignored.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* tag をこの actor のタグ集合へ追加する。新規のタグであれば
		* TagRegistry へ登録する。空の tag は無視する。
		*/
		SEEDCORE_API void AddTag(String tag);

		/**
		* [EN]
		* Returns whether this actor currently has tag.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor が現在 tag を持っているかどうかを返す。
		*/
		SEEDCORE_API Bool HasTag(String tag)const;

		/**
		* [EN]
		* Removes tag from this actor's tag set, if present.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* tag をこの actor のタグ集合から、存在すれば削除する。
		*/
		SEEDCORE_API void RemoveTag(String tag);

	public:
		/**
		* [EN]
		* Sets this actor's layer to the LayerRegistry slot at index
		* (clamped to a valid slot if out of range).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor のレイヤーを、index の LayerRegistry スロットに設定
		* する（範囲外なら有効なスロットへクランプする）。
		*/
		SEEDCORE_API void Layer(Size index);

		/**
		* [EN]
		* Sets this actor's layer by name via LayerRegistry::Find, which
		* gives LayerRegistry::DefaultLayer if name isn't registered.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LayerRegistry::Find 経由で、名前でこの actor のレイヤーを設定
		* する。name が未登録であれば LayerRegistry::DefaultLayer になる。
		*/
		SEEDCORE_API void Layer(const String& name);

		/**
		* [EN]
		* Returns this actor's current layer index (a LayerRegistry slot).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この actor の現在のレイヤーインデックス（LayerRegistry の
		* スロット）を返す。
		*/
		SEEDCORE_API Size Layer()const;

		/**
		* [EN]
		* Returns this actor's current layer's name, via LayerRegistry::Name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* LayerRegistry::Name 経由で、この actor の現在のレイヤー名を
		* 返す。
		*/
		SEEDCORE_API const String& LayerName()const;

	private:
		/**
		* [EN]
		* Type-erased overload of GetService: returns the service of this
		* actor's World whose type's name is type (a ServiceType<T>::name_),
		* or nullptr if there is none.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GetService の型消去版: この actor の World で、型の名前が type
		* （ServiceType<T>::name_）であるサービスを返す。無ければ nullptr。
		*/
		SEEDCORE_API void* GetService(const String& type)const;

	private:
		/// [EN] The World that owns this actor's data; nullptr for a default-constructed (invalid) Actor.
		/// [JP] この actor のデータを所有する World。デフォルト構築された（無効な）Actor では nullptr。
		World* world_ = nullptr;

		/// [EN] This actor's underlying entity handle.
		/// [JP] この actor の内部エンティティハンドル。
		Entity entity_;
	};
}

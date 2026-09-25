#pragma once
#include <FoundationEngine/Utility/Types.h>
#include <FoundationEngine/Log/Assert.h>
#include <type_traits>
#include <utility>

namespace SeedCore
{
	template<typename T, typename H, Size LogSize>
	class ObjectPool;

	/**
	* [EN]
	* Sole-ownership smart pointer, custom-built as a replacement for
	* std::unique_ptr. Move-only: copying is disabled so ownership of the
	* pointee can never be ambiguous. Supports moving/converting from a
	* ResourcePtr<U> to a ResourcePtr<T> when U* is implicitly convertible
	* to T* (e.g. Derived -> Base), so factories can return a derived
	* instance through a base-typed ResourcePtr; the pointee is then
	* destroyed through T*, so T needs a virtual destructor in that case.
	* The pointee is either heap-allocated (released with delete) or
	* created from an ObjectPool (returned to that pool with Recycle); a
	* pooled ResourcePtr remembers its pool and how to recycle into it,
	* and that pool must outlive it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* std::unique_ptr の代替として自作した単独所有スマートポインタ。
	* ムーブ専用（コピーは禁止）で、所有権の所在が曖昧にならないように
	* している。U* が T* へ暗黙変換可能（例: 派生 -> 基底）な場合、
	* ResourcePtr<U> から ResourcePtr<T> へのムーブ/変換に対応しており、
	* ファクトリが派生インスタンスを基底型の ResourcePtr で返せる。その
	* 場合オブジェクトは T* 経由で破棄されるので、T には仮想
	* デストラクタが必要。オブジェクトはヒープ確保（delete で解放）か
	* ObjectPool からの生成（Recycle でそのプールへ返却）のどちらかで、
	* プール生成の ResourcePtr は自分のプールと返却方法を覚えている。
	* そのプールは ResourcePtr より長く生きていなければならない。
	*/
	template <typename T>
	class ResourcePtr
	{
	public:
		/**
		* [EN]
		* Default-constructs a null ResourcePtr (owns nothing).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 何も所有していない null な ResourcePtr を構築する。
		*/
		constexpr ResourcePtr()noexcept = default;

		/**
		* [EN]
		* Constructs a null ResourcePtr from nullptr, for `= nullptr`-style
		* initialization at call sites.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* nullptr から null な ResourcePtr を構築する。呼び出し側での
		* `= nullptr` 形式の初期化のために用意している。
		*/
		constexpr ResourcePtr(std::nullptr_t)noexcept
		{
			/// No Code
		}

		/**
		* [EN]
		* Takes ownership of an already-allocated raw pointer. Explicit,
		* since implicitly wrapping a raw pointer is exactly the footgun
		* (double ownership via two separate wrappers) this type avoids;
		* prefer MakePtr() at call sites instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 既に確保済みの生ポインタの所有権を引き取る。生ポインタを暗黙に
		* ラップできてしまうと（別々のラッパーが同じポインタを二重所有する）
		* この型がまさに避けたい地雷になるため explicit にしている。
		* 呼び出し側では基本的に MakePtr() を使うこと。
		*/
		explicit ResourcePtr(T* pointer)noexcept: pointer_(pointer)
		{
			/// No Code
		}

		/**
		* [EN]
		* Takes ownership of a pointer created from pool; on release,
		* recycle(pool, object) returns it to that pool instead of deleting
		* it. Prefer the MakePtr(ObjectPool&, ...) overload at call sites.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* pool から生成されたポインタの所有権を引き取る。解放時は delete
		* せず、recycle(pool, object) でそのプールへ返却する。呼び出し側では
		* 基本的に MakePtr(ObjectPool&, ...) オーバーロードを使うこと。
		*/
		ResourcePtr(T* pointer, void* pool, void (*recycle)(void*, void*))noexcept: pointer_(pointer), pool_(pool), recycle_(recycle)
		{
			/// No Code
		}

		/**
		* [EN]
		* Copy construction is disabled — ownership must never be shared.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止。所有権が複数箇所で共有されることはない。
		*/
		ResourcePtr(const ResourcePtr&) = delete;

		/**
		* [EN]
		* Copy assignment is disabled — ownership must never be shared.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー代入は禁止。所有権が複数箇所で共有されることはない。
		*/
		ResourcePtr& operator=(const ResourcePtr&) = delete;

		/**
		* [EN]
		* Transfers ownership from other, leaving other null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* other から所有権を引き継ぎ、other は null になる。
		*/
		ResourcePtr(ResourcePtr&& other)noexcept: pointer_(other.pointer_), pool_(other.pool_), recycle_(other.recycle_)
		{
			/// [EN] other lets go of the pointer, so exactly one ResourcePtr releases it.
			/// [JP] other がポインタを手放すので、それを解放する ResourcePtr はちょうど1つになる。
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
		}

		/**
		* [EN]
		* Converting move constructor: transfers ownership from a
		* ResourcePtr<U> when U* implicitly converts to T* (e.g. taking
		* ownership of a Derived instance through a Base-typed ResourcePtr).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 変換ムーブコンストラクタ。U* が T* へ暗黙変換可能な場合に
		* ResourcePtr<U> から所有権を引き継ぐ（例: 派生インスタンスの
		* 所有権を基底型の ResourcePtr として受け取る）。
		*/
		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourcePtr(ResourcePtr<U>&& other)noexcept: pointer_(other.pointer_), pool_(other.pool_), recycle_(other.recycle_)
		{
			/// [EN] The pointer converts from U* to T* in the initializer; other lets go of it.
			/// [JP] 初期化の中でポインタは U* から T* へ変換される。other はそれを手放す。
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
		}

		/**
		* [EN]
		* Releases the currently-owned pointee (if any) and transfers
		* ownership from other, leaving other null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在所有しているオブジェクトを（あれば）解放し、other から
		* 所有権を引き継ぐ。other は null になる。
		*/
		ResourcePtr& operator=(ResourcePtr&& other)noexcept
		{
			/// [EN] Self-assignment is skipped, since reset() would release the object about to be taken over.
			/// [JP] 自己代入は飛ばす。reset() が、これから引き取るはずのオブジェクトを解放してしまうため。
			if (this != &other)
			{
				reset();
				pointer_ = other.pointer_;
				pool_ = other.pool_;
				recycle_ = other.recycle_;
				other.pointer_ = nullptr;
				other.pool_ = nullptr;
				other.recycle_ = nullptr;
			}
			return *this;
		}

		/**
		* [EN]
		* Converting move assignment: same as the converting move
		* constructor, but for an already-constructed ResourcePtr.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 変換ムーブ代入。変換ムーブコンストラクタと同様だが、既に構築済みの
		* ResourcePtr に対して行う。
		*/
		template <typename U>
			requires std::is_convertible_v<U*, T*>
		ResourcePtr& operator=(ResourcePtr<U>&& other)noexcept
		{
			/// [EN] The previous pointee is released before the new one is taken over.
			/// [JP] 新しいオブジェクトを引き取る前に、前のオブジェクトを解放する。
			reset();
			pointer_ = other.pointer_;
			pool_ = other.pool_;
			recycle_ = other.recycle_;
			other.pointer_ = nullptr;
			other.pool_ = nullptr;
			other.recycle_ = nullptr;
			return *this;
		}

		/**
		* [EN]
		* Resets to null, releasing the currently-owned pointee (if any).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* null に戻し、現在所有しているオブジェクトを（あれば）解放する。
		*/
		ResourcePtr& operator=(std::nullptr_t)noexcept
		{
			reset();
			return *this;
		}

		/**
		* [EN]
		* Destroys the owned pointee (if any) via reset().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* reset() を通じて、所有しているオブジェクトを（あれば）破棄する。
		*/
		~ResourcePtr()
		{
			reset();
		}

		/**
		* [EN]
		* Destroys the owned pointee (if any) and becomes null: a pooled
		* pointee is returned to its pool, any other is deleted. Safe to
		* call on an already-null ResourcePtr (no-op).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 所有しているオブジェクトを（あれば）破棄し、null になる。プール
		* 生成のものはそのプールへ返却し、それ以外は delete する。
		* 既に null な ResourcePtr に対して呼んでも安全（何もしない）。
		*/
		void reset()noexcept
		{
			/// [EN] The pointer is cleared after releasing, so a second reset does nothing.
			/// [JP] 解放した後にポインタを消すので、2回目の reset は何もしない。
			if (pointer_ != nullptr)
			{
				if (recycle_ != nullptr)
				{
					/// [EN] The pool takes the most-derived object's address; for a polymorphic T held through a base, dynamic_cast<void*> recovers it.
					/// [JP] プールには最派生オブジェクトのアドレスを渡す。基底型で持つ多態的な T は dynamic_cast<void*> でそれを取り戻す。
					void* object = nullptr;
					if constexpr (std::is_polymorphic_v<T>)
					{
						object = dynamic_cast<void*>(const_cast<std::remove_cv_t<T>*>(pointer_));
					}
					else
					{
						object = const_cast<std::remove_cv_t<T>*>(pointer_);
					}
					recycle_(pool_, object);
				}
				else
				{
					delete pointer_;
				}
				pointer_ = nullptr;
				pool_ = nullptr;
				recycle_ = nullptr;
			}
		}

		/**
		* [EN]
		* Returns the raw owned pointer without releasing ownership.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 所有権を手放さずに、所有している生ポインタを返す。
		*/
		T* get()const noexcept
		{
			return pointer_;
		}

		/**
		* [EN]
		* Dereferences the owned pointee. Asserts (in every build) if this
		* ResourcePtr is null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 所有しているオブジェクトを間接参照する。この ResourcePtr が
		* null の場合はアサートする（どのビルドでも）。
		*/
		T* operator->()const
		{
			SC_ASSERT(pointer_ != nullptr, "Dereferencing a null ResourcePtr.");
			return pointer_;
		}

		/**
		* [EN]
		* Same as operator->(), but returns a reference instead of a pointer.
		* Asserts (in every build) if this ResourcePtr is null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* operator->() と同様だが、ポインタではなく参照を返す。この
		* ResourcePtr が null の場合はアサートする（どのビルドでも）。
		*/
		T& operator*()const
		{
			SC_ASSERT(pointer_ != nullptr, "Dereferencing a null ResourcePtr.");
			return *pointer_;
		}

		/**
		* [EN]
		* Returns whether this ResourcePtr owns a non-null pointee.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この ResourcePtr が null でないオブジェクトを所有しているかを返す。
		*/
		explicit operator bool()const noexcept
		{
			return pointer_ != nullptr;
		}

		/**
		* [EN]
		* Compares the owned raw pointers of two ResourcePtr instances for
		* equality.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つの ResourcePtr が所有している生ポインタ同士を比較する。
		*/
		friend Bool operator==(const ResourcePtr& lhs, const ResourcePtr& rhs)noexcept
		{
			return lhs.pointer_ == rhs.pointer_;
		}

		/**
		* [EN]
		* Returns whether this ResourcePtr is null.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この ResourcePtr が null かどうかを返す。
		*/
		friend Bool operator==(const ResourcePtr& lhs, std::nullptr_t)noexcept
		{
			return lhs.pointer_ == nullptr;
		}

	private:
		/// [EN] Lets every ResourcePtr<U> reach this instantiation's private members, as the converting constructor/assignment above requires.
		/// [JP] 上の変換コンストラクタ/代入のため、あらゆる ResourcePtr<U> からこのインスタンスの private メンバに触れられるようにする。
		template <typename U>
		friend class ResourcePtr;

		/// [EN] The owned raw pointer; nullptr when this ResourcePtr owns nothing.
		/// [JP] 所有している生ポインタ。何も所有していない場合は nullptr。
		T* pointer_ = nullptr;

		/// [EN] The ObjectPool the pointee was created from, type-erased; nullptr for a heap-allocated pointee.
		/// [JP] オブジェクトの生成元の ObjectPool（型消去済み）。ヒープ確保のものは nullptr。
		void* pool_ = nullptr;

		/// [EN] Returns the most-derived object to pool_; nullptr for a heap-allocated pointee, which is deleted instead.
		/// [JP] 最派生オブジェクトを pool_ へ返却する関数。ヒープ確保のものは nullptr で、代わりに delete される。
		void (*recycle_)(void*, void*) = nullptr;
	};

	/**
	* [EN]
	* Constructs a T in place and returns it wrapped in a ResourcePtr,
	* analogous to std::make_unique. The intended way to obtain a new
	* ResourcePtr, since the allocation and the wrapping happen in one
	* place and the raw pointer never escapes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T をその場で構築し、ResourcePtr でラップして返す。std::make_unique
	* に相当する。確保とラップが1か所で行われ、生ポインタが外に漏れない
	* ので、新しい ResourcePtr はこれで得るのが基本。
	*/
	template <typename T, typename... Args>
	[[nodiscard]] ResourcePtr<T> MakePtr(Args&&... args)
	{
		/// [EN] The raw pointer goes straight into the ResourcePtr, so nothing else can hold it.
		/// [JP] 生ポインタはそのまま ResourcePtr へ渡るので、他の誰もそれを持てない。
		return ResourcePtr<T>(new T(std::forward<Args>(args)...));
	}

	/**
	* [EN]
	* Constructs a T in a block of pool and returns it wrapped in a
	* ResourcePtr that recycles it back into pool on release. Objects
	* created from one pool sit together in its memory and never move,
	* unlike a plain heap allocation per object. pool must outlive the
	* returned ResourcePtr.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* pool のブロック内に T を構築し、解放時に pool へ返却する
	* ResourcePtr でラップして返す。1つのプールから生成したオブジェクトは
	* そのメモリ上にまとまって置かれ、移動することもない（オブジェクト
	* ごとのヒープ確保とは異なる）。pool は返された ResourcePtr より長く
	* 生きていなければならない。
	*/
	template <typename T, typename H, Size LogSize, typename... Args>
	[[nodiscard]] ResourcePtr<T> MakePtr(ObjectPool<T, H, LogSize>& pool, Args&&... args)
	{
		return ResourcePtr<T>(pool.Create(std::forward<Args>(args)...), &pool, [](void* owner, void* object) { static_cast<ObjectPool<T, H, LogSize>*>(owner)->Recycle(static_cast<T*>(object)); });
	}
}

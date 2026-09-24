#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Pool/ObjectPool.h>

namespace SeedCore
{
	/**
	* [EN]
	* Concept that is satisfied when T is a pointer type.
	* Used to decide whether the "empty" sentinel value for a queue
	* slot can simply be nullptr.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T がポインタ型である場合に満たされるコンセプト。
	* キューの空スロットを表す「空」値として、単純に nullptr を
	* 使用できるかどうかを判定するために使う。
	*/
	template<typename T>
	concept Nullable = std::is_pointer_v<T>;

	/**
	* [EN]
	* Concept that is satisfied when T is NOT a pointer type.
	* In this case the "empty" sentinel value is represented using
	* std::optional<T> instead of a null pointer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T がポインタ型でない場合に満たされるコンセプト。
	* この場合、「空」値は null ポインタではなく std::optional<T> を
	* 使って表現される。
	*/
	template<typename T>
	concept Optionalable = !std::is_pointer_v<T>;

	/**
	* [EN]
	* Returns the sentinel "empty" value used by the worker queues when
	* there is no item to return. If T is a pointer type, this is a null
	* pointer; otherwise it is an empty std::optional<T>.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワーカーキューが返すべき要素が無い場合に使用する「空」値を返す。
	* T がポインタ型であれば null ポインタを、それ以外であれば空の
	* std::optional<T> を返す。
	*/
	template <typename T>
	constexpr auto WorkerQueueEmptyValue()
	{
		if constexpr (Nullable<T>)
		{
			return T{ nullptr };
		}
		else if constexpr (Optionalable<T>)
		{
			return std::optional<T>{std::nullopt};
		}
	}

	/**
	* [EN]
	* Default log2 size (number of bits) used to determine the initial
	* capacity of an UnboundedWorkerQueue (initial capacity = 1 << this value).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* UnboundedWorkerQueue の初期容量を決定する際に使用する、デフォルトの
	* log2 サイズ（ビット数）。初期容量は (1 << この値) で計算される。
	*/
	inline constexpr Size SC_DEFAULT_UNBOUNDED_QUEUE_LOG_SIZE = 10;

	/**
	* [EN]
	* Default log2 size (number of bits) used to determine the fixed
	* capacity of a BoundedWorkerQueue (capacity = 1 << this value).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* BoundedWorkerQueue の固定容量を決定する際に使用する、デフォルトの
	* log2 サイズ（ビット数）。容量は (1 << この値) で計算される。
	*/
	inline constexpr Size SC_DEFAULT_BOUNDED_QUEUE_LOG_SIZE = 8;

	/**
	* [EN]
	* A Chase-Lev style work-stealing deque whose internal ring buffer
	* grows automatically when it becomes full.
	*
	* The owning thread calls push/pop from the "bottom" end, while
	* other threads may concurrently call steal from the "top" end.
	* JobExecutor uses it for its overflow buffers, where pushes from
	* many threads are serialized by a mutex so there is still only one
	* pusher at a time.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Chase-Lev 方式のワークスティーリング用デックであり、内部のリング
	* バッファが満杯になった場合に自動的に拡張される。
	*
	* 所有スレッドは「底（bottom）」側から push/pop を行い、他のスレッドは
	* 「頂点（top）」側から並行して steal を呼び出すことができる。
	* JobExecutor はこれをあふれ用バッファに使い、多くのスレッドからの
	* push をミューテックスで1つずつにすることで、push 側を常に1つに
	* 保っている。
	*/
	template <typename T>
	class UnboundedWorkerQueue
	{
	private:
		/**
		* [EN]
		* Internal fixed-size ring buffer backing the queue at a given
		* point in time. When the queue needs more capacity, a new,
		* larger Array is allocated and the old one is retired.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ある時点でキューを支える、内部の固定サイズのリングバッファ。
		* キューがより大きな容量を必要とする場合、新しく大きな Array が
		* 確保され、古いものは廃棄（保留）される。
		*/
		struct Array
		{
			/// [EN] Total number of slots in this buffer.
			/// [JP] このバッファが持つスロットの総数。
			Size capacity_;

			/// [EN] Bitmask (capacity_ - 1) used to wrap indices around the ring buffer.
			/// [JP] リングバッファのインデックスを折り返すために使用するビットマスク（capacity_ - 1）。
			Size mask_;

			/// [EN] Pointer to the heap-allocated array of atomic slots.
			/// [JP] ヒープ上に確保された、アトミックなスロット配列へのポインタ。
			std::atomic<T>* slots_;

			/**
			* [EN]
			* Allocates capacity slots; capacity must be a power of two
			* so mask_ can wrap indices.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* capacity 個のスロットを確保する。mask_ でインデックスを折り返す
			* ため、capacity は 2 のべき乗である必要がある。
			*/
			explicit Array(Size capacity) :capacity_(capacity), mask_(capacity - 1), slots_(new std::atomic<T>[capacity_])
			{
				/// No Code
			}

			/**
			* [EN]
			* Frees the slot array.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* スロット配列を解放する。
			*/
			~Array()
			{
				delete[] slots_;
			}

			/**
			* [EN]
			* Returns the number of slots in this buffer.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* このバッファのスロット数を返す。
			*/
			Size capacity()const noexcept
			{
				return capacity_;
			}

			/**
			* [EN]
			* Stores item into the slot corresponding to index,
			* wrapping around using the ring buffer mask.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* リングバッファのマスクを用いて折り返しを行いながら、index に
			* 対応するスロットへ item を格納する。
			*/
			void push(Int64 index, T item)noexcept
			{
				slots_[index & mask_].store(item, std::memory_order_relaxed);
			}

			/**
			* [EN]
			* Loads and returns the value stored at the slot
			* corresponding to index.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* index に対応するスロットに格納されている値を読み込み、
			* 返却する。
			*/
			T pop(Int64 index)noexcept
			{
				return slots_[index & mask_].load(std::memory_order_relaxed);
			}

			/**
			* [EN]
			* Allocates a new Array with double the current capacity
			* and copies all live elements in range [top, bottom) into it.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 現在の容量の 2 倍の容量を持つ新しい Array を確保し、
			* [top, bottom) の範囲にある有効な要素をすべてコピーする。
			*/
			Array* resize(Int64 bottom, Int64 top)
			{
				/// [EN] Elements keep their logical indices; only the slot they map to changes with the larger mask.
				/// [JP] 要素は論理的なインデックスをそのまま保つ。大きくなったマスクで、対応するスロットだけが変わる。
				Array* ptr = new Array(2 * capacity_);
				for (Int64 index = top;index != bottom;index++)
				{
					ptr->push(index, pop(index));
				}
				return ptr;
			}

			/**
			* [EN]
			* Allocates a new Array large enough to hold the current
			* live elements plus n additional elements (rounded up to
			* the next power of two), and copies all live elements in
			* range [top, bottom) into it.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* 現在有効な要素に加えて n 個分の追加要素を格納できる
			* （2 のべき乗に切り上げた）容量を持つ新しい Array を確保し、
			* [top, bottom) の範囲にある有効な要素をすべてコピーする。
			*/
			Array* resize(Int64 bottom, Int64 top, Int64 n)
			{
				/// [EN] Rounded up to a power of two so the index mask still works.
				/// [JP] インデックスのマスクが使えるよう、2 のべき乗に切り上げる。
				Array* ptr = new Array(std::bit_ceil(capacity_ + n));
				for (Int64 index = top;index != bottom;++index)
				{
					ptr->push(index, pop(index));
				}
				return ptr;
			}
		};

		/// [EN] Index of the "top" end of the deque, from which other threads steal. Cache-line aligned to avoid false sharing.
		/// [JP] デックの「頂点（top）」側のインデックス。他のスレッドはここから steal を行う。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> top_;

		/// [EN] Index of the "bottom" end of the deque, from which the owning thread pushes/pops. Cache-line aligned to avoid false sharing.
		/// [JP] デックの「底（bottom）」側のインデックス。所有スレッドがここから push/pop を行う。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> bottom_;

		/// [EN] Locally cached copy of top_, used by the owning thread to avoid re-reading the atomic on every push.
		/// [JP] top_ をローカルにキャッシュした値。所有スレッドが push の度にアトミック変数を読み直すのを避けるために使用する。
		Int64 cacheTop_ = 0;

		/// [EN] Pointer to the currently active ring buffer. Cache-line aligned to avoid false sharing.
		/// [JP] 現在使用中のリングバッファへのポインタ。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<Array*> array_;

		/// [EN] List of retired (replaced) buffers kept alive until the queue itself is destroyed, since steal() may still be reading from them concurrently.
		/// [JP] 廃棄（置き換え）済みのバッファを保持するリスト。steal() が並行してまだ読み取っている可能性があるため、キュー自体が破棄されるまで保持しておく。
		DynamicArray<Array*> garbage_;

	public:
		/// [EN] The value type returned by pop()/steal(): T itself if T is a pointer, otherwise std::optional<T>.
		/// [JP] pop()/steal() が返す値の型。T がポインタ型ならば T そのもの、それ以外なら std::optional<T>。
		using ValueType = std::conditional_t<std::is_pointer_v<T>, T, std::optional<T>>;

		/**
		* [EN]
		* Constructs the queue with an initial capacity of
		* 1 << logSize slots.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 << logSize 個のスロットからなる初期容量でキューを構築する。
		*/
		explicit UnboundedWorkerQueue(Int64 logSize = SC_DEFAULT_UNBOUNDED_QUEUE_LOG_SIZE)
		{
			top_.store(0, std::memory_order_relaxed);
			bottom_.store(0, std::memory_order_relaxed);
			array_.store(new Array{ (Size{ 1 } << logSize) }, std::memory_order_relaxed);

			/// [EN] Each retirement doubles the buffer, so a handful of entries covers any realistic growth.
			/// [JP] 1回の廃棄でバッファは倍になるので、現実的な成長はこの程度の数で足りる。
			garbage_.reserve(32);
		}

		/**
		* [EN]
		* Destroys the queue, freeing both the currently active buffer
		* and any retired buffers kept in the garbage list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューを破棄し、現在使用中のバッファと、ガベージリストに
		* 保持されている廃棄済みバッファの両方を解放する。
		*/
		~UnboundedWorkerQueue()
		{
			/// [EN] No thread may steal any more at this point, so the retired buffers can finally be freed.
			/// [JP] この時点では誰も steal しないので、廃棄済みのバッファをようやく解放できる。
			for (auto garbage : garbage_)
			{
				delete garbage;
			}
			delete array_.load();
		}

		/**
		* [EN]
		* Returns whether the queue currently has no elements.
		* This is only a snapshot and may be stale if other threads
		* are concurrently modifying the queue.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューが現在要素を持たないかどうかを返す。これは単なる
		* スナップショットであり、他のスレッドが並行してキューを
		* 変更している場合は古い情報になる可能性がある。
		*/
		Bool empty()const noexcept
		{
			/// [EN] Relaxed loads are enough for a snapshot that may already be out of date when it returns.
			/// [JP] 戻った時点で古くなっているかもしれない一時点の値なので、relaxed の読み取りで足りる。
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return (bottom <= top);
		}

		/**
		* [EN]
		* Returns the approximate number of elements currently in the
		* queue, computed from a snapshot of top_/bottom_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* top_/bottom_ のスナップショットから計算した、キュー内の
		* 概算の要素数を返す。
		*/
		Size size()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return static_cast<Size>(bottom >= top ? bottom - top : 0);
		}

		/**
		* [EN]
		* Returns the capacity of the currently active internal buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在使用中の内部バッファの容量を返す。
		*/
		Size capacity()const noexcept
		{
			/// [EN] The buffer may be replaced right after this read; the value is only a hint.
			/// [JP] この読み取りの直後にバッファが置き換わることもあるので、値は目安でしかない。
			return array_.load(std::memory_order_relaxed)->capacity();
		}

		/**
		* [EN]
		* Pushes a single item onto the bottom of the queue. Must only
		* be called by the owning (producer) thread. Resizes the
		* internal buffer automatically if it is full.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの底（bottom）に単一の要素を push する。所有スレッド
		* （プロデューサー）からのみ呼び出すこと。内部バッファが満杯の
		* 場合は自動的にリサイズされる。
		*/
		void push(T item)
		{
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Array* array = array_.load(std::memory_order_relaxed);

			/// [EN] If the cached top suggests the buffer might be full, re-read the real top and check again before resizing.
			/// [JP] キャッシュした top から見てバッファが満杯になりそうな場合、実際の top を再読込し、リサイズ前に再確認する。
			if (array->capacity() < static_cast<Size>(bottom - cacheTop_ + 1)) [[unlikely]]
			{
				cacheTop_ = top_.load(std::memory_order_acquire);
				if (array->capacity() < static_cast<Size>(bottom - cacheTop_ + 1)) [[unlikely]]
				{
					array = resize_array(array, bottom, cacheTop_);
				}
			}

			array->push(bottom, item);

			/// [EN] Fence ensures the item write is visible before bottom_ is published, so concurrent stealers never observe a half-written slot.
			/// [JP] 要素の書き込みが bottom_ の公開より前に確実に見えるようにするフェンス。これにより並行する steal が未完了のスロットを観測しないようにする。
			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom + 1, std::memory_order_release);
		}

		/**
		* [EN]
		* Pushes n items, read sequentially from the iterator first,
		* onto the bottom of the queue in a single batch. Must only be
		* called by the owning (producer) thread. Resizes the internal
		* buffer automatically if there is not enough room.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* イテレータ first から順次読み取った n 個の要素を、まとめて
		* キューの底（bottom）に push する。所有スレッド（プロデューサー）
		* からのみ呼び出すこと。容量が不足している場合は内部バッファが
		* 自動的にリサイズされる。
		*/
		template <typename I>
		void bulk_push(I& first, Size n)
		{
			if (n == 0)
			{
				return;
			}

			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Array* array = array_.load(std::memory_order_relaxed);

			/// [EN] Same check as push, for n items at once: the cached top first, then the real one before resizing.
			/// [JP] push と同じ確認を n 個まとめて行う。まずキャッシュした top、リサイズ前に実際の top で確かめる。
			if ((bottom - cacheTop_ + n) > array->capacity()) [[unlikely]]
			{
				cacheTop_ = top_.load(std::memory_order_acquire);
				if ((bottom - cacheTop_ + n) > array->capacity()) [[unlikely]]
				{
					array = resize_array(array, bottom, cacheTop_, n);
				}
			}

			/// [EN] first is advanced as items are read, so the caller sees where the pushed range ended.
			/// [JP] 読み取りに合わせて first を進めるので、呼び出し側は push した範囲の終わりが分かる。
			for (Size index = 0;index < n;++index)
			{
				array->push(bottom++, *first++);
			}

			/// [EN] All items become visible to stealers at once, when bottom_ is published after the fence.
			/// [JP] フェンスの後に bottom_ を公開した時点で、全要素がまとめて steal 側から見えるようになる。
			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom, std::memory_order_release);
		}

		/**
		* [EN]
		* Pops a single item from the bottom of the queue. Must only be
		* called by the owning (producer) thread. Returns the empty
		* sentinel value if the queue is empty or if a concurrent
		* steal won the race for the last element.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの底（bottom）から単一の要素を pop する。所有スレッド
		* （プロデューサー）からのみ呼び出すこと。キューが空の場合、または
		* 最後の要素を巡って並行する steal との競合に敗れた場合は、
		* 空を表すセンチネル値を返す。
		*/
		ValueType pop()
		{
			/// [EN] bottom_ is lowered first and fenced before top_ is read, so a stealer sees the claim on the slot.
			/// [JP] top_ を読む前に bottom_ を下げてフェンスを挟む。これで steal 側がスロットの確保を見られる。
			Int64 bottom = bottom_.load(std::memory_order_relaxed) - 1;
			Array* array = array_.load(std::memory_order_relaxed);
			bottom_.store(bottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 top = top_.load(std::memory_order_relaxed);

			auto item = empty_value();

			if (top <= bottom)
			{
				/// [EN] At least one element remains; it is read first, and only the last-element case below needs to race steal().
				/// [JP] 少なくとも1個の要素が残っている。まず読み取り、steal() と競う必要があるのは下の最後の1個の場合だけ。
				item = array->pop(bottom);
				if (top == bottom)
				{
					/// [EN] Exactly one element remained, so race against concurrent steal() using CAS on top_.
					/// [JP] 残っていた要素がちょうど 1 個だったため、top_ への CAS によって並行する steal() との競合を解決する。
					if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
					{
						/// [EN] Lost the race to a concurrent steal(): no element was actually obtained.
						/// [JP] 並行する steal() との競合に敗れたため、実際には要素を取得できなかった。
						item = empty_value();
					}
					bottom_.store(bottom + 1, std::memory_order_relaxed);
				}
			}
			else
			{
				/// [EN] Queue was already empty: restore bottom_ to its consistent state.
				/// [JP] キューはすでに空だったため、bottom_ を整合した状態に戻す。
				bottom_.store(bottom + 1, std::memory_order_relaxed);
			}

			return item;
		}

		/**
		* [EN]
		* Attempts to steal a single item from the top of the queue.
		* Safe to call concurrently from multiple non-owning (consumer)
		* threads. Returns the empty sentinel value if the queue is
		* empty or if the steal lost a race with another steal/pop.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの頂点（top）から単一の要素を奪取（steal）しようとする。
		* 所有者でない複数の（コンシューマー）スレッドから並行して呼び出す
		* ことが安全である。キューが空の場合、または他の steal/pop との
		* 競合に敗れた場合は、空を表すセンチネル値を返す。
		*/
		ValueType steal()
		{
			/// [EN] top is read before bottom with a full fence between, pairing with the fence in pop() so both never take the last element.
			/// [JP] top を bottom より先に、間に完全なフェンスを挟んで読む。pop() 側のフェンスと対になり、最後の1個を両方が取ることは無い。
			Int64 top = top_.load(std::memory_order_acquire);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 bottom = bottom_.load(std::memory_order_acquire);

			auto item = empty_value();

			if (top < bottom)
			{
				/// [EN] An element may be available: read the current buffer pointer and load the value at top; retired buffers stay alive, so an old pointer is still safe to read.
				/// [JP] 要素がある可能性があるので、今のバッファを読み top の値を取る。廃棄済みバッファも生きているので、古いポインタでも安全に読める。
				Array* array = array_.load(std::memory_order_consume);
				item = array->pop(top);
				if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				{
					/// [EN] Lost the race against another steal() or pop(): discard the speculatively read value.
					/// [JP] 他の steal() または pop() との競合に敗れたため、推測的に読み取った値を破棄する。
					return empty_value();
				}
			}

			return item;
		}

		/**
		* [EN]
		* Returns the sentinel "empty" value returned by pop()/steal()
		* when there is no item (see WorkerQueueEmptyValue).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素が無い場合に pop()/steal() が返す「空」を表すセンチネル値を
		* 返す（WorkerQueueEmptyValue を参照）。
		*/
		static constexpr auto empty_value()
		{
			return WorkerQueueEmptyValue<T>();
		}

	private:
		/**
		* [EN]
		* Grows the internal buffer to accommodate the current elements,
		* retiring the old array into the garbage list and installing
		* the newly allocated buffer as the active one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の要素を収容できるよう内部バッファを拡張し、古い array を
		* ガベージリストへ廃棄として登録し、新しく確保したバッファを
		* アクティブなバッファとして設定する。
		*/
		Array* resize_array(Array* array, Int64 bottom, Int64 top)
		{
			/// [EN] The old buffer is kept rather than freed, because a stealer may have loaded its pointer just before the swap.
			/// [JP] 古いバッファは解放せずに残す。入れ替えの直前に steal 側がそのポインタを読んでいる可能性があるため。
			Array* temporary = array->resize(bottom, top);
			garbage_.push_back(array);
			array_.store(temporary, std::memory_order_release);
			return temporary;
		}

		/**
		* [EN]
		* Grows the internal buffer to accommodate the current elements
		* plus n additional ones, retiring the old array into the
		* garbage list and installing the newly allocated buffer as the
		* active one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の要素に加えて n 個分の追加要素を収容できるよう内部バッファを
		* 拡張し、古い array をガベージリストへ廃棄として登録し、新しく
		* 確保したバッファをアクティブなバッファとして設定する。
		*/
		Array* resize_array(Array* array, Int64 bottom, Int64 top, Size n)
		{
			/// [EN] The old buffer is kept rather than freed, because a stealer may have loaded its pointer just before the swap.
			/// [JP] 古いバッファは解放せずに残す。入れ替えの直前に steal 側がそのポインタを読んでいる可能性があるため。
			Array* temporary = array->resize(bottom, top, n);
			garbage_.push_back(array);
			array_.store(temporary, std::memory_order_release);
			return temporary;
		}
	};

	/**
	* [EN]
	* A Chase-Lev style work-stealing deque backed by a fixed-size
	* ring buffer of 1 << logSize slots. Unlike UnboundedWorkerQueue,
	* this queue never reallocates; try_push/try_bulk_push simply
	* fail when the buffer is full.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1 << logSize 個のスロットからなる固定サイズのリングバッファに
	* 裏付けられた、Chase-Lev 方式のワークスティーリング用デック。
	* UnboundedWorkerQueue とは異なり、このキューは再確保を行わず、
	* バッファが満杯の場合は try_push/try_bulk_push が単に失敗する。
	*/
	template <typename T, Size logSize = SC_DEFAULT_BOUNDED_QUEUE_LOG_SIZE>
	class BoundedWorkerQueue
	{
	private:
		/// [EN] Total number of slots in the fixed-size buffer (1 << logSize).
		/// [JP] 固定サイズバッファのスロット総数（1 << logSize）。
		constexpr static Size bufferSize = Size{ 1 } << logSize;

		/// [EN] Bitmask (bufferSize - 1) used to wrap indices around the ring buffer.
		/// [JP] リングバッファのインデックスを折り返すために使用するビットマスク（bufferSize - 1）。
		constexpr static Size bufferMask = (bufferSize - 1);

		/// [EN] The index mask only works for a power-of-two size of at least 2.
		/// [JP] インデックスのマスクは、2 以上の 2 のべき乗の大きさでしか使えない。
		static_assert((bufferSize >= 2) && ((bufferSize& (bufferSize - 1)) == 0));

		/// [EN] Index of the "top" end of the deque, from which other threads steal. Cache-line aligned to avoid false sharing.
		/// [JP] デックの「頂点（top）」側のインデックス。他のスレッドはここから steal を行う。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> top_{ 0 };
		
		/// [EN] Index of the "bottom" end of the deque, from which the owning thread pushes/pops. Cache-line aligned to avoid false sharing.
		/// [JP] デックの「底（bottom）」側のインデックス。所有スレッドがここから push/pop を行う。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<Int64> bottom_{ 0 };

		/// [EN] Fixed-size array of atomic slots backing the queue. Cache-line aligned to avoid false sharing.
		/// [JP] キューを支える、固定サイズのアトミックスロット配列。偽共有を避けるためキャッシュライン境界に整列。
		alignas(SC_CACHELINE_SIZE) std::atomic<T> buffer_[bufferSize];

	public:
		/// [EN] The value type returned by pop()/steal(): T itself if T is a pointer, otherwise std::optional<T>.
		/// [JP] pop()/steal() が返す値の型。T がポインタ型ならば T そのもの、それ以外なら std::optional<T>。
		using ValueType = std::conditional_t<std::is_pointer_v<T>, T, std::optional<T>>;

		/**
		* [EN]
		* Constructs an empty queue; top_ and bottom_ start at zero.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空のキューを構築する。top_ と bottom_ は 0 から始まる。
		*/
		BoundedWorkerQueue() = default;

		/**
		* [EN]
		* Destroys the queue; nothing is allocated, so there is nothing
		* to free.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューを破棄する。何も確保していないので、解放するものは無い。
		*/
		~BoundedWorkerQueue() = default;

		/**
		* [EN]
		* Returns whether the queue currently has no elements.
		* This is only a snapshot and may be stale if other threads
		* are concurrently modifying the queue.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューが現在要素を持たないかどうかを返す。これは単なる
		* スナップショットであり、他のスレッドが並行してキューを
		* 変更している場合は古い情報になる可能性がある。
		*/
		Bool empty()const noexcept
		{
			/// [EN] Relaxed loads are enough for a snapshot that may already be out of date when it returns.
			/// [JP] 戻った時点で古くなっているかもしれない一時点の値なので、relaxed の読み取りで足りる。
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return bottom <= top;
		}

		/**
		* [EN]
		* Returns the approximate number of elements currently in the
		* queue, computed from a snapshot of top_/bottom_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* top_/bottom_ のスナップショットから計算した、キュー内の
		* 概算の要素数を返す。
		*/
		Size size()const noexcept
		{
			Int64 top = top_.load(std::memory_order_relaxed);
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			return static_cast<Size>(bottom >= top ? bottom - top : 0);
		}

		/**
		* [EN]
		* Returns the fixed capacity of the queue's internal buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キュー内部バッファの固定容量を返す。
		*/
		constexpr Size capacity()const
		{
			return bufferSize;
		}

		/**
		* [EN]
		* Attempts to push a single item onto the bottom of the queue.
		* Must only be called by the owning (producer) thread. Returns
		* false without modifying the queue if the buffer is full.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの底（bottom）に単一の要素を push しようとする。所有スレッド
		* （プロデューサー）からのみ呼び出すこと。バッファが満杯の場合は
		* キューを変更せずに false を返す。
		*/
		template <typename O>
		Bool try_push(O&& item)
		{
			/// [EN] top is read with acquire, so slots freed by stealers are known to be reusable.
			/// [JP] top を acquire で読むので、steal 側が空けたスロットを再利用してよいと分かる。
			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Int64 top = top_.load(std::memory_order_acquire);

			if (static_cast<Size>(bottom - top + 1) > bufferSize) [[unlikely]]
			{
				return false;
			}

			buffer_[bottom & bufferMask].store(std::forward<O>(item), std::memory_order_relaxed);

			/// [EN] The item is made visible before bottom_ is published, so a stealer never reads a half-written slot.
			/// [JP] bottom_ を公開する前に要素を見えるようにするので、steal 側が書きかけのスロットを読むことは無い。
			std::atomic_thread_fence(std::memory_order_release);

			bottom_.store(bottom + 1, std::memory_order_release);

			return true;
		}

		/**
		* [EN]
		* Attempts to push up to n items, read sequentially from the
		* iterator first, onto the bottom of the queue. Pushes as
		* many as fit in the remaining capacity and returns the actual
		* number of items pushed. Must only be called by the owning
		* (producer) thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* イテレータ first から順次読み取った最大 n 個の要素を、
		* キューの底（bottom）に push しようとする。残りの容量に収まる
		* だけの個数を push し、実際に push した個数を返す。所有スレッド
		* （プロデューサー）からのみ呼び出すこと。
		*/
		template <typename I>
		Size try_bulk_push(I& first, Size n)
		{
			if (n == 0)
			{
				return 0;
			}

			Int64 bottom = bottom_.load(std::memory_order_relaxed);
			Int64 top = top_.load(std::memory_order_acquire);

			/// [EN] Clamp the number of items actually pushed to whatever room remains in the fixed-size buffer.
			/// [JP] 実際に push する個数を、固定サイズバッファに残っている空き容量に応じて制限する。
			Size remaining = bufferSize - (bottom - top);
			Size number = Min(n, remaining);

			/// [EN] first is advanced for every item pushed, so the caller can spill the rest from where it stops.
			/// [JP] push した要素ごとに first を進めるので、呼び出し側は止まった所から残りをあふれさせられる。
			if (n > 0)
			{
				for (Size index = 0;index < number;index++)
				{
					buffer_[bottom++ & bufferMask].store(*first++, std::memory_order_relaxed);
				}
				std::atomic_thread_fence(std::memory_order_release);

				bottom_.store(bottom, std::memory_order_release);
			}

			return number;
		}

		/**
		* [EN]
		* Pops a single item from the bottom of the queue. Must only be
		* called by the owning (producer) thread. Returns the empty
		* sentinel value if the queue is empty or if a concurrent
		* steal won the race for the last element.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの底（bottom）から単一の要素を pop する。所有スレッド
		* （プロデューサー）からのみ呼び出すこと。キューが空の場合、または
		* 最後の要素を巡って並行する steal との競合に敗れた場合は、
		* 空を表すセンチネル値を返す。
		*/
		ValueType pop()
		{
			/// [EN] bottom_ is lowered first and fenced before top_ is read, so a stealer sees the claim on the slot.
			/// [JP] top_ を読む前に bottom_ を下げてフェンスを挟む。これで steal 側がスロットの確保を見られる。
			Int64 bottom = bottom_.load(std::memory_order_relaxed) - 1;
			bottom_.store(bottom, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 top = top_.load(std::memory_order_relaxed);

			auto item = empty_value();

			if (top <= bottom)
			{
				/// [EN] At least one element remains; it is read first, and only the last-element case below needs to race steal().
				/// [JP] 少なくとも1個の要素が残っている。まず読み取り、steal() と競う必要があるのは下の最後の1個の場合だけ。
				item = buffer_[bottom & bufferMask].load(std::memory_order_relaxed);
				if (top == bottom)
				{
					/// [EN] Exactly one element remained, so race against concurrent steal() using CAS on top_.
					/// [JP] 残っていた要素がちょうど 1 個だったため、top_ への CAS によって並行する steal() との競合を解決する。
					if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
					{
						/// [EN] Lost the race to a concurrent steal(): no element was actually obtained.
						/// [JP] 並行する steal() との競合に敗れたため、実際には要素を取得できなかった。
						item = empty_value();
					}
					bottom_.store(bottom + 1, std::memory_order_relaxed);
				}
			}
			else
			{
				/// [EN] Queue was already empty: restore bottom_ to its consistent state.
				/// [JP] キューはすでに空だったため、bottom_ を整合した状態に戻す。
				bottom_.store(bottom + 1, std::memory_order_relaxed);
			}

			return item;
		}

		/**
		* [EN]
		* Attempts to steal a single item from the top of the queue.
		* Safe to call concurrently from multiple non-owning (consumer)
		* threads. Returns the empty sentinel value if the queue is
		* empty or if the steal lost a race with another steal/pop.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューの頂点（top）から単一の要素を奪取（steal）しようとする。
		* 所有者でない複数の（コンシューマー）スレッドから並行して呼び出す
		* ことが安全である。キューが空の場合、または他の steal/pop との
		* 競合に敗れた場合は、空を表すセンチネル値を返す。
		*/
		ValueType steal()
		{
			/// [EN] top is read before bottom with a full fence between, pairing with the fence in pop() so both never take the last element.
			/// [JP] top を bottom より先に、間に完全なフェンスを挟んで読む。pop() 側のフェンスと対になり、最後の1個を両方が取ることは無い。
			Int64 top = top_.load(std::memory_order_acquire);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			Int64 bottom = bottom_.load(std::memory_order_acquire);

			auto item = empty_value();

			if (top < bottom)
			{
				/// [EN] An element may be available: speculatively load the value at top.
				/// [JP] 要素が存在する可能性があるため、top の値を推測的に読み込む。
				item = buffer_[top & bufferMask].load(std::memory_order_relaxed);
				if (!top_.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
				{
					/// [EN] Lost the race against another steal() or pop(): discard the speculatively read value.
					/// [JP] 他の steal() または pop() との競合に敗れたため、推測的に読み取った値を破棄する。
					return empty_value();
				}
			}

			return item;
		}

		/**
		* [EN]
		* Returns the sentinel "empty" value returned by pop()/steal()
		* when there is no item (see WorkerQueueEmptyValue).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要素が無い場合に pop()/steal() が返す「空」を表すセンチネル値を
		* 返す（WorkerQueueEmptyValue を参照）。
		*/
		static constexpr auto empty_value()
		{
			return WorkerQueueEmptyValue<T>();
		}
	};
}
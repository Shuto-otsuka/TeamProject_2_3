#pragma once

namespace SeedCore
{
	class BenchmarkThread;

	/**
	* [EN]
	* Entry point for measuring a CPU section: wrap the code in Begin/End with
	* the same name. Calls nest, and each finished section is appended to the
	* calling thread's buffer; a thread that BenchmarkSystem has not attached
	* records nothing. The name is kept by address, so it must outlive the
	* recording: a string literal, or an interned String.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CPU 区間を測る入口。コードを同じ名前の Begin/End で囲む。呼び出しは
	* 入れ子にでき、終わった区間は呼んだスレッドのバッファに書き足される。
	* BenchmarkSystem が Attach していないスレッドでは何も記録しない。名前は
	* アドレスで保持するので、記録より長生きするもの（文字列リテラルか、
	* インターン済みの String）でなければならない。
	*/
	class SEEDCORE_API Benchmark
	{
	private:
		friend class BenchmarkSystem;

	public:
		/**
		* [EN]
		* Opens a section named by a string literal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 文字列リテラルの名前で区間を開く。
		*/
		template<Size Length>
		static void Begin(const Char(&name)[Length])
		{
			BeginLiteral(name);
		}

		/**
		* [EN]
		* Closes the innermost section, which must have the same name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 一番内側の区間を閉じる。名前は Begin と同じでなければならない。
		*/
		template<Size Length>
		static void End(const Char(&name)[Length])
		{
			EndLiteral(name);
		}

		/**
		* [EN]
		* Opens a section whose name is decided at run time, such as a
		* component type name. An interned String keeps its address for the
		* life of the pool, so it is as safe to hold as a literal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コンポーネントの型名など、実行時に決まる名前で区間を開く。インターン
		* 済みの String はプールが生きている間アドレスが変わらないので、リテラルと
		* 同じく保持して安全。
		*/
		static void Begin(const String& name);

		/**
		* [EN]
		* Closes the innermost section opened with the same String.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 同じ String で開いた一番内側の区間を閉じる。
		*/
		static void End(const String& name);

	private:
		/**
		* [EN]
		* Pushes the section onto the calling thread's open stack with the current tick.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼んだスレッドの開いている区間のスタックへ、今の tick と一緒に積む。
		*/
		static void BeginLiteral(const Char* name);

		/**
		* [EN]
		* Pops the innermost section and appends it as a finished event.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 一番内側の区間を降ろし、終わったイベントとして書き足す。
		*/
		static void EndLiteral(const Char* name);

		/**
		* [EN]
		* Binds the calling thread to its buffer, or unbinds it with nullptr.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 呼んだスレッドをバッファに結び付ける。nullptr なら外す。
		*/
		static void Bind(BenchmarkThread* thread);
	};
}
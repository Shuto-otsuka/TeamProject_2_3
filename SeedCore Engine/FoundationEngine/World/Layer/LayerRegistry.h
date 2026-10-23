#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Global registry of the engine's fixed-count Actor layer slots
	* (index 0 reserved as "Default"), used for query filtering (Physics::
	* Raycast/Spherecast/Overlap's layerMask) rather than physics
	* collision response - that is still governed separately by Jolt's
	* own STATIC/KINEMATIC/DYNAMIC ObjectLayer (see JoltLayerdef.h).
	* Unlike TagRegistry (any number of tags per Actor, dynamically
	* growing), the slot count here is fixed at LayerCount (comfortably
	* within a Uint32 layerMask's bit width) and each Actor belongs to
	* exactly one slot at a time. Every slot always has a name - a slot
	* never renamed is "Layer N" (index 0 is "Default"), and an empty name
	* is never accepted. Names, together with LayerCollisionMatrix's
	* collision entries, persist to a single LayerBindings.scg file via
	* Save()/Load().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* エンジンの固定数の Actor レイヤースロット（インデックス0は
	* "Default"として予約）を管理するグローバルレジストリ。物理の衝突応答
	* （こちらは引き続き Jolt 自身の STATIC/KINEMATIC/DYNAMIC ObjectLayer
	* が別途担う。JoltLayerdef.h 参照）ではなく、クエリの絞り込み
	* （Physics::Raycast/Spherecast/Overlap の layerMask）に使う。
	* TagRegistry（Actor 1つにつき任意個数のタグ、動的に増える）と異なり、
	* こちらはスロット数が LayerCount 固定（Uint32 の layerMask のビット幅に
	* 余裕を持って収まる）で、各 Actor は常にちょうど1つのスロットに属する。
	* 全スロットは常に名前を持つ。一度も名前を変えていないスロットは
	* "Layer N"（インデックス0は"Default"）で、空の名前は受け付けない。
	* 名前は LayerCollisionMatrix の衝突エントリと合わせて、Save()/Load()
	* 経由で1つの LayerBindings.scg ファイルへ永続化される。
	*/
	class SEEDCORE_API LayerRegistry
	{
	public:
		/// [EN] Fixed number of layer slots, matching the bit width of a Uint32 layerMask.
		/// [JP] レイヤースロットの固定数。Uint32 の layerMask のビット幅と一致する。
		SC_CONSTEX Size LayerCount = 16;

		/// [EN] Index of the built-in "Default" layer every new Actor starts in.
		/// [JP] 新規 Actor が最初に属する、組み込みの"Default"レイヤーのインデックス。
		SC_CONSTEX Size DefaultLayer = 0;

	public:
		/**
		* [EN]
		* Renames the layer slot at index; does nothing if index is out of
		* range or name is empty, so every slot keeps a name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index のレイヤースロットの名前を変更する。index が範囲外か name が
		* 空なら何もしないので、どのスロットも名前を持ち続ける。
		*/
		static void Name(Size index, String name);

		/**
		* [EN]
		* Returns the name of the layer slot at index, or an empty String if
		* index is out of range.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* index のレイヤースロットの名前を返す。index が範囲外なら空の
		* String。
		*/
		static const String& Name(Size index);

		/**
		* [EN]
		* Returns the index of the layer slot named name, or DefaultLayer if
		* no slot has that name.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* name という名前のレイヤースロットのインデックスを返す。該当する
		* スロットが無ければ DefaultLayer を返す。
		*/
		static Size Find(const String& name);

		/**
		* [EN]
		* Returns every layer slot's name, indexed by slot index (always
		* LayerCount entries long).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全レイヤースロットの名前を、スロットインデックスでアクセス
		* できる形で返す（常に LayerCount 個）。
		*/
		static std::span<const String> Names();

		/**
		* [EN]
		* Reads layer names and LayerCollisionMatrix's collision entries
		* from a single combined LayerBindings.scg (JSON) at path,
		* replacing both's current in-memory data. No-op if the file
		* doesn't exist or can't be read. Names always end up exactly
		* LayerCount entries long, and a missing or empty name falls back
		* to "Layer N" ("Default" for index 0); LayerCollisionMatrix's
		* entries are left at default instead of being resized if their
		* count doesn't match.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1つにまとまった LayerBindings.scg（JSON、path）から、レイヤー名と
		* LayerCollisionMatrix の衝突エントリを読み込み、両方の現在の
		* メモリ上のデータを置き換える。ファイルが存在しない/読み込めない
		* 場合は何もしない。名前は常にちょうど LayerCount 個になり、無いか
		* 空の名前は "Layer N"（インデックス0は"Default"）になる。
		* LayerCollisionMatrix のエントリは、個数が一致しない場合はリサイズ
		* せず既定値のままにする。
		*/
		static void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/LayerBindings.scg");

		/**
		* [EN]
		* Writes layer names and LayerCollisionMatrix's collision entries
		* together to path as a single LayerBindings.scg (JSON).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイヤー名と LayerCollisionMatrix の衝突エントリを、1つにまとめて
		* JSON（LayerBindings.scg）として path へ書き込む。
		*/
		static void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/LayerBindings.scg");

	private:
		/**
		* [EN]
		* Returns the backing name array, giving every slot its first name
		* ("Default" for DefaultLayer, "Layer N" for the others) on first
		* use.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 内部の名前配列を返す。初回使用時に、全スロットに最初の名前
		* （DefaultLayer は"Default"、それ以外は"Layer N"）を付ける。
		*/
		static DynamicArray<String>& Storage();
	};
}

#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
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
	void LayerRegistry::Name(Size index, String name)
	{
		if (index >= LayerCount || name == String{})
		{
			return;
		}

		Storage()[index] = name;
	}

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
	const String& LayerRegistry::Name(Size index)
	{
		static const String empty;

		if (index >= LayerCount)
		{
			return empty;
		}

		return Storage()[index];
	}

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
	Size LayerRegistry::Find(const String& name)
	{
		/// [EN] There are only LayerCount slots, so a linear search is enough.
		/// [JP] スロットは LayerCount 個しか無いので、線形探索で足りる。
		DynamicArray<String>& names = Storage();
		auto found = std::ranges::find(names, name);
		return found != names.end() ? static_cast<Size>(found - names.begin()) : DefaultLayer;
	}

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
	std::span<const String> LayerRegistry::Names()
	{
		DynamicArray<String>& names = Storage();
		return std::span<const String>(names.data(), names.size());
	}

	/**
	* [EN]
	* Reads layer names and LayerCollisionMatrix's collision entries
	* from a single combined LayerBindings.scg (JSON) at path, replacing
	* both's current in-memory data. No-op if the file doesn't exist or
	* can't be read. Names always end up exactly LayerCount entries long,
	* and a missing or empty name falls back to "Layer N" ("Default" for
	* index 0); LayerCollisionMatrix's entries are left at default instead
	* of being resized if their count doesn't match.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つにまとまった LayerBindings.scg（JSON、path）から、レイヤー名と
	* LayerCollisionMatrix の衝突エントリを読み込み、両方の現在のメモリ上の
	* データを置き換える。ファイルが存在しない/読み込めない場合は何もしない。
	* 名前は常にちょうど LayerCount 個になり、無いか空の名前は "Layer N"
	* （インデックス0は"Default"）になる。LayerCollisionMatrix のエントリは、
	* 個数が一致しない場合はリサイズせず既定値のままにする。
	*/
	void LayerRegistry::Load(const std::filesystem::path& path)
	{
		BinaryInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			return;
		}

		DynamicArray<String>& names = Storage();
		archive.TryField("names", names);

		if (names.size() != LayerCount)
		{
			names.resize(LayerCount);
		}

		/// [EN] A file written with fewer slots, or with a cleared name, leaves gaps that get the slot's first name back.
		/// [JP] スロットが少ない時に書かれたファイルや、名前を消したファイルは隙間を残すので、そこにはスロットの最初の名前を戻す。
		for (Size index = 0; index < LayerCount; ++index)
		{
			if (names[index] == String{})
			{
				if (index == DefaultLayer)
				{
					names[index] = String("Default");
				}
				else
				{
					std::string name = "Layer " + std::to_string(index);
					names[index] = String(std::string_view(name));
				}
			}
		}

		LayerCollisionMatrix::Load(archive);
	}

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
	void LayerRegistry::Save(const std::filesystem::path& path)
	{
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		BinaryOutputArchive archive;
		archive.Field("names", Storage());
		LayerCollisionMatrix::Save(archive);

		archive.Write(String(path.string()));
	}

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
	DynamicArray<String>& LayerRegistry::Storage()
	{
		/// [EN] A function-local static is built on first use, so it is ready even when another static's initialization asks for a layer name.
		/// [JP] 関数内の static は初回使用時に作られるので、他の static の初期化中にレイヤー名を求められても用意できている。
		static DynamicArray<String> names = []()
			{
				DynamicArray<String> initial(LayerCount);
				for (Size index = 0; index < LayerCount; ++index)
				{
					if (index == DefaultLayer)
					{
						initial[index] = String("Default");
					}
					else
					{
						std::string name = "Layer " + std::to_string(index);
						initial[index] = String(std::string_view(name));
					}
				}
				return initial;
			}();

		return names;
	}
}

#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	void LayerRegistry::Name(Size index, String name)
	{
		if (index >= LayerCount || name == String{})
		{
			return;
		}

		Storage()[index] = name;
	}

	const String& LayerRegistry::Name(Size index)
	{
		static const String empty;

		if (index >= LayerCount)
		{
			return empty;
		}

		return Storage()[index];
	}

	Size LayerRegistry::Find(const String& name)
	{
		DynamicArray<String>& names = Storage();
		auto found = std::ranges::find(names, name);
		return found != names.end() ? static_cast<Size>(found - names.begin()) : DefaultLayer;
	}

	std::span<const String> LayerRegistry::Names()
	{
		DynamicArray<String>& names = Storage();
		return std::span<const String>(names.data(), names.size());
	}

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

	DynamicArray<String>& LayerRegistry::Storage()
	{
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

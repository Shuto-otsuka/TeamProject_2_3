#include <GraphicsEngine/Effect/Zephyr/ZephyrNodeRegistry.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/File/FileLocator.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	const DynamicArray<ZephyrNodeDefinition>& ZephyrNodeRegistry::Nodes()
	{
		static const DynamicArray<ZephyrNodeDefinition> nodes = []()
			{
				StaticArray<std::pair<std::string, ZephyrType>, 13> types =
				{
					std::pair<std::string, ZephyrType>{ "float", ZephyrType::Float },
					std::pair<std::string, ZephyrType>{ "float2", ZephyrType::Float2 },
					std::pair<std::string, ZephyrType>{ "float3", ZephyrType::Float3 },
					std::pair<std::string, ZephyrType>{ "float4", ZephyrType::Float4 },
					std::pair<std::string, ZephyrType>{ "color", ZephyrType::Color },
					std::pair<std::string, ZephyrType>{ "int", ZephyrType::Int },
					std::pair<std::string, ZephyrType>{ "bool", ZephyrType::Bool },
					std::pair<std::string, ZephyrType>{ "numeric", ZephyrType::Numeric },
					std::pair<std::string, ZephyrType>{ "force", ZephyrType::Force },
					std::pair<std::string, ZephyrType>{ "position", ZephyrType::Position },
					std::pair<std::string, ZephyrType>{ "shape", ZephyrType::Shape },
					std::pair<std::string, ZephyrType>{ "renderer", ZephyrType::Renderer },
					std::pair<std::string, ZephyrType>{ "effekseer", ZephyrType::Effekseer },
				};
				auto parseType = [&types](const std::string& word, ZephyrType& type)
					{
						for (const std::pair<std::string, ZephyrType>& candidate : types)
						{
							if (candidate.first == word)
							{
								type = candidate.second;
								return true;
							}
						}
						return false;
					};

				DynamicArray<std::pair<std::string, std::string>> sources;
				std::filesystem::path folder = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr/Module";
				std::error_code error;
				if (std::filesystem::is_directory(folder, error))
				{
					for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder, error))
					{
						if (entry.path().extension() != ".hlsli")
						{
							continue;
						}

						std::ifstream file(entry.path(), std::ios::binary);
						sources.emplace_back(entry.path().filename().string(), std::string((std::istreambuf_iterator<Char>(file)), std::istreambuf_iterator<Char>()));
					}
				}
				else
				{
					BinaryInputArchive archive;
					DynamicArray<String> files;
					DynamicArray<String> annotations;
					if (archive.Read(String((FileLocator::EngineRoot() / "CompiledShaderObject/Application/Zephyr/Module.dx.cso").string())))
					{
						archive.TryField("files", files);
						archive.TryField("annotations", annotations);
					}
					for (Size index = 0;index < files.size() && index < annotations.size();++index)
					{
						sources.emplace_back(files[index].str(), annotations[index].str());
					}
				}

				DynamicArray<ZephyrNodeDefinition> table;
				for (const std::pair<std::string, std::string>& source : sources)
				{
					const std::string& filename = source.first;
					std::string include = "GraphicsEngine/Effect/Zephyr/Module/" + filename;
					std::istringstream file(source.second);
					std::string line;
					Size lineNumber = 0;
					Size current = SIZE_MAX;
					while (std::getline(file, line))
					{
						++lineNumber;
						if (!line.empty() && line.back() == '\r')
						{
							line.pop_back();
						}
						Size marker = line.find("// @");
						if (marker == std::string::npos)
						{
							continue;
						}

						std::istringstream words(line.substr(marker + 4));
						std::string keyword;
						words >> keyword;

						if (keyword == "node")
						{
							std::string type;
							words >> type;
							ZephyrNodeDefinition& node = table.emplace_back();
							node.type_ = String(type);
							node.label_ = String(type);
							node.category_ = String("その他");
							node.include_ = String(include);
							current = table.size() - 1;
							continue;
						}
						if (current == SIZE_MAX)
						{
							SC_LOG_ERROR("Zephyrシステム: {}:{} @node より前に @{} があります", filename, lineNumber, keyword);
							continue;
						}

						ZephyrNodeDefinition& node = table[current];
						if (keyword == "label" || keyword == "category")
						{
							std::string text;
							std::getline(words >> std::ws, text);
							(keyword == "label" ? node.label_ : node.category_) = String(text);
						}
						else if (keyword == "receiver")
						{
							node.removable_ = false;
							node.include_ = String();
						}
						else if (keyword == "output")
						{
							std::string typeWord;
							std::string name;
							words >> typeWord >> name;
							ZephyrNodeOutput output;
							if (!parseType(typeWord, output.type_))
							{
								SC_LOG_ERROR("Zephyrシステム: {}:{} 知らない型です: {}", filename, lineNumber, typeWord);
								continue;
							}
							if (name.empty())
							{
								name = typeWord;
								name[0] = static_cast<Char>(std::toupper(static_cast<Uint8>(name[0])));
							}
							output.name_ = String(name);
							if (output.type_ == ZephyrType::Renderer)
							{
								node.include_ = String();
							}
							node.outputs_.push_back(output);
						}
						else if (keyword == "input")
						{
							std::string typeWord;
							std::string name;
							words >> typeWord >> name;
							ZephyrNodeInput input;
							input.name_ = String(name);
							if (!parseType(typeWord, input.type_))
							{
								SC_LOG_ERROR("Zephyrシステム: {}:{} 知らない型です: {}", filename, lineNumber, typeWord);
								continue;
							}

							std::string rest;
							std::getline(words, rest);
							std::string flags = rest;
							Size equal = rest.find('=');
							if (equal != std::string::npos)
							{
								std::string value = rest.substr(equal + 1);
								std::string numbers;
								Size open = value.find('(');
								if (open != std::string::npos)
								{
									Size close = value.find(')', open);
									numbers = value.substr(open + 1, close - open - 1);
									flags = value.substr(close + 1);
								}
								else
								{
									std::istringstream valueWords(value);
									valueWords >> numbers;
									std::getline(valueWords, flags);
								}
								std::ranges::replace(numbers, ',', ' ');

								StaticArray<Float, 4> components = { 0.0f, 0.0f, 0.0f, 0.0f };
								std::istringstream componentWords(numbers);
								Size componentIndex = 0;
								Float component = 0.0f;
								while (componentIndex<components.size() && componentWords >> component)
								{
									components[componentIndex] = component;
									++componentIndex;
								}
								input.default_ = Vector4(components[0], components[1], components[2], components[3]);
							}
							input.pinned_ = flags.find("constant") == std::string::npos;
							input.multiple_ = flags.find("multiple") != std::string::npos;
							node.inputs_.push_back(input);
						}
						else
						{
							SC_LOG_ERROR("Zephyrシステム: {}:{} 知らない書き方です: @{}", filename, lineNumber, keyword);
						}
					}
				}
				return table;
			}();
		return nodes;
	}

	const ZephyrNodeDefinition* ZephyrNodeRegistry::Find(const String& type)
	{
		const DynamicArray<ZephyrNodeDefinition>& nodes = Nodes();
		auto found = std::ranges::find_if(nodes, [&type](const ZephyrNodeDefinition& node) { return node.type_ == type; });
		if (found == nodes.end())
		{
			return nullptr;
		}
		return &(*found);
	}

	Bool ZephyrNodeRegistry::Connectable(ZephyrType from, ZephyrType to)
	{
		if (from == to)
		{
			return true;
		}

		Bool fromValue = from == ZephyrType::Float || from == ZephyrType::Float2 || from == ZephyrType::Float3 || from == ZephyrType::Float4 || from == ZephyrType::Color || from == ZephyrType::Int;
		if (to == ZephyrType::Numeric)
		{
			return fromValue;
		}

		Bool toVector = to == ZephyrType::Float2 || to == ZephyrType::Float3 || to == ZephyrType::Float4;
		if (from == ZephyrType::Float)
		{
			return toVector || to == ZephyrType::Color;
		}
		if (from == ZephyrType::Int)
		{
			return to == ZephyrType::Float || toVector;
		}
		if (from == ZephyrType::Color)
		{
			return to == ZephyrType::Float4;
		}
		return false;
	}
}
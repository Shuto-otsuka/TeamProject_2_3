#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrNodeRegistry.h>
#include <GraphicsEngine/Effect/EffectAsset.h>
#include <FoundationEngine/File/FileLocator.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	ZephyrAssembly ZephyrModuleAssembler::Assemble(const EffectEmitter& emitter)
	{
		auto findNode = [&emitter](Uint32 nodeID) -> const EffectNode*
			{
				auto found = std::ranges::find_if(emitter.nodes_, [nodeID](const EffectNode& node) { return node.id_ == nodeID; });
				if (found == emitter.nodes_.end())
				{
					return nullptr;
				}
				return &(*found);
			};
		auto findReceiver = [&emitter](const String& type) -> const EffectNode*
			{
				auto found = std::ranges::find_if(emitter.nodes_, [&type](const EffectNode& node) { return node.type_ == type; });
				if (found == emitter.nodes_.end())
				{
					return nullptr;
				}
				return &(*found);
			};
		auto incoming = [&emitter](Uint32 nodeID, const String& pin) -> const EffectLink*
			{
				auto found = std::ranges::find_if(emitter.links_, [nodeID, &pin](const EffectLink& link) { return link.toNode_ == nodeID && link.toPin_ == pin; });
				if (found == emitter.links_.end())
				{
					return nullptr;
				}
				return &(*found);
			};

		const EffectNode* spawn = findReceiver(String("Spawn"));
		const EffectNode* simulate = findReceiver(String("Simulate"));
		Bool velocityChanged = spawn && std::ranges::any_of(spawn->parameters_, [](const EffectParameter& parameter) { return parameter.name_ == String("Velocity"); });

		Uint64 contentHash = 14695981039346656037ull;
		auto mix = [&contentHash](const std::string& text)
			{
				for (Char character : text)
				{
					contentHash ^= static_cast<Uint64>(static_cast<Uint8>(character));
					contentHash *= 1099511628211ull;
				}
				contentHash ^= static_cast<Uint64>(';');
				contentHash *= 1099511628211ull;
			};
		for (const EffectNode& node : emitter.nodes_)
		{
			mix(std::format("{}:{}", node.id_, node.type_.str()));
		}
		for (const EffectLink& link : emitter.links_)
		{
			mix(std::format("{}.{}>{}.{}", link.fromNode_, link.fromPin_.str(), link.toNode_, link.toPin_.str()));
		}
		mix(velocityChanged ? "velocity" : "");

		if (auto found = generatedCache_.find(contentHash);found != generatedCache_.end())
		{
			return found->second;
		}

		ZephyrAssembly assembly;
		DynamicArray<String> includes;

		auto include = [&includes](const String& path)
			{
				if (std::ranges::find(includes, path) == includes.end())
				{
					includes.push_back(path);
				}
			};
		auto typeName = [](ZephyrType type) -> std::string
			{
				switch (type)
				{
				case ZephyrType::Float:
					return "float";
				case ZephyrType::Float2:
					return "float2";
				case ZephyrType::Float3:
					return "float3";
				case ZephyrType::Int:
					return "int";
				case ZephyrType::Bool:
					return "bool";
				default:
					return "float4";
				}
			};
		auto variableOf = [](const EffectNode& node)
			{
				std::string variable = std::format("{}_{}_", node.type_.str(), node.id_);
				variable[0] = static_cast<Char>(std::tolower(static_cast<Uint8>(variable[0])));
				return variable;
			};
		auto row = [&assembly](const EffectNode& node, const ZephyrNodeInput& input) -> std::string
			{
				std::string value = std::format("parameters[base + {}]", assembly.rows_.size());
				assembly.rows_.push_back(ZephyrParameterRow{ node.id_, input.name_, input.default_ });
				switch (input.type_)
				{
				case ZephyrType::Float:
					return value + ".x";
				case ZephyrType::Float2:
					return value + ".xy";
				case ZephyrType::Float3:
					return value + ".xyz";
				case ZephyrType::Int:
					return "(int)" + value + ".x";
				case ZephyrType::Bool:
					return value + ".x != 0.0";
				default:
					return value;
				}
			};
		auto convert = [&typeName](const std::string& value, ZephyrType from, ZephyrType to) -> std::string
			{
				if (from == to)
				{
					return value;
				}
				if (from == ZephyrType::Float && to == ZephyrType::Color)
				{
					return std::format("float4({0}, {0}, {0}, 1.0)", value);
				}
				if (from == ZephyrType::Int)
				{
					return std::format("({})(float){}", typeName(to), value);
				}
				return std::format("({}){}", typeName(to), value);
			};

		auto expression = [&](auto& self, const EffectNode& node, const ZephyrNodeInput& input, std::ostringstream& code, FlatMap<Uint32, std::string>& results, Size depth) -> std::string
			{
				const EffectLink* link = incoming(node.id_, input.name_);
				const EffectNode* source = link ? findNode(link->fromNode_) : nullptr;
				const ZephyrNodeDefinition* definition = source ? ZephyrNodeRegistry::Find(source->type_) : nullptr;
				const ZephyrModuleDescriptor* module = source ? ZephyrModuleRegistry::Find(source->type_) : nullptr;
				if (!definition || !module || definition->outputs_.empty() || depth > emitter.nodes_.size())
				{
					return row(node, input);
				}

				ZephyrType output = definition->outputs_[0].type_;
				if (!results.contains(source->id_))
				{
					include(module->include_);
					std::string variable = variableOf(*source);
					code << "\t" << source->type_.str() << "Inputs " << variable << ";\n";
					for (const ZephyrNodeInput& sourceInput : definition->inputs_)
					{
						std::string value = self(self, *source, sourceInput, code, results, depth + 1);
						code << "\t" << variable << "." << sourceInput.name_.str() << "_ = " << value << ";\n";
					}
					std::string result = variable + "result_";
					code << "\t" << typeName(output) << " " << result << " = " << source->type_.str() << "(seed.seed_, " << variable << ");\n";
					results.insert({ source->id_, result });
				}
				return convert(results.at(source->id_), output, input.type_);
			};
		auto structure = [&](const EffectNode& node, std::ostringstream& code, FlatMap<Uint32, std::string>& results) -> std::string
			{
				const ZephyrNodeDefinition* definition = ZephyrNodeRegistry::Find(node.type_);
				std::string variable = variableOf(node);
				code << "\t" << node.type_.str() << "Inputs " << variable << ";\n";
				for (const ZephyrNodeInput& input : definition->inputs_)
				{
					std::string value = expression(expression, node, input, code, results, 0);
					code << "\t" << variable << "." << input.name_.str() << "_ = " << value << ";\n";
				}
				return variable;
			};

		std::ostringstream spawnCalls;
		FlatMap<Uint32, std::string> spawnResults;
		if (const ZephyrNodeDefinition* definition = spawn ? ZephyrNodeRegistry::Find(spawn->type_) : nullptr)
		{
			const EffectLink* shapeLink = incoming(spawn->id_, String("Shape"));
			const EffectNode* shapeNode = shapeLink ? findNode(shapeLink->fromNode_) : nullptr;
			const ZephyrModuleDescriptor* shapeModule = shapeNode ? ZephyrModuleRegistry::Find(shapeNode->type_) : nullptr;
			if (shapeModule)
			{
				include(shapeModule->include_);
				std::string variable = structure(*shapeNode, spawnCalls, spawnResults);
				spawnCalls << "\tParticleShape shape = " << shapeNode->type_.str() << "(seed.seed_, " << variable << ");\n";
				spawnCalls << "\tseed.position_ = mul(float4(shape.position_, 1.0), emitter.emitter_to_world_).xyz;\n";
			}

			for (const ZephyrNodeInput& input : definition->inputs_)
			{
				if (input.type_ == ZephyrType::Shape)
				{
					continue;
				}

				std::string value = expression(expression, *spawn, input, spawnCalls, spawnResults, 0);
				if (input.name_ == String("Lifetime"))
				{
					spawnCalls << "\tseed.lifetime_ = " << value << ";\n";
				}
				else if (input.name_ == String("Velocity"))
				{
					if (shapeModule && !velocityChanged && !incoming(spawn->id_, input.name_))
					{
						spawnCalls << "\tseed.base_velocity_ = normalize(mul(float4(shape.normal_, 0.0), emitter.emitter_to_world_).xyz) * length(" << value << ");\n";
					}
					else
					{
						spawnCalls << "\tseed.base_velocity_ = " << value << ";\n";
					}
					spawnCalls << "\tseed.live_velocity_ = seed.base_velocity_;\n";
				}
				else if (input.name_ == String("Color"))
				{
					spawnCalls << "\tseed.base_color_ = " << value << ";\n";
					spawnCalls << "\tseed.live_color_ = seed.base_color_;\n";
				}
				else if (input.name_ == String("Size"))
				{
					spawnCalls << "\tseed.base_size_ = " << value << ";\n";
					spawnCalls << "\tseed.live_size_ = seed.base_size_;\n";
				}
			}
		}

		std::ostringstream simulateCalls;
		FlatMap<Uint32, std::string> simulateResults;
		if (simulate)
		{
			for (const EffectLink& link : emitter.links_)
			{
				if (link.toNode_ != simulate->id_ || link.toPin_ != String("Force"))
				{
					continue;
				}

				const EffectNode* source = findNode(link.fromNode_);
				const ZephyrModuleDescriptor* module = source ? ZephyrModuleRegistry::Find(source->type_) : nullptr;
				if (!module || module->output_ != ZephyrType::Force)
				{
					continue;
				}

				include(module->include_);
				std::string variable = structure(*source, simulateCalls, simulateResults);
				simulateCalls << "\tforce += " << source->type_.str() << "(seed, " << variable << ", emitter);\n";
			}
		}

		std::ostringstream includeLines;
		for (const String& path : includes)
		{
			includeLines << "#include <" << path.str() << ">\n";
		}

		Uint32 shortHash = static_cast<Uint32>(contentHash ^ (contentHash >> 32));
		std::string fileName = std::format("Zephyr_{:08X}.hlsl", shortHash);
		std::filesystem::path outputPath = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr/Generated" / fileName;
		assembly.filePath_ = String(outputPath.string());

		std::filesystem::path templatePath = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr/ParticleTemplate.hlsli";
		std::ifstream templateFile(templatePath, std::ios::binary);
		if (!templateFile)
		{
			generatedCache_.insert({ contentHash, assembly });
			return assembly;
		}
		std::string text((std::istreambuf_iterator<Char>(templateFile)), std::istreambuf_iterator<Char>());

		StaticArray<std::pair<std::string, std::string>, 3> markers =
		{
			std::pair<std::string,std::string>{ "// @SC_ZEPHYR_INCLUDE", includeLines.str() },
			std::pair<std::string,std::string>{ "// @SC_ZEPHYR_SPAWN_CALL", spawnCalls.str() },
			std::pair<std::string,std::string>{ "// @SC_ZEPHYR_SIMULATE_CALL", simulateCalls.str() },
		};
		for (const std::pair<std::string, std::string>& marker : markers)
		{
			Size position = text.find(marker.first);
			if (position == std::string::npos)
			{
				SC_LOG_ERROR("Zephyrシステム: 目印が見つかりません: {}", marker.first);
				continue;
			}
			text.replace(position, marker.first.size(), marker.second);
		}

		std::error_code directoryError;

		std::filesystem::create_directories(outputPath.parent_path(), directoryError);

		std::ofstream outputFile(outputPath, std::ios::binary);
		if (!outputFile)
		{
			SC_LOG_ERROR("Zephyrシステム: 生成ファイルを書き出せません: {}", outputPath.string());
			return assembly;
		}
		outputFile.write(text.data(), static_cast<std::streamsize>(text.size()));
		outputFile.close();

		generatedCache_.insert({ contentHash, assembly });
		return assembly;
	}
}

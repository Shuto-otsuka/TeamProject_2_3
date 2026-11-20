#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>
#include <FoundationEngine/File/FileLocator.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	String ZephyrModuleAssembler::GetOrCreate(const DynamicArray<String>& moduleNames)
	{
		Uint64 contentHash = 14695981039346656037ull;
		for (const String& name : moduleNames)
		{
			std::string nameString = name.str();
			for (Char character : nameString)
			{
				contentHash ^= static_cast<Uint64>(static_cast<Uint8>(character));
				contentHash *= 1099511628211ull;
			}
			contentHash ^= static_cast<Uint64>(';');
			contentHash *= 1099511628211ull;
		}

		if (auto found = generatedCache_.find(contentHash); found != generatedCache_.end())
		{
			return found->second;
		}

		std::ostringstream includes;
		std::ostringstream spawnCalls;
		std::ostringstream simulateCalls;
		Uint32 row = 0;
		Size moduleIndex = 0;
		for (const String& name : moduleNames)
		{
			const ZephyrModuleDescriptor* descriptor = ZephyrModuleRegistry::Find(name);
			if (!descriptor)
			{
				SC_LOG_ERROR("Zephyrシステム: 未登録のモジュール名です: {}", name.str());
				continue;
			}

			std::string variableName = descriptor->name_.str();
			variableName[0] = static_cast<Char>(std::tolower(static_cast<Uint8>(variableName[0])));
			variableName += "_" + std::to_string(moduleIndex) + "_";
			++moduleIndex;

			includes << "#include <" << descriptor->includePath_.str() << ">\n";

			std::ostringstream assemble;
			assemble << "\t" << descriptor->configStructName_.str() << " " << variableName << ";\n";
			for (const ZephyrModuleField& field : descriptor->fields_)
			{
				assemble << "\t" << variableName << "." << field.name_.str() << " = parameters[base + " << row << "]." << field.swizzle_.str() << ";\n";
				++row;
			}

			if (descriptor->hasSpawn_)
			{
				spawnCalls << assemble.str() << "\tApply" << descriptor->name_.str() << "Spawn(seed, " << variableName << ", emitter);\n";
			}
			if (descriptor->hasSimulate_)
			{
				simulateCalls << assemble.str() << "\tApply" << descriptor->name_.str() << "Simulate(seed, " << variableName << ", emitter);\n";
			}
		}

		std::filesystem::path templatePath = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr/ParticleTemplate.hlsli";
		std::ifstream templateFile(templatePath, std::ios::binary);
		if (!templateFile)
		{
			SC_LOG_ERROR("Zephyrシステム: テンプレートが見つかりません: {}", templatePath.string());
			return String();
		}
		std::string text((std::istreambuf_iterator<Char>(templateFile)), std::istreambuf_iterator<Char>());

		StaticArray<std::pair<std::string, std::string>, 3> markers =
		{
			std::pair<std::string,std::string>{ "// @SC_ZEPHYR_INCLUDE", includes.str() },
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

		std::ostringstream fileName;
		fileName << "Zephyr_" << std::hex << contentHash << ".hlsl";
		std::filesystem::path outputPath = FileLocator::EngineRoot() / "GraphicsEngine/Effect/Zephyr/Generated" / fileName.str();

		std::error_code directoryError;

		std::filesystem::create_directories(outputPath.parent_path(), directoryError);

		std::ofstream outputFile(outputPath, std::ios::binary);
		if (!outputFile)
		{
			SC_LOG_ERROR("Zephyrシステム: 生成ファイルを書き出せません: {}", outputPath.string());
			return String();
		}
		outputFile.write(text.data(), static_cast<std::streamsize>(text.size()));
		outputFile.close();

		String filePath = String(outputPath.string());
		generatedCache_.insert({ contentHash, filePath });
		return filePath;
	}
}

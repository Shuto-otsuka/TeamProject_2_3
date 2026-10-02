#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	ZephyrGeneratedShader ZephyrModuleAssembler::GetOrCreate(const DynamicArray<String>& moduleNames, ShaderHotReload& shaderHotReload)
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

		DynamicArray<const ZephyrModuleDescriptor*> resolvedModules;
		DynamicArray<String> fieldNames;
		for (const String& name : moduleNames)
		{
			const ZephyrModuleDescriptor* descriptor = ZephyrModuleRegistry::Find(name);
			if (!descriptor)
			{
				SC_LOG_ERROR("Zephyrシステム: 未登録のモジュール名です: {}", name.str());
				continue;
			}

			Size index = resolvedModules.size();
			std::string configStructName = descriptor->name_.str();
			configStructName[0] = static_cast<Char>(std::tolower(static_cast<Uint8>(configStructName[0])));

			std::ostringstream fieldName;
			fieldName << configStructName << "_" << index << "_";

			resolvedModules.push_back(descriptor);
			fieldNames.push_back(String(fieldName.str()));
		}

		std::ostringstream includes;
		for (const ZephyrModuleDescriptor* descriptor : resolvedModules)
		{
			includes << "#include \"../" << descriptor->includePath_.str() << "\"\n";
		}

		std::ostringstream fields;
		for (Size moduleIndex = 0; moduleIndex < resolvedModules.size(); ++moduleIndex)
		{
			fields << "\t" << resolvedModules[moduleIndex]->configStructName_.str() << " " << fieldNames[moduleIndex].str() << ";\n";
		}

		std::ostringstream spawnCalls;
		for (Size moduleIndex = 0; moduleIndex < resolvedModules.size(); ++moduleIndex)
		{
			if (!resolvedModules[moduleIndex]->hasSpawn_)
			{
				continue;
			}
			spawnCalls << "\tApply" << resolvedModules[moduleIndex]->name_.str() << "Spawn(seed, modules." << fieldNames[moduleIndex].str() << ", particle);\n";
		}

		std::ostringstream updateCalls;
		for (Size moduleIndex = 0; moduleIndex < resolvedModules.size(); ++moduleIndex)
		{
			if (!resolvedModules[moduleIndex]->hasUpdate_)
			{
				continue;
			}
			updateCalls << "\tApply" << resolvedModules[moduleIndex]->name_.str() << "Update(seed, modules." << fieldNames[moduleIndex].str() << ", particle);\n";
		}

		std::ifstream templateFile("../GraphicsEngine/Effect/Zephyr/ParticleTemplate.hlsli", std::ios::binary);
		if (!templateFile)
		{
			SC_LOG_ERROR("Zephyrシステム: テンプレートが見つかりません");
			return ZephyrGeneratedShader{};
		}
		std::string text((std::istreambuf_iterator<Char>(templateFile)), std::istreambuf_iterator<Char>());

		auto replaceMarker = [&text](const std::string& marker, const std::string& content)
			{
				Size position = text.find(marker);
				if (position == std::string::npos)
				{
					SC_LOG_ERROR("Zephyrシステム: 目印が見つかりません: {}", marker);
					return;
				}
				text.replace(position, marker.size(), content);
			};
		replaceMarker("// @SC_ZEPHYR_INCLUDE", includes.str());
		replaceMarker("// @SC_ZEPHYR_MODULE_FIELD", fields.str());
		replaceMarker("// @SC_ZEPHYR_SPAWN_CALL", spawnCalls.str());
		replaceMarker("// @SC_ZEPHYR_UPDATE_CALL", updateCalls.str());

		std::ostringstream fileNameStream;
		fileNameStream << "../GraphicsEngine/Effect/Zephyr/Generated/Zephyr_" << std::hex << contentHash << ".hlsl";
		String filePath = String(fileNameStream.str());

		std::filesystem::path outputPath(filePath.str());
		std::error_code directoryError;
		std::filesystem::create_directories(outputPath.parent_path(), directoryError);

		std::ofstream outputFile(outputPath, std::ios::binary);
		if (outputFile)
		{
			outputFile.write(text.data(), static_cast<std::streamsize>(text.size()));
		}
		outputFile.close();

		ZephyrGeneratedShader generatedShader{};
		generatedShader.generatedFilePath_ = filePath;
		generatedShader.spawnShader_ = shaderHotReload.GetOrCreateComputeShader(filePath, String("SpawnMain"));
		generatedShader.updateShader_ = shaderHotReload.GetOrCreateComputeShader(filePath, String("UpdateMain"));

		generatedCache_.insert({ contentHash, generatedShader });
		return generatedShader;
	}
}

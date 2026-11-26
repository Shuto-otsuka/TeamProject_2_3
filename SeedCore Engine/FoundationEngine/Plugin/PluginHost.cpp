#include <FoundationEngine/Plugin/PluginHost.h>
#include <FoundationEngine/Log/Notice.h>
#include <FoundationEngine/World/World.h>
#include <Windows.h>
#include <algorithm>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/File/FileDirectory.h>

namespace SeedCore
{
	Bool PluginHost::Initialize(World& world)
	{
		pluginDirectory_ = FileDirectory::ExecutableDirectory();
		imguiContext_ = ImGui::GetCurrentContext();

		Load(world);
		return true;
	}

	void PluginHost::Load(World& world)
	{
		if (!std::filesystem::exists(pluginDirectory_))
		{
			SC_LOG_NOTICE("PluginHost: プラグインディレクトリがありません ({})。プラグインは読み込まれません。", pluginDirectory_.string());
			return;
		}

		ScanAndLoad(world);
	}

	void PluginHost::Unload(World& world)
	{
		for (Entry& entry : entries_)
		{
			entry.module_->Unload(world);
		}
		entries_.clear();
	}

	void PluginHost::Tick(World& world)
	{
		Uint64 nowTick = GetTickCount64();

		for (Entry& entry : entries_)
		{
			Uint64 writeTime = PluginModule::GetLastWriteTime(entry.module_->SourcePath());
			if (writeTime == 0 || !entry.module_->SourceChanged())
			{
				continue;
			}

			if (writeTime == entry.pendingWriteTime_)
			{
				if (world.GetService<GameTimer>()->Playing())
				{
					continue;
				}
				if (nowTick - entry.pendingStableSinceTick_ >= stableWindowMilliseconds_)
				{
					entry.module_->Reload(world, imguiContext_);
					entry.pendingWriteTime_ = 0;
					entry.pendingStableSinceTick_ = 0;
				}
			}
			else
			{
				entry.pendingWriteTime_ = writeTime;
				entry.pendingStableSinceTick_ = nowTick;
			}
		}

		if (nowTick - lastRescanTick_ < rescanIntervalMilliseconds_)
		{
			return;
		}
		lastRescanTick_ = nowTick;

		SeedCore::erase_if(entries_, [&world](Entry& entry)
		{
			if (std::filesystem::exists(entry.module_->SourcePath()))
			{
				return false;
			}
			entry.module_->Unload(world);

			entry.module_->Keep();
			return true;
		});

		ScanAndLoad(world);
	}

	PluginModule* PluginHost::Find(const std::filesystem::path& stem)const
	{
		auto found = std::ranges::find(entries_, stem, [](const Entry& entry){ return entry.module_->SourcePath().stem(); });
		return found != entries_.end() ? found->module_.get() : nullptr;
	}

	void PluginHost::ReloadModule(World& world, PluginModule& module)
	{
		module.Reload(world, imguiContext_);

		auto found = std::ranges::find(entries_, &module, [](const Entry& entry){ return entry.module_.get(); });
		if (found != entries_.end())
		{
			found->pendingWriteTime_ = 0;
			found->pendingStableSinceTick_ = 0;
		}
	}

	void PluginHost::ScanAndLoad(World& world)
	{
		auto isShadowCopy = [](const std::filesystem::path& path) -> Bool
		{
			std::wstring stem = path.stem().wstring();
			Size underscore = stem.rfind(L'_');
			if (underscore == std::wstring::npos || underscore + 1 >= stem.size())
			{
				return false;
			}
			std::wstring_view digits = std::wstring_view(stem).substr(underscore + 1);
			return std::ranges::all_of(digits, [](wchar_t character){ return character >= L'0' && character <= L'9'; });
		};

		auto isPluginCandidate = [](const std::filesystem::path& path) -> Bool
		{
			if (GetModuleHandleW(path.filename().c_str()))
			{
				return false;
			}
			HMODULE probe = LoadLibraryExW(path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
			if (!probe)
			{
				return false;
			}
			Bool hasEntryPoints = GetProcAddress(probe, "SC_OnGameLoad") != nullptr && GetProcAddress(probe, "SC_OnGameUnload") != nullptr;
			FreeLibrary(probe);
			return hasEntryPoints;
		};

		std::error_code errorCode;
		std::filesystem::directory_iterator iterator(pluginDirectory_, errorCode);
		if (errorCode)
		{
			return;
		}

		for (const auto& entry : iterator)
		{
			std::filesystem::path path = entry.path();
			if (path.extension() != L".dll" || isShadowCopy(path))
			{
				continue;
			}

			if (Find(path.stem()) != nullptr)
			{
				continue;
			}

			String fileName = String(path.filename().string());
			if (std::ranges::contains(rejectedFileNames_, fileName))
			{
				continue;
			}

			if (!isPluginCandidate(path))
			{
				rejectedFileNames_.push_back(fileName);
				continue;
			}

			ResourcePtr<PluginModule> module = MakePtr<PluginModule>(path);
			if (!module->Load(world, imguiContext_))
			{
				continue;
			}

			Entry loaded;
			loaded.module_ = std::move(module);
			entries_.push_back(std::move(loaded));
		}
	}
}

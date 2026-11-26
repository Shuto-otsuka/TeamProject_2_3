#include <Editor/Editor/ImGui/ImGuiTexture.h>
#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/Context/EditorContext.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12TransferLane.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	ImGuiTexture::ImGuiTexture(EditorContext& context)
	{
		D3D12Context& d3d12Context = context.graphics_.graphics_->Device().Context();
		BindlessHeap* bindlessHeap = &context.graphics_.graphics_->Device().ResourceHeap();

		D3D12TransferLane* transferLane = d3d12Context.GetLoadTransferLane(LoadThread::Main);
		transferLane->Begin(d3d12Context.GetDevice(), *d3d12Context.GetCopyQueue());

		std::filesystem::path iconRoot("Icon");
		std::error_code error;
		for (auto it = std::filesystem::recursive_directory_iterator(iconRoot, error); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (!it->is_regular_file() || it->path().extension() != ".icon")
			{
				continue;
			}

			std::filesystem::path relative = std::filesystem::relative(it->path(), iconRoot);
			relative.replace_extension();
			String name = String(relative.generic_string());

			Uint index = bindlessHeap->AllocateIndex();
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			String filePath = String(it->path().generic_string());
			TextureCreator::CreateTexturePath(d3d12Context.GetDevice(), *transferLane, bindlessHeap->Heap(), filePath, resource, index);

			icons_.insert({ name, static_cast<Uint64>(bindlessHeap->GPUHandle(index).ptr) });
			resources_.push_back(std::move(resource));
		}

		d3d12Context.GetCopyQueue()->WaitFor(transferLane->Submit(*d3d12Context.GetCopyQueue()));

		if (icons_.contains(String("Misc/Missing")))
		{
			missing_ = icons_.at(String("Misc/Missing"));
		}
	}

	Uint64 ImGuiTexture::Icon(const String& name)const
	{
		auto found = icons_.find(name);
		if (found != icons_.end())
		{
			return found->second;
		}

		if (std::ranges::find(reported_, name) == reported_.end())
		{
			reported_.push_back(name);
			SC_LOG_WARNING("ImGuiTexture: アイコンが見つかりません: Icon/{}.icon", name.str());
		}
		return missing_;
	}

	Uint64 ImGuiTexture::Icon(const String& name, const String& fallback)const
	{
		auto found = icons_.find(name);
		if (found != icons_.end())
		{
			return found->second;
		}
		return Icon(fallback);
	}
}

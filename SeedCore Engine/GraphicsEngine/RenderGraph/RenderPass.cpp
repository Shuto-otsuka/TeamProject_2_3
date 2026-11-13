#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	RenderPass::RenderPass(RenderGraph& graph, D3D12CommandList& list, Uint32 pass, const RenderView* view) :graph_(graph), cmdList_(list), pass_(pass), view_(view)
	{
		/// No Code
	}

	D3D12CommandList& RenderPass::List()const
	{
		return cmdList_;
	}

	RenderDevice& RenderPass::Device()const
	{
		return graph_.device_;
	}

	const RenderView* RenderPass::View()const
	{
		return view_;
	}

	Uint32 RenderPass::Width()const
	{
		return view_ ? view_->renderWidth_ : 0;
	}

	Uint32 RenderPass::Height()const
	{
		return view_ ? view_->renderHeight_ : 0;
	}

	Uint32 RenderPass::Iteration()const
	{
		return graph_.stages_[graph_.passes_[pass_].stage_].iteration_;
	}

	ID3D12Resource* RenderPass::Resource(const RenderResource& resource)const
	{
		const RenderGraphPass& pass = graph_.passes_[pass_];
		const RenderGraphStage& stage = graph_.stages_[pass.stage_];
		const RenderGraphAccess* found = nullptr;
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.index_ == SC_INVALID || std::string_view(access.resourceName_) != resource.Name())
			{
				continue;
			}
			if (!found || found->type_ == RenderAccessType::History)
			{
				found = &access;
			}
		}
		if (found)
		{
			Uint32 view = graph_.resources_[found->index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
			const RenderGraphInstance& instance = graph_.instances_[found->index_ * RenderViewCount + view];
			const RenderGraphAllocation* memory = found->type_ == RenderAccessType::History && instance.history_ != SC_INVALID ? &graph_.persistent_[instance.history_] : instance.memory_;
			return memory ? memory->resource_.Get() : nullptr;
		}
		SC_LOG_ERROR("RenderPass: {} は {} を宣言していません", stage.stageName_.c_str(), resource.Name());
		return nullptr;
	}

	void RenderPass::ClearDepth(const RenderResource& resource)
	{
		const RenderGraphPass& pass = graph_.passes_[pass_];
		const RenderGraphStage& stage = graph_.stages_[pass.stage_];
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.index_ == SC_INVALID || access.type_ != RenderAccessType::DepthWrite || std::string_view(access.resourceName_) != resource.Name())
			{
				continue;
			}
			Uint32 view = graph_.resources_[access.index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
			const RenderGraphAllocation* memory = graph_.instances_[access.index_ * RenderViewCount + view].memory_;
			if (!memory)
			{
				return;
			}
			cmdList_.Get()->ClearDepthStencilView(graph_.device_.DepthStencilHeap().CPUHandle(memory->depthStencilView_), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
			return;
		}
		SC_LOG_ERROR("RenderPass: {} は {} を DepthWrite で宣言していません", stage.stageName_.c_str(), resource.Name());
	}

	void RenderPass::ClearTarget(const RenderResource& resource)
	{
		ClearTarget(resource, { 0.0f, 0.0f, 0.0f, 0.0f });
	}

	void RenderPass::ClearTarget(const RenderResource& resource, const StaticArray<Float, 4>& color)
	{
		const RenderGraphPass& pass = graph_.passes_[pass_];
		const RenderGraphStage& stage = graph_.stages_[pass.stage_];
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.index_ == SC_INVALID || access.type_ != RenderAccessType::Target || std::string_view(access.resourceName_) != resource.Name())
			{
				continue;
			}
			Uint32 view = graph_.resources_[access.index_].scope_ == RenderResourceScope::View && pass.view_ != SC_INVALID ? pass.view_ : 0;
			const RenderGraphAllocation* memory = graph_.instances_[access.index_ * RenderViewCount + view].memory_;
			if (!memory)
			{
				return;
			}
			cmdList_.Get()->ClearRenderTargetView(graph_.device_.RenderTargetHeap().CPUHandle(memory->renderTargetView_), color.data(), 0, nullptr);
			return;
		}
		SC_LOG_ERROR("RenderPass: {} は {} を Target で宣言していません", stage.stageName_.c_str(), resource.Name());
	}

#if SC_DEBUG
	void RenderPass::Verify(std::span<IDxcBlob* const> reflections)
	{
		const RenderGraphStage& stage = graph_.stages_[graph_.passes_[pass_].stage_];
		IDxcBlob* key = nullptr;
		for (IDxcBlob* reflection : reflections)
		{
			if (reflection && !key)
			{
				key = reflection;
			}
		}
		for (const std::pair<String, IDxcBlob*>& verified : graph_.verified_)
		{
			if (verified.first == stage.stageName_ && verified.second == key)
			{
				return;
			}
		}
		graph_.verified_.push_back({ stage.stageName_, key });

		Microsoft::WRL::ComPtr<IDxcUtils> utils;
		if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))))
		{
			return;
		}

		ID3D12ShaderReflectionConstantBuffer* bindings = nullptr;
		DynamicArray<Microsoft::WRL::ComPtr<IUnknown>> keepAlive;
		for (IDxcBlob* reflection : reflections)
		{
			if (!reflection || bindings)
			{
				continue;
			}
			DxcBuffer buffer{};
			buffer.Ptr = reflection->GetBufferPointer();
			buffer.Size = reflection->GetBufferSize();

			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection;
			if (SUCCEEDED(utils->CreateReflection(&buffer, IID_PPV_ARGS(&shaderReflection))))
			{
				keepAlive.push_back(shaderReflection);
				D3D12_SHADER_DESC shaderDesc{};
				shaderReflection->GetDesc(&shaderDesc);
				for (Uint32 index = 0;index < shaderDesc.BoundResources;++index)
				{
					D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
					shaderReflection->GetResourceBindingDesc(index, &bindDesc);
					if (bindDesc.Type == D3D_SIT_CBUFFER && bindDesc.BindPoint == 0 && bindDesc.Space == 1)
					{
						bindings = shaderReflection->GetConstantBufferByName(bindDesc.Name);
					}
				}
				continue;
			}

			Microsoft::WRL::ComPtr<ID3D12LibraryReflection> libraryReflection;
			if (SUCCEEDED(utils->CreateReflection(&buffer, IID_PPV_ARGS(&libraryReflection))))
			{
				keepAlive.push_back(libraryReflection);
				D3D12_LIBRARY_DESC libraryDesc{};
				libraryReflection->GetDesc(&libraryDesc);
				for (Uint32 functionIndex = 0;functionIndex < libraryDesc.FunctionCount && !bindings;++functionIndex)
				{
					ID3D12FunctionReflection* function = libraryReflection->GetFunctionByIndex(static_cast<Int>(functionIndex));
					D3D12_FUNCTION_DESC functionDesc{};
					function->GetDesc(&functionDesc);
					for (Uint32 index = 0;index < functionDesc.BoundResources;++index)
					{
						D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
						function->GetResourceBindingDesc(index, &bindDesc);
						if (bindDesc.Type == D3D_SIT_CBUFFER && bindDesc.BindPoint == 0 && bindDesc.Space == 1)
						{
							bindings = function->GetConstantBufferByName(bindDesc.Name);
						}
					}
				}
			}
		}

		Size expectedCount = 0;
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.type_ != RenderAccessType::Target && access.type_ != RenderAccessType::DepthWrite && access.type_ != RenderAccessType::DepthTest)
			{
				++expectedCount;
			}
		}
		if (!bindings)
		{
			if (expectedCount > 0)
			{
				SC_LOG_WARNING("RenderPass: {} のシェーダーが b0 のバインディングを使っていません", stage.stageName_.c_str());
			}
			return;
		}

		ID3D12ShaderReflectionType* type = bindings->GetVariableByIndex(0)->GetType();
		D3D12_SHADER_TYPE_DESC typeDesc{};
		type->GetDesc(&typeDesc);

		Uint32 memberIndex = 0;
		for (const RenderGraphAccess& access : stage.accesses_)
		{
			if (access.type_ == RenderAccessType::Target || access.type_ == RenderAccessType::DepthWrite || access.type_ == RenderAccessType::DepthTest)
			{
				continue;
			}
			std::string expected;
			std::string_view name = std::string_view(access.bindingName_);
			for (Size index = 0;index < name.size();++index)
			{
				if (name[index] == '.')
				{
					expected += '_';
					continue;
				}
				Bool upper = std::isupper(static_cast<Uchar>(name[index])) != 0;
				Bool previousUpper = index > 0 && std::isupper(static_cast<Uchar>(name[index - 1])) != 0;
				Bool nextLower = index + 1 < name.size() && std::islower(static_cast<Uchar>(name[index + 1])) != 0;
				if (upper && index > 0 && name[index - 1] != '.' && (!previousUpper || nextLower))
				{
					expected += '_';
				}
				expected += static_cast<Char>(std::tolower(static_cast<Uchar>(name[index])));
			}
			if (access.type_ == RenderAccessType::History)
			{
				expected += "_history";
			}
			expected += '_';

			const Char* actual = memberIndex < typeDesc.Members ? type->GetMemberTypeName(memberIndex) : "（なし）";
			if (expected != actual)
			{
				SC_LOG_ERROR("RenderPass: {} のバインディングの {} 番目が合いません（C++ は {}、シェーダーは {}）", stage.stageName_.c_str(), memberIndex, expected, actual);
			}
			++memberIndex;
		}
		if (memberIndex < typeDesc.Members)
		{
			SC_LOG_ERROR("RenderPass: {} のシェーダーのバインディングが {} 個多いです", stage.stageName_.c_str(), typeDesc.Members - memberIndex);
		}
	}
#endif
}

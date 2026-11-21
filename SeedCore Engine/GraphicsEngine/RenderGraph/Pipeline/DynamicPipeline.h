#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/PipelineDescriptionAccess.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/AmplificationShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/PipelineState/ComputeShader.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	enum class DynamicPipelineKind :Uint8
	{
		Compute,
		Graphics,
	};

	struct DynamicComputeDescription
	{
		String file_;

		String entryPoint_ = String("main");

		Uint32 threadGroupSizeX_ = 1;

		Uint32 threadGroupSizeY_ = 1;

		Uint32 threadGroupSizeZ_ = 1;

		const RenderResource* arguments_ = nullptr;
	};

	struct DynamicGraphicsDescription
	{
		String amplificationFile_;

		String meshFile_;

		String vertexFile_;

		String pixelFile_;

		String pixelEntryPoint_ = String("main");

		GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		Uint32 instancePerGroup_ = 1;

		Uint32 fallbackVertexCount_ = 3;

		DynamicArray<RenderFormat> targetFormat_;

		RenderFormat depthFormat_ = RenderFormat::Unknown;

		BlendStateType blend_ = BlendStateType::Opaque;

		DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		RasterizerStateType rasterizer_ = RasterizerStateType::SolidBackLHS;

		const RenderResource* arguments_ = nullptr;

		const RenderResource* fallbackArguments_ = nullptr;
	};

	template<DynamicPipelineKind Kind>
	class DynamicPipeline
	{
	public:
		using Description = std::conditional_t<Kind == DynamicPipelineKind::Compute, DynamicComputeDescription, DynamicGraphicsDescription>;

	public:
		Bool Create(RenderDevice& device, const Description& description)
		{
			description_ = description;
			pipelineState_ = Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>>::null();
#if SC_DEBUG
			reflections_ = { nullptr, nullptr, nullptr };
#endif

			SeedCore::ShaderHotReload& shaderHotReload = device.ShaderHotReload();
			PipelineStateKey key{};
			key.rootSignature_ = device.RootSignature().Signature();

			if constexpr (Kind == DynamicPipelineKind::Compute)
			{
				ComputeShader* computeShader = shaderHotReload.GetComputeShader(shaderHotReload.GetOrCreateComputeShader(description.file_, description.entryPoint_));
				if (!computeShader)
				{
					SC_LOG_ERROR("DynamicPipeline: コンピュートシェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(description.file_));
					return false;
				}
				key.computeShader_ = computeShader->Bytecode();
#if SC_DEBUG
				reflections_[0] = computeShader->ReflectionBlob();
#endif
			}
			else
			{
				if (description.fallback_ == GraphicsFallback::Indirect && (!description.arguments_ || !description.fallbackArguments_))
				{
					SC_LOG_ERROR("DynamicPipeline: Indirect の記述には arguments_ と fallbackArguments_ が要ります");
					return false;
				}

				mesh_ = device.Level() == D3D12Level::D12_2;
				key.rasterizerDesc_ = RasterizerState::Get(description.rasterizer_);
				key.blendDesc_ = BlendState::Get(description.blend_);
				key.depthStencilDesc_ = DepthStencilState::Get(description.depthStencil_);
				key.renderTargetViewCount_ = static_cast<Uint32>(description.targetFormat_.size());
				for (Size index = 0;index < description.targetFormat_.size();++index)
				{
					key.renderTargetViewFormat_[index] = static_cast<DXGI_FORMAT>(description.targetFormat_[index]);
				}
				key.depthStencilViewFormat_ = static_cast<DXGI_FORMAT>(description.depthFormat_);
				key.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

				if (!description.pixelFile_.str().empty())
				{
					PixelShader* pixelShader = shaderHotReload.GetPixelShader(shaderHotReload.GetOrCreatePixelShader(description.pixelFile_, description.pixelEntryPoint_));
					if (!pixelShader)
					{
						SC_LOG_ERROR("DynamicPipeline: ピクセルシェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(description.pixelFile_));
						return false;
					}
					key.pixelShader_ = pixelShader->Bytecode();
#if SC_DEBUG
					reflections_[2] = pixelShader->ReflectionBlob();
#endif
				}

				if (mesh_)
				{
					if (!description.amplificationFile_.str().empty())
					{
						AmplificationShader* amplificationShader = shaderHotReload.GetAmplificationShader(shaderHotReload.GetOrCreateAmplificationShader(description.amplificationFile_));
						if (!amplificationShader)
						{
							SC_LOG_ERROR("DynamicPipeline: 増幅シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(description.amplificationFile_));
							return false;
						}
						key.amplificationShader_ = amplificationShader->Bytecode();
#if SC_DEBUG
						reflections_[0] = amplificationShader->ReflectionBlob();
#endif
					}
					MeshShader* meshShader = shaderHotReload.GetMeshShader(shaderHotReload.GetOrCreateMeshShader(description.meshFile_));
					if (!meshShader)
					{
						SC_LOG_ERROR("DynamicPipeline: メッシュシェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(description.meshFile_));
						return false;
					}
					key.meshShader_ = meshShader->Bytecode();
#if SC_DEBUG
					reflections_[1] = meshShader->ReflectionBlob();
#endif
				}
				else
				{
					VertexShader* vertexShader = shaderHotReload.GetVertexShader(shaderHotReload.GetOrCreateVertexShader(description.vertexFile_));
					if (!vertexShader)
					{
						SC_LOG_ERROR("DynamicPipeline: 頂点シェーダーのコンパイルに失敗しました: {}", shaderHotReload.GetErrorMessage(description.vertexFile_));
						return false;
					}
					key.vertexShader_ = vertexShader->Bytecode();
#if SC_DEBUG
					reflections_[1] = vertexShader->ReflectionBlob();
#endif
				}
			}

			pipelineState_ = device.PipelineState().GetOrCreate(device.Context().GetDevice(), key);
			return true;
		}

		void Dispatch(RenderPass& pass, Uint32 width, Uint32 height = 1, Uint32 depth = 1, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Kind == DynamicPipelineKind::Compute, "Dispatch はコンピュートの DynamicPipeline で使うこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			if (!pipelineState)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(std::span<IDxcBlob* const>(&reflections_[0], 1));
#endif
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState(pipelineState);
			cmdList->SetComputeRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);
			cmdList->Dispatch((width + description_.threadGroupSizeX_ - 1) / description_.threadGroupSizeX_, (height + description_.threadGroupSizeY_ - 1) / description_.threadGroupSizeY_, (depth + description_.threadGroupSizeZ_ - 1) / description_.threadGroupSizeZ_);
		}

		void DispatchIndirect(RenderPass& pass, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Kind == DynamicPipelineKind::Compute, "DispatchIndirect はコンピュートの DynamicPipeline で使うこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			ID3D12Resource* arguments = description_.arguments_ ? pass.Resource(*description_.arguments_) : nullptr;
			if (!pipelineState || !arguments)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(std::span<IDxcBlob* const>(&reflections_[0], 1));
#endif
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState(pipelineState);
			cmdList->SetComputeRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);
			cmdList->ExecuteIndirect(pass.Device().DispatchSignature().Signature(), 1, arguments, 0, nullptr, 0);
		}

		void Draw(RenderPass& pass, Uint32 instanceCount = 1, D3D12_GPU_VIRTUAL_ADDRESS bindings = 0, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Kind == DynamicPipelineKind::Graphics, "Draw は描画の DynamicPipeline で使うこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			if (!pipelineState || description_.fallback_ == GraphicsFallback::Indirect)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(reflections_);
#endif
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState(pipelineState);
			if (bindings != 0)
			{
				cmdList->SetGraphicsRootConstantBufferView(static_cast<Uint32>(RootParameter::Stage), bindings);
			}
			cmdList->SetGraphicsRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);
			if (description_.fallback_ == GraphicsFallback::FullscreenTriangle)
			{
				if (mesh_)
				{
					cmdList->DispatchMesh(1, 1, 1);
				}
				else
				{
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(3, 1, 0, 0);
				}
			}
			else
			{
				if (mesh_)
				{
					cmdList->DispatchMesh((instanceCount + description_.instancePerGroup_ - 1) / description_.instancePerGroup_, 1, 1);
				}
				else
				{
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(description_.fallbackVertexCount_, instanceCount, 0, 0);
				}
			}
			ProfilerStats::AddDrawCall();
		}

		void DrawIndirect(RenderPass& pass, Uint32 maxCount, Uint32 first = 0, D3D12_GPU_VIRTUAL_ADDRESS bindings = 0, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Kind == DynamicPipelineKind::Graphics, "DrawIndirect は描画の DynamicPipeline で使うこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			const RenderResource* argumentResource = mesh_ ? description_.arguments_ : description_.fallbackArguments_;
			ID3D12Resource* arguments = argumentResource ? pass.Resource(*argumentResource) : nullptr;
			if (!pipelineState || !arguments)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(reflections_);
#endif
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState(pipelineState);
			if (bindings != 0)
			{
				cmdList->SetGraphicsRootConstantBufferView(static_cast<Uint32>(RootParameter::Stage), bindings);
			}
			cmdList->SetGraphicsRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);
			if (!mesh_)
			{
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
			const CommandSignature& signature = mesh_ ? pass.Device().MeshSignature() : pass.Device().VertexSignature();
			cmdList->ExecuteIndirect(signature.Signature(), maxCount, arguments, first * signature.Stride(), nullptr, 0);
			ProfilerStats::AddDrawCall();
		}

	private:
		Description description_;

		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineState_;

		Bool mesh_ = false;

#if SC_DEBUG
		StaticArray<IDxcBlob*, 3> reflections_ = { nullptr, nullptr, nullptr };
#endif
	};
}

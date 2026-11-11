#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/PipelineDescriptionAccess.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/AmplificationShader.h>
#include <GraphicsEngine/D3D12/PipelineState/MeshShader.h>
#include <GraphicsEngine/D3D12/PipelineState/VertexShader.h>
#include <GraphicsEngine/D3D12/PipelineState/PixelShader.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	template<class Description>
	class GraphicsPipeline
	{
	private:
		SC_STATIC_ASSERT(PipelineDescriptionAccess::IsGraphicsDescription<Description>, "Graphics の記述は meshFile_ / vertexFile_ / fallback_ / targetFormat_ / depthFormat_ / blend_ / depthStencil_ / rasterizer_ を持つこと");

		SC_STATIC_ASSERT(Description::fallback_ != GraphicsFallback::Instanced || PipelineDescriptionAccess::IsInstancedDescription<Description>, "Instanced の記述は instancesPerGroup_ / fallbackVertexCount_ も持つこと");

		SC_STATIC_ASSERT(Description::fallback_ != GraphicsFallback::Indirect || PipelineDescriptionAccess::IsIndirectDescription<Description>, "Indirect の記述は arguments_ / fallbackArguments_ も持つこと");

	public:
		void Create(RenderDevice& device)
		{
			SeedCore::ShaderCache& shaderCache = device.ShaderCache();
			mesh_ = device.Level() == D3D12Level::D12_2;

			PipelineStateKey key{};
			key.rootSignature_ = device.RootSignature().Signature();
			key.rasterizerDesc_ = RasterizerState::Get(Description::rasterizer_);
			key.blendDesc_ = BlendState::Get(Description::blend_);
			key.depthStencilDesc_ = DepthStencilState::Get(Description::depthStencil_);
			key.renderTargetViewCount_ = static_cast<Uint32>(Description::targetFormat_.size());
			for (Size index = 0;index < Description::targetFormat_.size();++index)
			{
				key.renderTargetViewFormat_[index] = Description::targetFormat_[index];
			}
			key.depthStencilViewFormat_ = Description::depthFormat_;
			key.primitiveTopologyType_ = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

			if constexpr (requires { Description::pixelFile_; })
			{
				String pixelEntryPoint = String("main");
				if constexpr (requires { Description::pixelEntryPoint_; })
				{
					pixelEntryPoint = String(Description::pixelEntryPoint_);
				}
				PixelShader* pixelShader = shaderCache.GetPixelShader(shaderCache.GetOrCreatePixelShader(Description::pixelFile_, pixelEntryPoint));
				if (!pixelShader)
				{
					SC_LOG_ERROR("GraphicsPipeline: ピクセルシェーダーのコンパイルに失敗しました");
					return;
				}
				key.pixelShader_ = pixelShader->Bytecode();
#if SC_DEBUG
				reflections_[2] = pixelShader->ReflectionBlob();
#endif
			}

			if (mesh_)
			{
				if constexpr (requires { Description::amplificationFile_; })
				{
					AmplificationShader* amplificationShader = shaderCache.GetAmplificationShader(shaderCache.GetOrCreateAmplificationShader(Description::amplificationFile_));
					if (!amplificationShader)
					{
						SC_LOG_ERROR("GraphicsPipeline: 増幅シェーダーのコンパイルに失敗しました");
						return;
					}
					key.amplificationShader_ = amplificationShader->Bytecode();
#if SC_DEBUG
					reflections_[0] = amplificationShader->ReflectionBlob();
#endif
				}
				MeshShader* meshShader = shaderCache.GetMeshShader(shaderCache.GetOrCreateMeshShader(Description::meshFile_));
				if (!meshShader)
				{
					SC_LOG_ERROR("GraphicsPipeline: メッシュシェーダーのコンパイルに失敗しました");
					return;
				}
				key.meshShader_ = meshShader->Bytecode();
#if SC_DEBUG
				reflections_[1] = meshShader->ReflectionBlob();
#endif
			}
			else
			{
				VertexShader* vertexShader = shaderCache.GetVertexShader(shaderCache.GetOrCreateVertexShader(Description::vertexFile_));
				if (!vertexShader)
				{
					SC_LOG_ERROR("GraphicsPipeline: 頂点シェーダーのコンパイルに失敗しました");
					return;
				}
				key.vertexShader_ = vertexShader->Bytecode();
#if SC_DEBUG
				reflections_[1] = vertexShader->ReflectionBlob();
#endif
			}

			pipelineState_ = device.PipelineState().GetOrCreate(device.Context().GetDevice(), key);
		}

		void Draw(RenderPass& pass, Uint32 instanceCount = 1, D3D12_GPU_VIRTUAL_ADDRESS bindings = 0, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Description::fallback_ != GraphicsFallback::Indirect, "Indirect の記述は DrawIndirect で描くこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			if (!pipelineState)
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
			if constexpr (Description::fallback_ == GraphicsFallback::FullscreenTriangle)
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
					cmdList->DispatchMesh((instanceCount + Description::instancesPerGroup_ - 1) / Description::instancesPerGroup_, 1, 1);
				}
				else
				{
					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					cmdList->DrawInstanced(Description::fallbackVertexCount_, instanceCount, 0, 0);
				}
			}
		}

		void DrawIndirect(RenderPass& pass, Uint32 maxCount, Uint32 first = 0, Uint32 countIndex = 0, D3D12_GPU_VIRTUAL_ADDRESS bindings = 0, Uint32 constant = 0)
		{
			SC_STATIC_ASSERT(Description::fallback_ == GraphicsFallback::Indirect, "DrawIndirect は Indirect の記述で使うこと");
			ID3D12PipelineState* pipelineState = pass.Device().PipelineState().Get(pipelineState_);
			ID3D12Resource* arguments = pass.Resource(mesh_ ? Description::arguments_ : Description::fallbackArguments_);
			if (!pipelineState || !arguments)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(reflections_);
#endif
			ID3D12Resource* counts = nullptr;
			if constexpr (requires{ Description::counts_; })
			{
				counts = pass.Resource(Description::counts_);
			}
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
			cmdList->ExecuteIndirect(signature.Signature(), maxCount, arguments, first * signature.Stride(), counts, countIndex * sizeof(Uint32));
		}

	private:
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineState_;

		Bool mesh_ = false;

#if SC_DEBUG
		StaticArray<IDxcBlob*, 3> reflections_ = { nullptr, nullptr, nullptr };
#endif
	};
}

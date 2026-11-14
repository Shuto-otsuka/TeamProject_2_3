#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/PipelineDescriptionAccess.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/D3D12/PipelineState/RaytracingStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/RaytracingShader.h>
#include <GraphicsEngine/D3D12/Buffer/StagingBuffer.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	template<class Description>
	class RaytracingPipeline
	{
	private:
		SC_STATIC_ASSERT(PipelineDescriptionAccess::IsRaytracingDescription<Description>, "Raytracing の記述は file_ / rayGeneration_ / miss_ / closestHit_ / hitGroup_ / payloadSize_ / recursionDepth_ を持つこと");

	public:
		void Create(RenderDevice& device)
		{
			SeedCore::ShaderCache& shaderCache = device.ShaderCache();
			RaytracingShader* library = shaderCache.GetRaytracingShader(shaderCache.GetOrCreateRaytracingShader(Description::file_));
			if (!library)
			{
				SC_LOG_ERROR("RaytracingPipeline: シェーダーのコンパイルに失敗しました");
				return;
			}
#if SC_DEBUG
			reflection_ = library->ReflectionBlob();
#endif

			RaytracingStateKey key{};
			key.globalRootSignature_ = device.RootSignature().Signature();
			key.localRootSignature_ = nullptr;
			key.libraryShader_ = library->Bytecode();
			key.rayGenExportName_ = String(Description::rayGeneration_);
			key.missExportName_ = String(Description::miss_);
			key.closestHitExportName_ = String(Description::closestHit_);
			if constexpr (requires{ Description::anyHit_; })
			{
				key.anyHitExportName_ = String(Description::anyHit_);
			}
			key.hitGroupName_ = String(Description::hitGroup_);
			key.maxTraceRecursionDepth_ = Description::recursionDepth_;
			key.maxPayloadSizeInBytes_ = Description::payloadSize_;
			if constexpr (requires{ Description::attributeSize_; })
			{
				key.maxAttributeSizeInBytes_ = Description::attributeSize_;
			}

			Microsoft::WRL::ComPtr<ID3D12Device5> raytracingDevice;
			if (FAILED(device.Context().GetDevice()->QueryInterface(IID_PPV_ARGS(&raytracingDevice))))
			{
				SC_LOG_ERROR("RaytracingPipeline: ID3D12Device5 を取得できません");
				return;
			}
			stateObject_ = device.RaytracingState().GetOrCreate(raytracingDevice.Get(), key);
			ID3D12StateObject* stateObject = device.RaytracingState().Get(stateObject_);

			Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> properties;
			if (!stateObject || FAILED(stateObject->QueryInterface(IID_PPV_ARGS(&properties))))
			{
				SC_LOG_ERROR("RaytracingPipeline: ステートオブジェクトの生成に失敗しました");
				return;
			}

			StaticArray<void*, 3> identifiers = { properties->GetShaderIdentifier(key.rayGenExportName_.w_str().c_str()), properties->GetShaderIdentifier(key.missExportName_.w_str().c_str()), properties->GetShaderIdentifier(key.hitGroupName_.w_str().c_str()) };
			table_ = MakePtr<StagingBuffer>(device.Context().GetDevice(), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * identifiers.size(), D3D12_HEAP_TYPE_UPLOAD);
			for (Size index = 0;index < identifiers.size();++index)
			{
				if (!identifiers[index])
				{
					SC_LOG_ERROR("RaytracingPipeline: エクスポートの名前が見つかりません");
					table_.reset();
					return;
				}
				table_->Write(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * index, std::span<const std::byte>(static_cast<const std::byte*>(identifiers[index]), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES));
			}
		}

		void DispatchRays(RenderPass& pass, Uint32 width, Uint32 height, Uint32 depth = 1, Uint32 constant = 0)
		{
			ID3D12StateObject* stateObject = pass.Device().RaytracingState().Get(stateObject_);
			if (!stateObject || !table_)
			{
				return;
			}
#if SC_DEBUG
			pass.Verify(std::span<IDxcBlob* const>(&reflection_, 1));
#endif
			ID3D12GraphicsCommandList6* cmdList = pass.List().Get();
			cmdList->SetPipelineState1(stateObject);
			cmdList->SetComputeRoot32BitConstant(static_cast<Uint32>(RootParameter::Dispatch), constant, 0);

			D3D12_GPU_VIRTUAL_ADDRESS table = table_->Address();
			D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};
			dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = table;
			dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.MissShaderTable.StartAddress = table + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.MissShaderTable.SizeInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.MissShaderTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.HitGroupTable.StartAddress = table + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * 2;
			dispatchRaysDesc.HitGroupTable.SizeInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.HitGroupTable.StrideInBytes = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
			dispatchRaysDesc.Width = width;
			dispatchRaysDesc.Height = height;
			dispatchRaysDesc.Depth = depth;
			cmdList->DispatchRays(&dispatchRaysDesc);
		}

	private:
		Handle<Microsoft::WRL::ComPtr<ID3D12StateObject>> stateObject_;

		ResourcePtr<StagingBuffer> table_;

#if SC_DEBUG
		IDxcBlob* reflection_ = nullptr;
#endif
	};
}
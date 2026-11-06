#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/SamplerState.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	void RootSignature::Create(ID3D12Device* device)
	{
		D3D12_ROOT_PARAMETER1 parameters[3]{};

		D3D12_ROOT_PARAMETER1& stage = parameters[static_cast<Uint32>(RootParameter::Stage)];
		stage.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		stage.Descriptor.ShaderRegister = 0;
		stage.Descriptor.RegisterSpace = 1;
		stage.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		stage.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER1& view = parameters[static_cast<Uint32>(RootParameter::View)];
		view.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		view.Descriptor.ShaderRegister = 1;
		view.Descriptor.RegisterSpace = 1;
		view.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		view.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_PARAMETER1& dispatch = parameters[static_cast<Uint32>(RootParameter::Dispatch)];
		dispatch.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		dispatch.Constants.ShaderRegister = 2;
		dispatch.Constants.RegisterSpace = 1;
		dispatch.Constants.Num32BitValues = 1;
		dispatch.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		StaticArray<D3D12_STATIC_SAMPLER_DESC, static_cast<Size>(SamplerStateType::Count)> samplers = SamplerState::Create();

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = 3;
		rootSignatureDesc.Desc_1_1.pParameters = parameters;
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<Uint>(samplers.size());
		rootSignatureDesc.Desc_1_1.pStaticSamplers = samplers.data();
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignature;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &serializedRootSignature, &errorBlob);
		if (FAILED(hr) && errorBlob)
		{
			OutputDebugStringA(static_cast<const Char*>(errorBlob->GetBufferPointer()));
		}
		SC_HR_CHECK(hr, "RootSignature のシリアライズに失敗しました");

		hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		SC_HR_CHECK(hr, "RootSignature の生成に失敗しました");
	}

	ID3D12RootSignature* RootSignature::Signature()const
	{
		return rootSignature_.Get();
	}
}
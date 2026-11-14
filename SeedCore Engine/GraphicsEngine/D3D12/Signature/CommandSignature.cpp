#include <GraphicsEngine/D3D12/Signature/CommandSignature.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Bool CommandSignature::Create(ID3D12Device* device, D3D12_INDIRECT_ARGUMENT_TYPE type)
	{
		stride_ = 0;
		if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW)
		{
			stride_ = sizeof(D3D12_DRAW_ARGUMENTS);
		}
		else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED)
		{
			stride_ = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
		}
		else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH)
		{
			stride_ = sizeof(D3D12_DISPATCH_ARGUMENTS);
		}
		else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH)
		{
			stride_ = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
		}
		else if (type == D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS)
		{
			stride_ = sizeof(D3D12_DISPATCH_RAYS_DESC);
		}
		if (stride_ == 0)
		{
			SC_LOG_ERROR("CommandSignature: 描く・Dispatch する引数の種類だけを使えます");
			return false;
		}

		D3D12_INDIRECT_ARGUMENT_DESC argumentDesc{};
		argumentDesc.Type = type;
		D3D12_COMMAND_SIGNATURE_DESC signatureDesc{};
		signatureDesc.ByteStride = stride_;
		signatureDesc.NumArgumentDescs = 1;
		signatureDesc.pArgumentDescs = &argumentDesc;
		return SUCCEEDED(device->CreateCommandSignature(&signatureDesc, nullptr, IID_PPV_ARGS(&signature_)));
	}

	ID3D12CommandSignature* CommandSignature::Signature()const
	{
		return signature_.Get();
	}

	Uint32 CommandSignature::Stride()const
	{
		return stride_;
	}
}
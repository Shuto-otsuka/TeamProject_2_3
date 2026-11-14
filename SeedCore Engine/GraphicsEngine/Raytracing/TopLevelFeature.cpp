#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <FoundationEngine/Log/DxFail.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(TopLevelFeature, UpdateStage, BuildStage);

	void TopLevelFeature::Create(RenderDevice& device)
	{
		updatePipeline_.Create(device);
		resolvePipeline_.Create(device);

		Microsoft::WRL::ComPtr<ID3D12Device5> raytracingDevice;
		HRESULT hr = device.Context().GetDevice()->QueryInterface(IID_PPV_ARGS(&raytracingDevice));
		SC_HR_CHECK(hr, "TopLevel: ID3D12Device5 を取得できません");

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		inputs.NumDescs = capacity_;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
		raytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = Max(prebuildInfo.ScratchDataSizeInBytes, 1ull);
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		hr = raytracingDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&scratch_));
		SC_HR_CHECK(hr, "TopLevel: TLAS のスクラッチの生成に失敗しました");
		scratch_->SetName(L"TopLevel.Scratch");
	}

	void TopLevelFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		count_ = 0;
		for (const TopLevelRequest& request : frame.List<TopLevelRequest>())
		{
			count_ += request.count_;
		}
		count_ = Min(count_, capacity_);
	}
}

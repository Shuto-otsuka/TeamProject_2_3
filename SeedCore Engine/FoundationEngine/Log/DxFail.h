#pragma once
#include <FoundationEngine/Pool/InternPool.h>
#include <FoundationEngine/Log/AftermathCrashTracker.h>

#define SC_HR_CHECK(hr, msg) SeedCore::DxFail(hr, msg, __FILE__, __LINE__)

#define SC_DEVICE_REMOVED_CHECK(hr, device, msg) SeedCore::DeviceRemovedFail(hr, device, msg, __FILE__, __LINE__)

namespace SeedCore
{
	inline void DxFail(HRESULT hr, const std::string& msg, const Char* file, Int line)
	{
		if (SUCCEEDED(hr))
		{
			return;
		}

		_com_error error(hr);

		std::string output = std::format("重要：DirectX 処理が失敗しました。\n\n" "詳細: {}\n" "コード: {:#010x}\n" "内容: {}\n\n" "場所: {}:{}", msg, static_cast<Uint32>(hr), ConvertToCharString(error.ErrorMessage()), std::filesystem::path(file).filename().string(), line);

		std::wstring wideOutput = ConvertToWideString(output);

		MessageBoxW(NULL, wideOutput.c_str(), L"SeedCore Engine - DirectX Error", MB_ICONERROR | MB_OK);

		__debugbreak();
	}

	inline const Char* AutoBreadcrumbOpToString(D3D12_AUTO_BREADCRUMB_OP op)
	{
		switch (op)
		{
		case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:                    return "SetMarker";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:                   return "BeginEvent";
		case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:                     return "EndEvent";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:                return "DrawInstanced";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:         return "DrawIndexedInstanced";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:              return "ExecuteIndirect";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:                     return "Dispatch";
		case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:             return "CopyBufferRegion";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:            return "CopyTextureRegion";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:                 return "CopyResource";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:                    return "CopyTiles";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:           return "ResolveSubresource";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:        return "ClearRenderTargetView";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:     return "ClearUnorderedAccessView";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:        return "ClearDepthStencilView";
		case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:              return "ResourceBarrier";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:                return "ExecuteBundle";
		case D3D12_AUTO_BREADCRUMB_OP_PRESENT:                      return "Present";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:             return "ResolveQueryData";
		case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:              return "BeginSubmission";
		case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:                return "EndSubmission";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:           return "ExecuteMetaCommand";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:         return "AtomicCopyBufferUint";
		case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:       return "AtomicCopyBufferUint64";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:     return "ResolveSubresourceRegion";
		case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:  return "SetProtectedResourceSession";
		case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:                  return "DecodeFrame";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:                 return "DispatchRays";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:        return "InitializeMetaCommand";
		case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:               return "EstimateMotion";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:      return "ResolveMotionVectorHeap";
		case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:            return "SetPipelineState1";
		case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:   return "InitializeExtensionCommand";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:      return "ExecuteExtensionCommand";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:                 return "DispatchMesh";
		case D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME:                  return "EncodeFrame";
		case D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA: return "ResolveEncoderOutputMetadata";
		default:                                                    return "Unknown";
		}
	}

	inline void DeviceRemovedFail(HRESULT hr, ID3D12Device* device, const std::string& msg, const Char* file, Int line)
	{
		if (SUCCEEDED(hr))
		{
			return;
		}

		if (hr != DXGI_ERROR_DEVICE_REMOVED && hr != DXGI_ERROR_DEVICE_HUNG)
		{
			DxFail(hr, msg, file, line);
			return;
		}

		HRESULT removedReason = device ? device->GetDeviceRemovedReason() : E_FAIL;
		_com_error reasonError(removedReason);

		std::string output = std::format("重要：GPUデバイスが削除されました。\n\n" "詳細: {}\n" "Present等のコード: {:#010x}\n" "GetDeviceRemovedReason: {:#010x} ({})\n\n" "場所: {}:{}", msg, static_cast<Uint32>(hr), static_cast<Uint32>(removedReason), ConvertToCharString(reasonError.ErrorMessage()), std::filesystem::path(file).filename().string(), line);

		if (device)
		{
			Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
			HRESULT dredHr = device->QueryInterface(IID_PPV_ARGS(&dred));
			if (FAILED(dredHr))
			{
				output += std::format("\n\nDRED: 取得できません (QueryInterface {:#010x})", static_cast<Uint32>(dredHr));
			}
			else
			{
				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
				HRESULT breadcrumbHr = dred->GetAutoBreadcrumbsOutput1(&breadcrumbs);
				if (FAILED(breadcrumbHr))
				{
					output += std::format("\n\nDRED ブレッドクラム: 取得できません ({:#010x})", static_cast<Uint32>(breadcrumbHr));
				}
				else if (!breadcrumbs.pHeadAutoBreadcrumbNode)
				{
					output += "\n\nDRED ブレッドクラム: 記録がありません(デバイス生成前に有効化されていない可能性)";
				}
				else
				{
					Uint32 nodeCount = 0;
					Uint32 incompleteCount = 0;
					for (const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbs.pHeadAutoBreadcrumbNode; node != nullptr; node = node->pNext)
					{
						nodeCount++;

						Uint32 completedCount = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
						if (!node->pCommandHistory || completedCount >= node->BreadcrumbCount)
						{
							continue;
						}

						incompleteCount++;
						if (incompleteCount > 4)
						{
							continue;
						}

						output += std::format("\n\n[未完了 {}] 実行中断時のGPUコマンド: {} (完了済み {}/{})", incompleteCount, AutoBreadcrumbOpToString(node->pCommandHistory[completedCount]), completedCount, node->BreadcrumbCount);

						Uint32 historyStart = completedCount > 5 ? completedCount - 5 : 0;
						for (Uint32 historyIndex = historyStart; historyIndex < completedCount; historyIndex++)
						{
							output += std::format("\n    直前: {}", AutoBreadcrumbOpToString(node->pCommandHistory[historyIndex]));
						}
					}

					output += std::format("\n\nDRED ノード数: {} (うち未完了 {})", nodeCount, incompleteCount);
				}

				D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFault{};
				HRESULT pageFaultHr = dred->GetPageFaultAllocationOutput1(&pageFault);
				if (FAILED(pageFaultHr))
				{
					output += std::format("\nページフォルト情報: 取得できません ({:#010x})", static_cast<Uint32>(pageFaultHr));
				}
				else if (pageFault.PageFaultVA == 0)
				{
					output += "\nページフォルト: なし";
				}
				else
				{
					output += std::format("\nページフォルトVA: {:#018x}", pageFault.PageFaultVA);
				}
			}
		}

#if SC_DEBUG
		output += AftermathCrashTracker::Report().str();
#endif

		std::wstring wideOutput = ConvertToWideString(output);

		MessageBoxW(NULL, wideOutput.c_str(), L"SeedCore Engine - GPU Device Removed", MB_ICONERROR | MB_OK);

		__debugbreak();
	}
}
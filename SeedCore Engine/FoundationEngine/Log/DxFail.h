#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Pool/InternPool.h>
#include <FoundationEngine/Log/AftermathCrashTracker.h>

/**
* [EN]
* Checks an HRESULT from a DirectX call. On failure, forwards the
* current file/line to DxFail.
*
* ---------------------------------------------------------------------
*
* [JP]
* DirectX 呼び出しの HRESULT を検査する。失敗時は現在のファイル/行を
* DxFail に渡す。
*/
#define SC_HR_CHECK(hr, msg) SeedCore::DxFail(hr, msg, __FILE__, __LINE__)

/**
* [EN]
* Checks an HRESULT from a swap chain Present/command queue call that can
* fail with DXGI_ERROR_DEVICE_REMOVED/DXGI_ERROR_DEVICE_HUNG. On that
* failure, forwards device (to query the real removal reason and DRED
* breadcrumbs/page fault data) plus the current file/line to
* DeviceRemovedFail.
*
* ---------------------------------------------------------------------
*
* [JP]
* DXGI_ERROR_DEVICE_REMOVED/DXGI_ERROR_DEVICE_HUNG で失敗しうる、
* スワップチェーン Present/コマンドキュー呼び出しの HRESULT を検査する。
* 失敗時は device（実際の削除理由と DRED のブレッドクラム/ページフォルト
* 情報を照会するため）と現在のファイル/行を DeviceRemovedFail に渡す。
*/
#define SC_DEVICE_REMOVED_CHECK(hr, device, msg) SeedCore::DeviceRemovedFail(hr, device, msg, __FILE__, __LINE__)

namespace SeedCore
{
	/**
	* [EN]
	* Handles a failed HRESULT: does nothing if hr succeeded, otherwise
	* shows a blocking error message box with the failing HRESULT/message
	* and breaks into the debugger.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 失敗した HRESULT を処理する。hr が成功していれば何もしない。
	* それ以外は失敗した HRESULT/メッセージを含むブロッキングのエラー
	* メッセージボックスを表示し、デバッガへブレークする。
	*/
	inline void DxFail(HRESULT hr, const std::string& msg, const Char* file, Int line)
	{
		if (SUCCEEDED(hr))
		{
			return;
		}

		/// [EN] _com_error turns the HRESULT into the system's own description of the error.
		/// [JP] _com_error で、HRESULT をシステム自身のエラー説明に変える。
		_com_error error(hr);

		/// [EN] Only the file name is shown, since __FILE__ is a full build path.
		/// [JP] __FILE__ はビルド時の完全なパスなので、ファイル名だけを表示する。
		std::string output = std::format("重要：DirectX 処理が失敗しました。\n\n" "詳細: {}\n" "コード: {:#010x}\n" "内容: {}\n\n" "場所: {}:{}", msg, static_cast<Uint32>(hr), ConvertToCharString(error.ErrorMessage()), std::filesystem::path(file).filename().string(), line);

		/// [EN] The message box takes UTF-16, so the Japanese text is converted first.
		/// [JP] メッセージボックスは UTF-16 を受け取るので、先に日本語の文字列を変換する。
		std::wstring wideOutput = ConvertToWideString(output);

		MessageBoxW(NULL, wideOutput.c_str(), L"SeedCore Engine - DirectX Error", MB_ICONERROR | MB_OK);

		/// [EN] Stops in the debugger at the failing call once the message has been read.
		/// [JP] メッセージを読んだ後、失敗した呼び出しの位置でデバッガを止める。
		__debugbreak();
	}

	/**
	* [EN]
	* Best-effort human-readable name for a DRED auto-breadcrumb op, so the
	* device-removed report names the actual GPU command that was in flight
	* (e.g. "BuildRaytracingAccelerationStructure") instead of just an
	* opaque enum number.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* DRED オートブレッドクラムの op を人間可読な名前に変換する
	* (ベストエフォート)。device-removed レポートに、実行中だった実際の
	* GPU コマンド(例: "BuildRaytracingAccelerationStructure")を、
	* 不透明な enum 番号の代わりに出せるようにする。
	*/
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

	/**
	* [EN]
	* Handles a device-removal-capable HRESULT (typically from
	* IDXGISwapChain::Present): does nothing if hr succeeded. On
	* DXGI_ERROR_DEVICE_REMOVED/DXGI_ERROR_DEVICE_HUNG, queries device for
	* the real GetDeviceRemovedReason(), plus (when D3D12DebugLayer::Enable
	* turned DRED on before device creation) the DRED auto-breadcrumb of
	* the last GPU command that was in flight and any page-fault VA, then
	* shows a blocking error message box with all of that and breaks into
	* the debugger. Other failures fall back to DxFail's plain HRESULT
	* report.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイス削除しうる HRESULT（典型的には IDXGISwapChain::Present から）
	* を処理する。hr が成功していれば何もしない。
	* DXGI_ERROR_DEVICE_REMOVED/DXGI_ERROR_DEVICE_HUNG の場合、device から
	* 実際の GetDeviceRemovedReason() と、(D3D12DebugLayer::Enable が
	* デバイス作成前に DRED を有効化していれば) 実行中だった最後の GPU
	* コマンドの DRED オートブレッドクラムおよびページフォルト VA を照会し、
	* それら全てを含むブロッキングのエラーメッセージボックスを表示して
	* デバッガへブレークする。それ以外の失敗は DxFail の通常の HRESULT
	* レポートにフォールバックする。
	*/
	inline void DeviceRemovedFail(HRESULT hr, ID3D12Device* device, const std::string& msg, const Char* file, Int line)
	{
		if (SUCCEEDED(hr))
		{
			return;
		}

		/// [EN] Any other failure has no removal reason or breadcrumbs to collect.
		/// [JP] それ以外の失敗には、集めるべき削除理由もブレッドクラムも無い。
		if (hr != DXGI_ERROR_DEVICE_REMOVED && hr != DXGI_ERROR_DEVICE_HUNG)
		{
			DxFail(hr, msg, file, line);
			return;
		}

		/// [EN] The HRESULT from Present only says "removed"; the device knows why.
		/// [JP] Present の HRESULT は「削除された」としか言わない。理由はデバイスが知っている。
		HRESULT removedReason = device ? device->GetDeviceRemovedReason() : E_FAIL;
		_com_error reasonError(removedReason);

		std::string output = std::format("重要：GPUデバイスが削除されました。\n\n" "詳細: {}\n" "Present等のコード: {:#010x}\n" "GetDeviceRemovedReason: {:#010x} ({})\n\n" "場所: {}:{}", msg, static_cast<Uint32>(hr), static_cast<Uint32>(removedReason), ConvertToCharString(reasonError.ErrorMessage()), std::filesystem::path(file).filename().string(), line);

		if (device)
		{
			/// [EN] Each DRED query can fail on its own, and every failure is written into the report instead of stopping it.
			/// [JP] DRED の問い合わせはそれぞれ単独で失敗しうる。失敗はレポートを止めずに、その中へ書き込む。
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
					/// [EN] Walk every node, not just the head: there is one node per command list, and the hung one is where the completed count never reached the total.
					/// [JP] 先頭だけでなく全ノードを辿る。ノードはコマンドリストごとに1つあり、ハングしたのは完了数が総数に届かなかったノード。
					Uint32 nodeCount = 0;
					Uint32 incompleteCount = 0;
					for (const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbs.pHeadAutoBreadcrumbNode; node != nullptr; node = node->pNext)
					{
						nodeCount++;

						/// [EN] pLastBreadcrumbValue is how many ops of this list the GPU finished; a list that finished them all is not the culprit.
						/// [JP] pLastBreadcrumbValue は、このリストのうち GPU が終えたコマンドの数。全て終えたリストは原因ではない。
						Uint32 completedCount = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
						if (!node->pCommandHistory || completedCount >= node->BreadcrumbCount)
						{
							continue;
						}

						/// [EN] Only the first four incomplete lists are detailed, to keep the message box readable; the rest are only counted.
						/// [JP] メッセージボックスを読める大きさに保つため、詳しく出すのは未完了のリストの最初の4つだけ。残りは数えるだけ。
						incompleteCount++;
						if (incompleteCount > 4)
						{
							continue;
						}

						output += std::format("\n\n[未完了 {}] 実行中断時のGPUコマンド: {} (完了済み {}/{})", incompleteCount, AutoBreadcrumbOpToString(node->pCommandHistory[completedCount]), completedCount, node->BreadcrumbCount);

						/// [EN] The few ops that did complete just before it, for context.
						/// [JP] 直前に完了していた数個のコマンド(文脈把握用)。
						Uint32 historyStart = completedCount > 5 ? completedCount - 5 : 0;
						for (Uint32 historyIndex = historyStart; historyIndex < completedCount; historyIndex++)
						{
							output += std::format("\n    直前: {}", AutoBreadcrumbOpToString(node->pCommandHistory[historyIndex]));
						}
					}

					output += std::format("\n\nDRED ノード数: {} (うち未完了 {})", nodeCount, incompleteCount);
				}

				/// [EN] A non-zero page-fault address means the GPU touched memory that was not (or no longer) mapped.
				/// [JP] ページフォルトのアドレスが 0 でなければ、GPU がマップされていない（もう無い）メモリに触れたことを示す。
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

		/// [EN] Aftermath's report can block for seconds while the driver finishes the dump, so it goes last to keep the DRED report above from being delayed.
		/// [JP] Aftermath のレポートはドライバがダンプを書き終えるまで数秒止まることがあるので、上の DRED レポートを遅らせないよう最後に行う。
		output += AftermathCrashTracker::Report().str();

		/// [EN] The message box takes UTF-16, so the Japanese text is converted first.
		/// [JP] メッセージボックスは UTF-16 を受け取るので、先に日本語の文字列を変換する。
		std::wstring wideOutput = ConvertToWideString(output);

		MessageBoxW(NULL, wideOutput.c_str(), L"SeedCore Engine - GPU Device Removed", MB_ICONERROR | MB_OK);

		__debugbreak();
	}
}
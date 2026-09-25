#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Process-wide NVIDIA Nsight Aftermath integration: collects GPU crash
	* dumps (and, since GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo
	* is enabled, shader debug information) so a GPU hang/TDR leaves a
	* `.nv-gpudmp` file that Nsight Graphics can open to show the exact
	* shader/instruction/call stack that was executing, which DRED alone
	* cannot pinpoint. It is a class of static members (like LogSystem)
	* rather than an instance, because Aftermath itself is a single
	* per-process facility: GFSDK_Aftermath_EnableGpuCrashDumps and
	* GFSDK_Aftermath_DX12_Initialize apply to the one device the process
	* creates.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プロセス全体で共有する NVIDIA Nsight Aftermath 統合。GPU クラッシュ
	* ダンプ（および GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo
	* を有効にしているのでシェーダデバッグ情報）を収集し、GPU ハング/TDR
	* 発生時に `.nv-gpudmp` ファイルを残す。Nsight Graphics で開けば、
	* 実行中だった正確なシェーダ/命令/コールスタックが分かり、DRED だけ
	* では突き止められない箇所まで特定できる。LogSystem と同様、
	* インスタンスではなく static メンバのクラスとして持つ。Aftermath 自体
	* がプロセス単位の機能で、GFSDK_Aftermath_EnableGpuCrashDumps と
	* GFSDK_Aftermath_DX12_Initialize はプロセスが作る1つのデバイスに
	* 適用されるため。
	*/
	class SEEDCORE_API AftermathCrashTracker
	{
	public:
		/**
		* [EN]
		* Enables Aftermath GPU crash dump collection. Must be called before
		* any D3D12 device is created; crashes on devices created earlier are
		* invisible to Aftermath.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Aftermath の GPU クラッシュダンプ収集を有効化する。D3D12 デバイスを
		* 作成する前に呼ぶ必要がある。それより前に作成されたデバイスの
		* クラッシュは Aftermath から見えない。
		*/
		static void Enable();

		/**
		* [EN]
		* Configures Aftermath for the given device. Resource tracking is
		* always on, so a page fault's address can be tied back to a
		* resource, and shader debug info generation is always on, so a
		* crash dump's faulting shader resolves to a source file/line in
		* Nsight Graphics using the debug info DXC embeds. The heavier
		* features (event markers with automatic call stack capture, extra
		* shader error reporting) are enabled only with
		* DEEP_D3D12_DEBUG_MODE, like D3D12DebugLayer's GPU-Based
		* Validation, because both carry real overhead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定デバイスに対して Aftermath を構成する。リソース追跡は常に有効で、
		* ページフォルトのアドレスをリソースへ結び付けられる。シェーダ
		* デバッグ情報の生成も常に有効で、DXC が埋め込んだデバッグ情報を
		* 使って、クラッシュダンプの落ちたシェーダが Nsight Graphics で
		* ソースファイル/行番号に解決される。より重い機能（自動コール
		* スタック付きのイベントマーカー、追加のシェーダエラー報告）は、
		* D3D12DebugLayer の GPU-Based Validation と同じく実コストを伴うため、
		* DEEP_D3D12_DEBUG_MODE のときだけ有効にする。
		*/
		static void Create(ID3D12Device* device);

		/**
		* [EN]
		* Call once device removal/hang is detected (from DeviceRemovedFail).
		* Queries Aftermath's own device status (a finer-grained reason than
		* GetDeviceRemovedReason: Timeout/PageFault/OutOfMemory/DmaFault/...)
		* and polls GFSDK_Aftermath_GetCrashDumpStatus for up to 5 seconds so
		* the crash dump write (done asynchronously by the NVIDIA driver
		* thread) has a chance to finish before the process moves on. Returns
		* a human-readable summary to fold into DeviceRemovedFail's message
		* box, including the crash dump file path once written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デバイス削除/ハング検知時（DeviceRemovedFail から）に1度呼ぶ。
		* Aftermath 自身のデバイスステータス（GetDeviceRemovedReason より
		* 細かい理由。Timeout/PageFault/OutOfMemory/DmaFault/…）を照会し、
		* GFSDK_Aftermath_GetCrashDumpStatus を最大5秒ポーリングして、
		* （NVIDIA ドライバのスレッドが非同期に書く）クラッシュダンプの書き
		* 込みが、プロセスが先に進む前に終わる機会を与える。DeviceRemovedFail
		* のメッセージボックスへ組み込める人間可読の要約を返す。書き込み済み
		* ならクラッシュダンプファイルのパスも含む。
		*/
		static String Report();

		/**
		* [EN]
		* Disables Aftermath GPU crash dump collection. Call once before
		* process shutdown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Aftermath の GPU クラッシュダンプ収集を無効化する。プロセス終了前に
		* 1度呼ぶ。
		*/
		static void Disable();

	private:
		/**
		* [EN]
		* Returns the directory every crash dump / shader debug info file
		* is written into, creating it if it does not exist yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全てのクラッシュダンプ/シェーダデバッグ情報ファイルの書き出し先
		* ディレクトリを返す。まだ無ければ作成する。
		*/
		static const Char* DumpDirectory();

		/**
		* [EN]
		* Aftermath callback that receives a finished GPU crash dump and
		* writes it to a timestamped .nv-gpudmp file. A static function,
		* since Aftermath takes plain C callbacks.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 完成した GPU クラッシュダンプを受け取り、時刻付きの .nv-gpudmp
		* ファイルへ書き出す Aftermath のコールバック。Aftermath は素の C の
		* コールバックを受け取るので static 関数にしている。
		*/
		static void OnCrashDump(const void* gpuCrashDump, Uint32 gpuCrashDumpSize, void* userData);

		/**
		* [EN]
		* Aftermath callback that receives one shader's debug info and
		* writes it to a .nvdbg file named by its identifier, where Nsight
		* Graphics looks for it when opening the dump.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダ1つ分のデバッグ情報を受け取り、その識別子を名前にした
		* .nvdbg ファイルへ書き出す Aftermath のコールバック。Nsight Graphics
		* はダンプを開くときにこれを探す。
		*/
		static void OnShaderDebugInfo(const void* shaderDebugInfo, Uint32 shaderDebugInfoSize, void* userData);

		/**
		* [EN]
		* Aftermath callback that adds descriptive key/value pairs (the
		* application name) to every crash dump.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全てのクラッシュダンプへ説明用のキーと値（アプリケーション名）を
		* 追加する Aftermath のコールバック。
		*/
		static void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* userData);

	private:
		/// [EN] Guards lastCrashDumpPath_ and shaderDebugInfoCount_, which Aftermath callbacks write from driver threads.
           /// [JP] Aftermath のコールバックがドライバのスレッドから書き込む lastCrashDumpPath_ と shaderDebugInfoCount_ を守る。
		static std::mutex mutex_;

		/// [EN] Path of the most recently written crash dump, or empty if none has been written in this process.
		/// [JP] 直近に書き出したクラッシュダンプのパス。このプロセスで一度も書いていなければ空。
		static String lastCrashDumpPath_;

		/// [EN] Number of shader debug info files written in this process; included in Report()'s summary.
		/// [JP] このプロセスで書き出したシェーダデバッグ情報ファイルの数。Report() の要約に含める。
		static Uint32 shaderDebugInfoCount_;
	};
}

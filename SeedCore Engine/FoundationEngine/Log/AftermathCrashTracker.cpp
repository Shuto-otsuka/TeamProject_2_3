#include <FoundationEngine/Log/AftermathCrashTracker.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Notice.h>

namespace SeedCore
{
	/// [EN] Definitions of the shared state declared in AftermathCrashTracker.h.
	/// [JP] AftermathCrashTracker.h で宣言した、共有の状態の定義。
	std::mutex AftermathCrashTracker::mutex_;
	String AftermathCrashTracker::lastCrashDumpPath_;
	Uint32 AftermathCrashTracker::shaderDebugInfoCount_ = 0;

	/**
	* [EN]
	* Returns the directory every crash dump / shader debug info file is
	* written into, creating it if it does not exist yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのクラッシュダンプ/シェーダデバッグ情報ファイルの書き出し先
	* ディレクトリを返す。まだ無ければ作成する。
	*/
	const Char* AftermathCrashTracker::DumpDirectory()
	{
		/// [EN] A path relative to the working directory, one level up from it.
		/// [JP] 作業ディレクトリを基準にした、その1つ上のパス。
		static const Char* directory = "../Logs/GpuCrashDumps";

		/// [EN] create_directories does nothing when the directory already exists, so calling it every time is safe.
		/// [JP] create_directories はディレクトリが既にあれば何もしないので、毎回呼んでも問題ない。
		std::filesystem::create_directories(directory);
		return directory;
	}

	/**
	* [EN]
	* Enables Aftermath GPU crash dump collection. Must be called before
	* any D3D12 device is created.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Aftermath の GPU クラッシュダンプ収集を有効化する。D3D12 デバイスを
	* 作成する前に呼ぶ必要がある。
	*/
	void AftermathCrashTracker::Enable()
	{
		/// [EN] Only DX devices are watched, and the three callbacks receive dumps, shader debug info and description data.
		/// [JP] 監視するのは DX のデバイスだけ。3つのコールバックがダンプ、シェーダデバッグ情報、説明の情報を受け取る。
		GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
			GFSDK_Aftermath_Version_API,
			GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
			GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,
			OnCrashDump,
			OnShaderDebugInfo,
			OnDescription,
			nullptr,
			nullptr);

		/// [EN] Failing here is expected on non-NVIDIA hardware, so it is only a warning and the engine carries on without dumps.
		/// [JP] NVIDIA 以外のハードウェアではここで失敗するのが普通なので、警告だけ出してダンプ無しで続ける。
		if (!GFSDK_Aftermath_SUCCEED(result))
		{
			SC_LOG_WARNING("Nsight Aftermath の有効化に失敗しました(結果コード: {:#010x})。対応する NVIDIA GPU/ドライバでない可能性があります。GPU クラッシュダンプは収集されません。", static_cast<Uint32>(result));
		}
	}

	/**
	* [EN]
	* Configures Aftermath for the given device, with resource tracking
	* and shader debug info always on and the heavier features only in
	* DEEP_D3D12_DEBUG_MODE.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定デバイスに対して Aftermath を構成する。リソース追跡とシェーダ
	* デバッグ情報は常に有効にし、より重い機能は DEEP_D3D12_DEBUG_MODE の
	* ときだけ有効にする。
	*/
	void AftermathCrashTracker::Create(ID3D12Device* device)
	{
		/// [EN] GenerateShaderDebugInfo is always on: the debug info DXC embeds only reaches a dump's shader location if Aftermath captures it too.
		/// [JP] GenerateShaderDebugInfo は常に有効にする。DXC が埋め込むデバッグ情報は、Aftermath も収集しないとダンプのシェーダ位置に出ない。
		Uint32 flags = GFSDK_Aftermath_FeatureFlags_EnableResourceTracking | GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;

		/// [EN] Markers, call stacks and shader error reporting slow every frame, so they are only for deep debugging sessions.
		/// [JP] マーカー、コールスタック、シェーダエラー報告は毎フレームを遅くするので、詳しく調べるときだけ使う。
#if DEEP_D3D12_DEBUG_MODE
		flags |= GFSDK_Aftermath_FeatureFlags_EnableMarkers;
		flags |= GFSDK_Aftermath_FeatureFlags_CallStackCapturing;
		flags |= GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting;
#endif

		GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, flags, device);
		if (!GFSDK_Aftermath_SUCCEED(result))
		{
			SC_LOG_WARNING("Nsight Aftermath のデバイス初期化に失敗しました(結果コード: {:#010x})。GPU クラッシュダンプは収集されません。", static_cast<Uint32>(result));
			return;
		}

		/// [EN] Telling where dumps will go up front saves searching for them after a crash.
		/// [JP] ダンプの書き出し先を先に知らせておけば、クラッシュ後に探す手間が省ける。
		SC_LOG_NOTICE("Nsight Aftermath を有効化しました。GPU デバイス削除時は '{}' にクラッシュダンプを書き出します。", DumpDirectory());
	}

	/**
	* [EN]
	* Builds the Aftermath part of the device-removed report: the device
	* status, and the crash dump path after waiting up to 5 seconds for
	* the driver to finish writing it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイス削除レポートの Aftermath 部分を作る。デバイスステータスと、
	* ドライバが書き終えるのを最大5秒待った上でのクラッシュダンプのパス。
	*/
	String AftermathCrashTracker::Report()
	{
		std::string output;

		GFSDK_Aftermath_Device_Status deviceStatus = GFSDK_Aftermath_Device_Status_Unknown;
		GFSDK_Aftermath_Result deviceStatusResult = GFSDK_Aftermath_GetDeviceStatus(&deviceStatus);
		if (GFSDK_Aftermath_SUCCEED(deviceStatusResult))
		{
			/// [EN] Finer than GetDeviceRemovedReason: tells a timeout apart from a page fault, OOM, or a removal with no GPU fault.
			/// [JP] GetDeviceRemovedReason より細かく、タイムアウトを、ページフォルト・メモリ不足・GPU 障害の無い削除と区別する。
			const Char* statusText = "Unknown";
			switch (deviceStatus)
			{
			case GFSDK_Aftermath_Device_Status_Active:                  statusText = "Active(正常)"; break;
			case GFSDK_Aftermath_Device_Status_Timeout:                 statusText = "Timeout(長時間実行しているシェーダ/処理によるタイムアウト)"; break;
			case GFSDK_Aftermath_Device_Status_OutOfMemory:             statusText = "OutOfMemory"; break;
			case GFSDK_Aftermath_Device_Status_PageFault:               statusText = "PageFault(不正な GPU 仮想アドレスアクセス)"; break;
			case GFSDK_Aftermath_Device_Status_Stopped:                 statusText = "Stopped"; break;
			case GFSDK_Aftermath_Device_Status_Reset:                   statusText = "Reset"; break;
			case GFSDK_Aftermath_Device_Status_DmaFault:                statusText = "DmaFault(不正なレンダリング呼び出し)"; break;
			case GFSDK_Aftermath_Device_Status_DeviceRemovedNoGpuFault:  statusText = "DeviceRemovedNoGpuFault(GPU 側の障害を伴わないデバイス削除 - ドライバクラッシュや外部リセットの可能性)"; break;
			default:                                                     statusText = "Unknown(未対応ドライバの可能性)"; break;
			}
			output += std::format("\n\nNsight Aftermath デバイスステータス: {}", statusText);
		}
		else
		{
			output += std::format("\n\nNsight Aftermath デバイスステータス: 取得できません({:#010x})。Aftermath が有効化されていない、または未対応の GPU/ドライバの可能性があります。", static_cast<Uint32>(deviceStatusResult));
		}

		/// [EN] The dump is written asynchronously on an NVIDIA driver thread; the wait has a limit so a driver that never finishes cannot hang the report.
		/// [JP] ダンプは NVIDIA ドライバのスレッドで非同期に書かれる。終わらないドライバでレポートが止まらないよう、待ち時間に上限を付ける。
		GFSDK_Aftermath_CrashDump_Status crashDumpStatus = GFSDK_Aftermath_CrashDump_Status_Unknown;
		GFSDK_Aftermath_GetCrashDumpStatus(&crashDumpStatus);

		auto waitStart = std::chrono::steady_clock::now();
		constexpr auto crashDumpTimeout = std::chrono::milliseconds(5000);

		/// [EN] The wait ends on CollectingDataFailed, Finished or Unknown, or when the time limit runs out.
		/// [JP] 待つのは CollectingDataFailed、Finished、Unknown のどれかになるか、時間切れになるまで。
		while (crashDumpStatus != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed &&
			crashDumpStatus != GFSDK_Aftermath_CrashDump_Status_Finished &&
			crashDumpStatus != GFSDK_Aftermath_CrashDump_Status_Unknown &&
			std::chrono::steady_clock::now() - waitStart < crashDumpTimeout)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			GFSDK_Aftermath_GetCrashDumpStatus(&crashDumpStatus);
		}

		/// [EN] The path and count are written by the callbacks on driver threads, so they are read under the lock.
		/// [JP] パスと数はドライバのスレッドでコールバックが書くので、ロックの内側で読む。
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (!lastCrashDumpPath_.str().empty())
			{
				output += std::format("\n\nNsight Aftermath クラッシュダンプ: {}", lastCrashDumpPath_.str());
				if (shaderDebugInfoCount_ > 0)
				{
					output += std::format("\n(シェーダデバッグ情報 {} 個を同ディレクトリへ書き出し済み。Nsight Graphics でこのダンプを開く際に指定してください)", shaderDebugInfoCount_);
				}
			}
			else
			{
				output += "\n\nNsight Aftermath クラッシュダンプ: 書き出されませんでした(未対応の GPU/ドライバ、または EnableCrashDumps 未実行の可能性)";
			}
		}

		return String(output);
	}

	/**
	* [EN]
	* Disables Aftermath GPU crash dump collection.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Aftermath の GPU クラッシュダンプ収集を無効化する。
	*/
	void AftermathCrashTracker::Disable()
	{
		GFSDK_Aftermath_DisableGpuCrashDumps();
	}

	/**
	* [EN]
	* Writes a finished GPU crash dump to a timestamped .nv-gpudmp file
	* and remembers its path for Report().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 完成した GPU クラッシュダンプを時刻付きの .nv-gpudmp ファイルへ
	* 書き出し、Report() のためにそのパスを覚えておく。
	*/
	void AftermathCrashTracker::OnCrashDump(const void* gpuCrashDump, Uint32 gpuCrashDumpSize, void* userData)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		/// [EN] A local-time stamp in the file name keeps dumps from several crashes apart and sorts them by time.
		/// [JP] ファイル名に現地時刻を入れることで、複数のクラッシュのダンプを区別し、時刻順に並べられる。
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm localNow{};
		localtime_s(&localNow, &now);

		std::ostringstream timestamp;
		timestamp << std::put_time(&localNow, "%Y%m%d_%H%M%S");

		std::string path = std::format("{}/{}.nv-gpudmp", DumpDirectory(), timestamp.str());

		/// [EN] The dump is an opaque binary blob that only Nsight Graphics reads.
		/// [JP] ダンプは Nsight Graphics だけが読む、中身の分からないバイナリ。
		std::ofstream file(path, std::ios::binary);
		if (file)
		{
			file.write(static_cast<const Char*>(gpuCrashDump), gpuCrashDumpSize);
		}

		lastCrashDumpPath_ = String(path);
	}

	/**
	* [EN]
	* Writes one shader's debug info to a .nvdbg file named by its
	* Aftermath identifier, which is how Nsight Graphics matches it to
	* the shader in a dump.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シェーダ1つ分のデバッグ情報を、Aftermath の識別子を名前にした
	* .nvdbg ファイルへ書き出す。Nsight Graphics はこの名前でダンプ内の
	* シェーダと結び付ける。
	*/
	void AftermathCrashTracker::OnShaderDebugInfo(const void* shaderDebugInfo, Uint32 shaderDebugInfoSize, void* userData)
	{
		/// [EN] Without an identifier the file could never be matched to a shader, so it is not written.
		/// [JP] 識別子が得られなければファイルをシェーダと結び付けられないので、書き出さない。
		GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier{};
		GFSDK_Aftermath_Result result = GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, shaderDebugInfo, shaderDebugInfoSize, &identifier);
		if (!GFSDK_Aftermath_SUCCEED(result))
		{
			return;
		}

		std::lock_guard<std::mutex> lock(mutex_);

		/// [EN] The two 64-bit halves of the identifier, in hex, form the file name.
		/// [JP] 識別子の2つの64ビットの値を16進にしたものがファイル名になる。
		std::string path = std::format("{}/{:016x}{:016x}.nvdbg", DumpDirectory(), identifier.id[0], identifier.id[1]);

		std::ofstream file(path, std::ios::binary);
		if (file)
		{
			file.write(static_cast<const Char*>(shaderDebugInfo), shaderDebugInfoSize);
			shaderDebugInfoCount_++;
		}
	}

	/**
	* [EN]
	* Adds the application name to every crash dump.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのクラッシュダンプへアプリケーション名を追加する。
	*/
	void AftermathCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* userData)
	{
		addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "SeedCore Engine");
	}
}

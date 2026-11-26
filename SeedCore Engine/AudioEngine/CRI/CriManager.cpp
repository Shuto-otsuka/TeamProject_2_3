#include <External/CRI/Include/cri_adx2le.h>
#include <cstdlib>
#include <AudioEngine/CRI/CriManager.h>
#include <AudioEngine/Audio/AudioByteStream.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>

#pragma comment(lib, "../External/CRI/Lib/cri_ware_pcx64_le_import.lib")

namespace SeedCore
{
	namespace
	{
		/**
		* [JP]
		* 音のライブラリが使うメモリを用意する。
		*
		* @brief  メモリを用意する
		* @param  size 用意する大きさ
		* @return 用意したメモリ
		*
		* [EN]
		* Gets memory for the sound library.
		*
		* @brief  Gets memory
		* @param  size How much to get
		* @return The memory
		*/
		void* ScCriMalloc(void*, CriUint32 size)
		{
			return std::malloc(size);
		}

		/**
		* [JP]
		* 音のライブラリが使ったメモリを返す。
		*
		* @brief メモリを返す
		* @param ptr 返すメモリ
		*
		* [EN]
		* Gives back memory the sound library used.
		*
		* @brief Frees memory
		* @param ptr The memory to give back
		*/
		void ScCriFree(void*, void* ptr)
		{
			std::free(ptr);
		}

		/**
		* [JP]
		* 音のライブラリからのエラーや警告を、エンジンのログに出す。
		*
		* @brief エラーをログに出す
		* @param id 何のエラーかを表す文字
		* @param p1 説明に使う値その1
		* @param p2 説明に使う値その2
		*
		* [EN]
		* Sends errors and warnings from the sound library to the engine log.
		*
		* @brief Logs errors
		* @param id Text that tells which error it is
		* @param p1 The first value for the message
		* @param p2 The second value for the message
		*/
		void ScCriErrorCallback(const CriChar8* id, CriUint32 p1, CriUint32 p2, CriUint32*)
		{
			const CriChar8* message = criErr_ConvertIdToMessage(id, p1, p2);
			if (id && id[0] == 'W')
			{
				SC_LOG_WARNING("CRIWARE: {} ({})", message ? message : "", id);
			}
			else
			{
				SC_LOG_ERROR("CRIWARE: {} ({})", message ? message : "", id);
			}
		}
	}

	/**
	* [JP]
	* CriManager の中身。音のライブラリを実際に動かす。
	*
	* @brief CriManager の中身
	*
	* [EN]
	* The inside of CriManager; it actually runs the sound library.
	*
	* @brief The inside of CriManager
	*/
	class CriManager::Impl final
	{
	public:
		/**
		* [JP]
		* ゲーム用に始める。ACF はファイルから読まず、保存してあるものを使う。
		*
		* @brief  ゲーム用に始める
		* @param  gameConfig ゲームの設定
		* @return うまくいったら true
		*
		* [EN]
		* Starts for the game; it uses the saved ACF instead of reading the file.
		*
		* @brief  Starts for the game
		* @param  gameConfig The game settings
		* @return True if it worked
		*/
		Bool Initialize(const GameConfig& gameConfig);

		/**
		* [JP]
		* エディタ用に始める。設定にある ACF ファイルを読む。
		*
		* @brief  エディタ用に始める
		* @param  editorConfig エディタの設定
		* @return うまくいったら true
		*
		* [EN]
		* Starts for the editor; it reads the ACF file named in the settings.
		*
		* @brief  Starts for the editor
		* @param  editorConfig The editor settings
		* @return True if it worked
		*/
		Bool Initialize(const EditorConfig& editorConfig);

		/**
		* [JP]
		* 音のライブラリを1フレーム分動かす。
		*
		* @brief 1フレーム分動かす
		*
		* [EN]
		* Runs the sound library for one frame.
		*
		* @brief Runs one frame
		*/
		void Execute();

		/**
		* [JP]
		* 音のライブラリを終わらせる。
		*
		* @brief 終わらせる
		*
		* [EN]
		* Shuts the sound library down.
		*
		* @brief Shuts down
		*/
		void Finalize();

	public:
		/**
		* [JP]
		* 全体の音量を変える。
		*
		* @brief 全体の音量を変える
		* @param volume 音量
		*
		* [EN]
		* Changes the overall volume.
		*
		* @brief Changes the overall volume
		* @param volume The volume
		*/
		void MasterVolume(Float volume);

		/**
		* [JP]
		* 全体の音量を返す。
		*
		* @brief  全体の音量を返す
		* @return 全体の音量
		*
		* [EN]
		* Returns the overall volume.
		*
		* @brief  Returns the overall volume
		* @return The overall volume
		*/
		Float MasterVolume()const;

		/**
		* [JP]
		* 元に戻すときに使う、最初の全体の音量を決める。
		*
		* @brief 最初の全体の音量を決める
		* @param volume 最初の音量
		*
		* [EN]
		* Sets the starting overall volume used when resetting.
		*
		* @brief Sets the default overall volume
		* @param volume The starting volume
		*/
		void DefaultMasterVolume(Float volume);

		/**
		* [JP]
		* 最初の全体の音量を返す。
		*
		* @brief  最初の全体の音量を返す
		* @return 最初の音量
		*
		* [EN]
		* Returns the starting overall volume.
		*
		* @brief  Returns the default overall volume
		* @return The starting volume
		*/
		Float DefaultMasterVolume()const;

		/**
		* [JP]
		* 種類ごとの音量を変える。
		*
		* @brief 種類ごとの音量を変える
		* @param category 種類の名前
		* @param volume   音量
		*
		* [EN]
		* Changes the volume of one category.
		*
		* @brief Changes a category volume
		* @param category The category name
		* @param volume   The volume
		*/
		void CategoryVolume(const String& category, Float volume);

		/**
		* [JP]
		* 種類ごとの音量を返す。変えていなければ最初の音量を返す。
		*
		* @brief  種類ごとの音量を返す
		* @param  category 種類の名前
		* @return その種類の音量
		*
		* [EN]
		* Returns the volume of one category, or its starting volume if never changed.
		*
		* @brief  Returns a category volume
		* @param  category The category name
		* @return That category's volume
		*/
		Float CategoryVolume(const String& category)const;

		/**
		* [JP]
		* 音の種類の名前の一覧を返す。
		*
		* @brief  種類の一覧を返す
		* @return 種類の名前の一覧
		*
		* [EN]
		* Returns the list of category names.
		*
		* @brief  Returns the category list
		* @return The category names
		*/
		const DynamicArray<String>& CategoryNameList()const;

		/**
		* [JP]
		* 音量を全部、最初の値に戻す。
		*
		* @brief 音量を元に戻す
		*
		* [EN]
		* Puts every volume back to its starting value.
		*
		* @brief Resets the volumes
		*/
		void ResetVolume();

	public:
		/**
		* [JP]
		* 保存してある ACF と音量をファイルから読む。
		*
		* @brief  保存した設定を読む
		* @return うまくいったら true
		*
		* [EN]
		* Reads the saved ACF and volumes from a file.
		*
		* @brief  Loads the saved settings
		* @return True if it worked
		*/
		Bool Load();

		/**
		* [JP]
		* ACF と音量をファイルに保存する。
		*
		* @brief  設定を保存する
		* @return うまくいったら true
		*
		* [EN]
		* Saves the ACF and volumes to a file.
		*
		* @brief  Saves the settings
		* @return True if it worked
		*/
		Bool Save();

	private:
		/**
		* [JP]
		* 音のライブラリを始める本体。ACF の登録や、音を鳴らす枠の確保もする。
		*
		* @brief  音のライブラリを始める
		* @param  masterPath 読む ACF ファイル。空なら保存してあるものを使う
		* @return うまくいったら true
		*
		* [EN]
		* The main start-up of the sound library; it also registers the ACF and reserves voices.
		*
		* @brief  Starts the sound library
		* @param  masterPath The ACF file to read; empty uses the saved one
		* @return True if it worked
		*/
		Bool Initialize(const String& masterPath);

	private:
		/// [JP] WAV を鳴らすための枠
		/// [EN] The voices used to play WAVs
		CriAtomExVoicePoolHn waveVoicePool_ = nullptr;

		/// [JP] キューシートを鳴らすための枠
		/// [EN] The voices used to play cue sheets
		CriAtomExVoicePoolHn standardVoicePool_ = nullptr;

		/// [JP] 長い音を少しずつ読むための仕組みの番号
		/// [EN] The id of the system that streams long sounds
		CriAtomDbasId dbasID_ = CRIATOMEXDBAS_ILLEGAL_ID;

		/// [JP] ACF のデータ
		/// [EN] The ACF data
		DynamicArray<Byte> acfData_;

		/// [JP] 音の種類の名前の一覧
		/// [EN] The category names
		DynamicArray<String> categoryNames_;

		/// [JP] 種類ごとの最初の音量
		/// [EN] The starting volume of each category
		FlatMap<String, Float> defaultCategoryVolumes_;

		/// [JP] 種類ごとの今の音量
		/// [EN] The current volume of each category
		FlatMap<String, Float> categoryVolumes_;

		/// [JP] 読む ACF ファイルの場所
		/// [EN] Where the ACF file to read is
		String masterPath_;

		/// [JP] 全体の今の音量
		/// [EN] The current overall volume
		Float masterVolume_ = 1.0f;

		/// [JP] 全体の最初の音量
		/// [EN] The starting overall volume
		Float defaultMasterVolume_ = 1.0f;
	};

	/**
	* [JP]
	* ゲーム用に始める。ACF はファイルから読まず、保存してあるものを使う。
	*
	* @brief  ゲーム用に始める
	* @param  gameConfig ゲームの設定
	* @return うまくいったら true
	*
	* [EN]
	* Starts for the game; it uses the saved ACF instead of reading the file.
	*
	* @brief  Starts for the game
	* @param  gameConfig The game settings
	* @return True if it worked
	*/
	Bool CriManager::Impl::Initialize(const GameConfig& gameConfig)
	{
		return Initialize(String());
	}

	/**
	* [JP]
	* エディタ用に始める。設定にある ACF ファイルを読む。
	*
	* @brief  エディタ用に始める
	* @param  editorConfig エディタの設定
	* @return うまくいったら true
	*
	* [EN]
	* Starts for the editor; it reads the ACF file named in the settings.
	*
	* @brief  Starts for the editor
	* @param  editorConfig The editor settings
	* @return True if it worked
	*/
	Bool CriManager::Impl::Initialize(const EditorConfig& editorConfig)
	{
		return Initialize(editorConfig.acfPath_);
	}

	/**
	* [JP]
	* 音のライブラリを1フレーム分動かす。
	*
	* @brief 1フレーム分動かす
	*
	* [EN]
	* Runs the sound library for one frame.
	*
	* @brief Runs one frame
	*/
	void CriManager::Impl::Execute()
	{
		criAtomEx_ExecuteMain();
	}

	/**
	* [JP]
	* 音のライブラリを終わらせる。
	*
	* @brief 終わらせる
	*
	* [EN]
	* Shuts the sound library down.
	*
	* @brief Shuts down
	*/
	void CriManager::Impl::Finalize()
	{
		if (!criAtomEx_IsInitialized())
		{
			return;
		}

		/// [JP] 音を鳴らす枠と、長い音を読む仕組みを片付ける
		/// [EN] Free the voices and the streaming system
		criAtomExVoicePool_FreeAll();
		waveVoicePool_ = nullptr;
		standardVoicePool_ = nullptr;

		if (dbasID_ != CRIATOMEXDBAS_ILLEGAL_ID)
		{
			criAtomExDbas_Destroy(dbasID_);
			dbasID_ = CRIATOMEXDBAS_ILLEGAL_ID;
		}

		/// [JP] ACF の登録を外して、音のライブラリを終わらせる
		/// [EN] Unregister the ACF and shut the sound library down
		if (!acfData_.empty())
		{
			criAtomEx_UnregisterAcf();
			acfData_.clear();
		}

		criAtomEx_Finalize_WASAPI();

		AudioByteStream::Uninstall();
	}

	/**
	* [JP]
	* 全体の音量を変える。
	*
	* @brief 全体の音量を変える
	* @param volume 音量
	*
	* [EN]
	* Changes the overall volume.
	*
	* @brief Changes the overall volume
	* @param volume The volume
	*/
	void CriManager::Impl::MasterVolume(Float volume)
	{
		masterVolume_ = volume;

		criAtomExAsr_SetBusVolume(0, masterVolume_);
	}

	/**
	* [JP]
	* 全体の音量を返す。
	*
	* @brief  全体の音量を返す
	* @return 全体の音量
	*
	* [EN]
	* Returns the overall volume.
	*
	* @brief  Returns the overall volume
	* @return The overall volume
	*/
	Float CriManager::Impl::MasterVolume()const
	{
		return masterVolume_;
	}

	/**
	* [JP]
	* 元に戻すときに使う、最初の全体の音量を決める。
	*
	* @brief 最初の全体の音量を決める
	* @param volume 最初の音量
	*
	* [EN]
	* Sets the starting overall volume used when resetting.
	*
	* @brief Sets the default overall volume
	* @param volume The starting volume
	*/
	void CriManager::Impl::DefaultMasterVolume(Float volume)
	{
		defaultMasterVolume_ = volume;
	}

	/**
	* [JP]
	* 最初の全体の音量を返す。
	*
	* @brief  最初の全体の音量を返す
	* @return 最初の音量
	*
	* [EN]
	* Returns the starting overall volume.
	*
	* @brief  Returns the default overall volume
	* @return The starting volume
	*/
	Float CriManager::Impl::DefaultMasterVolume()const
	{
		return defaultMasterVolume_;
	}

	/**
	* [JP]
	* 種類ごとの音量を変える。
	*
	* @brief 種類ごとの音量を変える
	* @param category 種類の名前
	* @param volume   音量
	*
	* [EN]
	* Changes the volume of one category.
	*
	* @brief Changes a category volume
	* @param category The category name
	* @param volume   The volume
	*/
	void CriManager::Impl::CategoryVolume(const String& category, Float volume)
	{
		/// [JP] 覚えておいてから、音のライブラリにも伝える
		/// [EN] Remember it, then tell the sound library too
		if (categoryVolumes_.contains(category))
		{
			categoryVolumes_.at(category) = volume;
		}
		else
		{
			categoryVolumes_.insert({ category, volume });
		}

		criAtomExCategory_SetVolumeByName(category.c_str(), volume);
	}

	/**
	* [JP]
	* 種類ごとの音量を返す。変えていなければ最初の音量を返す。
	*
	* @brief  種類ごとの音量を返す
	* @param  category 種類の名前
	* @return その種類の音量
	*
	* [EN]
	* Returns the volume of one category, or its starting volume if never changed.
	*
	* @brief  Returns a category volume
	* @param  category The category name
	* @return That category's volume
	*/
	Float CriManager::Impl::CategoryVolume(const String& category)const
	{
		/// [JP] 変えた音量があればそれを、なければ最初の音量を返す
		/// [EN] Return the changed volume if there is one, otherwise the starting one
		if (categoryVolumes_.contains(category))
		{
			return categoryVolumes_.at(category);
		}

		if (defaultCategoryVolumes_.contains(category))
		{
			return defaultCategoryVolumes_.at(category);
		}

		return 1.0f;
	}

	/**
	* [JP]
	* 音の種類の名前の一覧を返す。
	*
	* @brief  種類の一覧を返す
	* @return 種類の名前の一覧
	*
	* [EN]
	* Returns the list of category names.
	*
	* @brief  Returns the category list
	* @return The category names
	*/
	const DynamicArray<String>& CriManager::Impl::CategoryNameList()const
	{
		return categoryNames_;
	}

	/**
	* [JP]
	* 音量を全部、最初の値に戻す。
	*
	* @brief 音量を元に戻す
	*
	* [EN]
	* Puts every volume back to its starting value.
	*
	* @brief Resets the volumes
	*/
	void CriManager::Impl::ResetVolume()
	{
		/// [JP] 変えた音量を忘れて、全部最初の値に戻す
		/// [EN] Forget the changed volumes and put everything back to the start
		categoryVolumes_.clear();
		masterVolume_ = defaultMasterVolume_;

		for (const String& category : categoryNames_)
		{
			Float volume = defaultCategoryVolumes_.contains(category) ? defaultCategoryVolumes_.at(category) : 1.0f;
			criAtomExCategory_SetVolumeByName(category.c_str(), volume);
		}

		criAtomExAsr_SetBusVolume(0, masterVolume_);
	}

	/**
	* [JP]
	* 保存してある ACF と音量をファイルから読む。
	*
	* @brief  保存した設定を読む
	* @return うまくいったら true
	*
	* [EN]
	* Reads the saved ACF and volumes from a file.
	*
	* @brief  Loads the saved settings
	* @return True if it worked
	*/
	Bool CriManager::Impl::Load()
	{
		/// [JP] 設定ファイルを読んで、あるものだけ取り出す
		/// [EN] Read the settings file and take out whatever is there
		BinaryInputArchive archive;
		if (!archive.Read(String("../UserProject/Assets/Config/AudioBindings.scg")))
		{
			return false;
		}

		archive.TryField("acfData", acfData_);
		archive.TryField("masterVolume", masterVolume_);
		archive.TryField("defaultMasterVolume", defaultMasterVolume_);
		archive.TryField("categoryVolumes", categoryVolumes_);

		return true;
	}

	/**
	* [JP]
	* ACF と音量をファイルに保存する。
	*
	* @brief  設定を保存する
	* @return うまくいったら true
	*
	* [EN]
	* Saves the ACF and volumes to a file.
	*
	* @brief  Saves the settings
	* @return True if it worked
	*/
	Bool CriManager::Impl::Save()
	{
		/// [JP] 保存先のフォルダがなければ作る
		/// [EN] Make the folder to save into if it is missing
		std::filesystem::path path("../UserProject/Assets/Config/AudioBindings.scg");
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		BinaryOutputArchive archive;
		archive.Field("acfData", acfData_);
		archive.Field("masterVolume", masterVolume_);
		archive.Field("defaultMasterVolume", defaultMasterVolume_);
		archive.Field("categoryVolumes", categoryVolumes_);

		return archive.Write(String(path.string()));
	}

	/**
	* [JP]
	* 音のライブラリを始める本体。ACF の登録や、音を鳴らす枠の確保もする。
	*
	* @brief  音のライブラリを始める
	* @param  masterPath 読む ACF ファイル。空なら保存してあるものを使う
	* @return うまくいったら true
	*
	* [EN]
	* The main start-up of the sound library; it also registers the ACF and reserves voices.
	*
	* @brief  Starts the sound library
	* @param  masterPath The ACF file to read; empty uses the saved one
	* @return True if it worked
	*/
	Bool CriManager::Impl::Initialize(const String& masterPath)
	{
		masterPath_ = masterPath;

		/// [JP] エラーの出し先、メモリの用意の仕方、ファイルの読み方を決める
		/// [EN] Set where errors go, how memory is handled and how files are read
		criErr_SetCallback(ScCriErrorCallback);
		criAtomEx_SetUserAllocator(ScCriMalloc, ScCriFree, nullptr);
		criFs_SetUserAllocator(ScCriMalloc, ScCriFree, nullptr);
		AudioByteStream::Install();

		/// [JP] ファイルを扱う数の上限を決める
		/// [EN] Set the limits on how many files it handles
		CriFsConfig fileSystemConfig{};
		criFs_SetDefaultConfig(&fileSystemConfig);
		fileSystemConfig.num_binders = 256;
		fileSystemConfig.max_binds = 256;
		fileSystemConfig.num_loaders = 64;
		fileSystemConfig.max_files = 64;
		fileSystemConfig.max_path = 512;

		/// [JP] 鳴らせる音の数や音の高さの上限などを決める
		/// [EN] Set limits such as how many sounds can play and how high the pitch can go
		CriAtomExConfig_WASAPI config{};
		criAtomEx_SetDefaultConfig_WASAPI(&config);
		config.atom_ex.fs_config = &fileSystemConfig;
		config.atom_ex.thread_model = CRIATOMEX_THREAD_MODEL_MULTI;
		config.atom_ex.max_virtual_voices = 512;
		config.atom_ex.max_sequences = 512;
		config.atom_ex.max_tracks = 1024;
		config.atom_ex.max_track_items = 1024;
		config.atom_ex.max_parameter_blocks = 16384;
		config.atom_ex.coordinate_system = CRIATOMEX_COORDINATE_SYSTEM_LEFT_HANDED;

		config.atom_ex.server_frequency = 60.0f;
		config.atom_ex.parameter_update_interval = 1;
		config.atom_ex.max_pitch = 2400.0f;

		/// [JP] 出力の形（48kHz・6チャンネル）を決めて始める
		/// [EN] Set the output (48 kHz, 6 channels) and start
		config.asr.server_frequency = config.atom_ex.server_frequency;
		config.asr.output_sampling_rate = 48000;
		config.asr.output_channels = 6;
		config.asr.speaker_mapping = CRIATOM_SPEAKER_MAPPING_AUTO;

		criAtomEx_Initialize_WASAPI(&config, nullptr, 0);

		if (!criAtomEx_IsInitialized())
		{
			return false;
		}

		/// [JP] 保存してある設定を読み、ACF ファイルが変わっていたら新しいものにする
		/// [EN] Load the saved settings and switch to the ACF file if it changed
		Load();

		Bool masterDirty = false;
		if (!masterPath_.view().empty())
		{
			std::ifstream masterStream(masterPath_.c_str(), std::ios::binary | std::ios::ate);
			if (masterStream)
			{
				DynamicArray<Byte> masterData(static_cast<Size>(masterStream.tellg()));
				masterStream.seekg(0);
				masterStream.read(masterData.data(), static_cast<std::streamsize>(masterData.size()));

				if (masterStream && !masterData.empty() && (masterData.size() != acfData_.size() || std::memcmp(masterData.data(), acfData_.data(), masterData.size()) != 0))
				{
					acfData_ = std::move(masterData);
					masterDirty = true;
				}
			}
			else
			{
				SC_LOG_WARNING("CriManager: ACFファイルを開けませんでした ({})", masterPath_.str());
			}
		}

		/// [JP] ACF を登録して、種類ごとの音量を当てはめる
		/// [EN] Register the ACF and apply each category's volume
		if (!acfData_.empty())
		{
			if (criAtomEx_RegisterAcfData(acfData_.data(), static_cast<CriSint32>(acfData_.size()), nullptr, 0) == CRI_TRUE)
			{
				CriSint32 categoryCount = criAtomExAcf_GetNumCategories();
				for (CriSint32 categoryIndex = 0; categoryIndex < categoryCount; ++categoryIndex)
				{
					CriAtomExCategoryInfo categoryInfo{};
					if (criAtomExAcf_GetCategoryInfo(static_cast<CriUint16>(categoryIndex), &categoryInfo) != CRI_TRUE || !categoryInfo.name)
					{
						continue;
					}

					String categoryName(categoryInfo.name);
					categoryNames_.push_back(categoryName);
					defaultCategoryVolumes_.insert({ categoryName, categoryInfo.volume });

					Float volume = categoryVolumes_.contains(categoryName) ? categoryVolumes_.at(categoryName) : categoryInfo.volume;
					criAtomExCategory_SetVolumeByName(categoryName.c_str(), volume);
				}

				criAtomExAsr_SetBusVolume(0, masterVolume_);

				/// [JP] ACF が変わっていたら保存し直す
				/// [EN] Save again if the ACF changed
				if (masterDirty)
				{
					Save();
				}
			}
			else
			{
				SC_LOG_ERROR("CriManager: ACFの登録に失敗しました");
				acfData_.clear();
			}
		}

		/// [JP] WAV を鳴らすための枠を用意する
		/// [EN] Reserve voices for WAVs
		CriAtomExWaveVoicePoolConfig waveConfig{};
		criAtomExVoicePool_SetDefaultConfigForWaveVoicePool(&waveConfig);
		waveConfig.num_voices = 128;
		waveConfig.player_config.max_channels = 6;
		waveConfig.player_config.max_sampling_rate = 96000;
		waveConfig.player_config.streaming_flag = CRI_TRUE;

		waveVoicePool_ = criAtomExVoicePool_AllocateWaveVoicePool(&waveConfig, nullptr, 0);
		if (!waveVoicePool_)
		{
			SC_LOG_ERROR("CriManager: Waveボイスプールの確保に失敗しました");
			return false;
		}

		/// [JP] キューシートを鳴らすための枠を用意する
		/// [EN] Reserve voices for cue sheets
		CriAtomExStandardVoicePoolConfig standardConfig{};
		criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&standardConfig);
		standardConfig.num_voices = 128;
		standardConfig.player_config.max_channels = 6;
		standardConfig.player_config.max_sampling_rate = 96000;
		standardConfig.player_config.streaming_flag = CRI_TRUE;

		standardVoicePool_ = criAtomExVoicePool_AllocateStandardVoicePool(&standardConfig, nullptr, 0);
		if (!standardVoicePool_)
		{
			SC_LOG_ERROR("CriManager: Standardボイスプールの確保に失敗しました");
			return false;
		}

		/// [JP] 長い音を少しずつ読む仕組みを作る
		/// [EN] Create the system that streams long sounds
		CriAtomDbasConfig dbasConfig{};
		criAtomDbas_SetDefaultConfig(&dbasConfig);
		dbasConfig.max_streams = 8;
		dbasConfig.max_bps = 12288000;
		dbasConfig.max_mana_streams = 0;
		dbasConfig.max_mana_bps = 0;

		dbasID_ = criAtomExDbas_Create(&dbasConfig, nullptr, 0);
		if (dbasID_ == CRIATOMEXDBAS_ILLEGAL_ID)
		{
			SC_LOG_ERROR("CriManager: D-BASの作成に失敗しました");
			return false;
		}

		return true;
	}

	/**
	* [JP]
	* 中身を作る。
	*
	* @brief 中身を作る
	*
	* [EN]
	* Makes the inner parts.
	*
	* @brief Makes the inner parts
	*/
	CriManager::CriManager() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	CriManager::~CriManager() = default;

	/**
	* [JP]
	* ゲーム用に始める。ACF はファイルから読まず、保存してあるものを使う。
	*
	* @brief  ゲーム用に始める
	* @param  gameConfig ゲームの設定
	* @return うまくいったら true
	*
	* [EN]
	* Starts for the game; it uses the saved ACF instead of reading the file.
	*
	* @brief  Starts for the game
	* @param  gameConfig The game settings
	* @return True if it worked
	*/
	Bool CriManager::Initialize(const GameConfig& gameConfig)
	{
		/// [JP] 始められたら、どこからでも使えるように登録する
		/// [EN] Once started, register it so anything can use it
		if (!impl_->Initialize(gameConfig))
		{
			return false;
		}

		Gateway::BindCriManager(this);
		return true;
	}

	/**
	* [JP]
	* エディタ用に始める。設定にある ACF ファイルを読む。
	*
	* @brief  エディタ用に始める
	* @param  editorConfig エディタの設定
	* @return うまくいったら true
	*
	* [EN]
	* Starts for the editor; it reads the ACF file named in the settings.
	*
	* @brief  Starts for the editor
	* @param  editorConfig The editor settings
	* @return True if it worked
	*/
	Bool CriManager::Initialize(const EditorConfig& editorConfig)
	{
		if (!impl_->Initialize(editorConfig))
		{
			return false;
		}

		Gateway::BindCriManager(this);
		return true;
	}

	/**
	* [JP]
	* 音のライブラリを1フレーム分動かす。
	*
	* @brief 1フレーム分動かす
	*
	* [EN]
	* Runs the sound library for one frame.
	*
	* @brief Runs one frame
	*/
	void CriManager::Execute()
	{
		impl_->Execute();
	}

	/**
	* [JP]
	* 音のライブラリを終わらせる。
	*
	* @brief 終わらせる
	*
	* [EN]
	* Shuts the sound library down.
	*
	* @brief Shuts down
	*/
	void CriManager::Finalize()
	{
		impl_->Finalize();
	}

	/**
	* [JP]
	* 全体の音量を変える。
	*
	* @brief 全体の音量を変える
	* @param volume 音量
	*
	* [EN]
	* Changes the overall volume.
	*
	* @brief Changes the overall volume
	* @param volume The volume
	*/
	void CriManager::MasterVolume(Float volume)
	{
		impl_->MasterVolume(volume);
	}

	/**
	* [JP]
	* 全体の音量を返す。
	*
	* @brief  全体の音量を返す
	* @return 全体の音量
	*
	* [EN]
	* Returns the overall volume.
	*
	* @brief  Returns the overall volume
	* @return The overall volume
	*/
	Float CriManager::MasterVolume()const
	{
		return impl_->MasterVolume();
	}

	/**
	* [JP]
	* 元に戻すときに使う、最初の全体の音量を決める。
	*
	* @brief 最初の全体の音量を決める
	* @param volume 最初の音量
	*
	* [EN]
	* Sets the starting overall volume used when resetting.
	*
	* @brief Sets the default overall volume
	* @param volume The starting volume
	*/
	void CriManager::DefaultMasterVolume(Float volume)
	{
		impl_->DefaultMasterVolume(volume);
	}

	/**
	* [JP]
	* 最初の全体の音量を返す。
	*
	* @brief  最初の全体の音量を返す
	* @return 最初の音量
	*
	* [EN]
	* Returns the starting overall volume.
	*
	* @brief  Returns the default overall volume
	* @return The starting volume
	*/
	Float CriManager::DefaultMasterVolume()const
	{
		return impl_->DefaultMasterVolume();
	}

	/**
	* [JP]
	* 種類ごとの音量を変える。
	*
	* @brief 種類ごとの音量を変える
	* @param category 種類の名前
	* @param volume   音量
	*
	* [EN]
	* Changes the volume of one category.
	*
	* @brief Changes a category volume
	* @param category The category name
	* @param volume   The volume
	*/
	void CriManager::CategoryVolume(const String& category, Float volume)
	{
		impl_->CategoryVolume(category, volume);
	}

	/**
	* [JP]
	* 種類ごとの音量を返す。変えていなければ最初の音量を返す。
	*
	* @brief  種類ごとの音量を返す
	* @param  category 種類の名前
	* @return その種類の音量
	*
	* [EN]
	* Returns the volume of one category, or its starting volume if never changed.
	*
	* @brief  Returns a category volume
	* @param  category The category name
	* @return That category's volume
	*/
	Float CriManager::CategoryVolume(const String& category)const
	{
		return impl_->CategoryVolume(category);
	}

	/**
	* [JP]
	* 音の種類の名前の一覧を返す。
	*
	* @brief  種類の一覧を返す
	* @return 種類の名前の一覧
	*
	* [EN]
	* Returns the list of category names.
	*
	* @brief  Returns the category list
	* @return The category names
	*/
	const DynamicArray<String>& CriManager::CategoryNameList()const
	{
		return impl_->CategoryNameList();
	}

	/**
	* [JP]
	* 音量を全部、最初の値に戻す。
	*
	* @brief 音量を元に戻す
	*
	* [EN]
	* Puts every volume back to its starting value.
	*
	* @brief Resets the volumes
	*/
	void CriManager::ResetVolume()
	{
		impl_->ResetVolume();
	}

	/**
	* [JP]
	* 保存してある ACF と音量をファイルから読む。
	*
	* @brief  保存した設定を読む
	* @return うまくいったら true
	*
	* [EN]
	* Reads the saved ACF and volumes from a file.
	*
	* @brief  Loads the saved settings
	* @return True if it worked
	*/
	Bool CriManager::Load()
	{
		return impl_->Load();
	}

	/**
	* [JP]
	* ACF と音量をファイルに保存する。
	*
	* @brief  設定を保存する
	* @return うまくいったら true
	*
	* [EN]
	* Saves the ACF and volumes to a file.
	*
	* @brief  Saves the settings
	* @return True if it worked
	*/
	Bool CriManager::Save()
	{
		return impl_->Save();
	}
}

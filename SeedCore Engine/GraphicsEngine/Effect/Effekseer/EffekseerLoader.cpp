#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerCache.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	namespace
	{
		/// [EN] Mirrors Effekseer's internal PathHelper::Combine exactly (append a
		///      separator if path1 doesn't already end in one, then normalise every
		///      backslash in the result to a forward slash) so the keys this loader
		///      computes match the paths Effekseer itself passes to FileInterface::OpenRead.
		/// [JP] Effekseer内部のPathHelper::Combineを正確に再現する(path1が区切り文字で
		///      終わっていなければ追加し、結果全体のバックスラッシュをスラッシュへ正規化)。
		///      これによりこのローダーが計算するキーが、Effekseer自身が
		///      FileInterface::OpenReadへ渡すパスと一致する。
		std::u16string CombinePath(const std::u16string& path1, const std::u16string& path2)
		{
			std::u16string result = path1;
			if (!result.empty() && result.back() != u'/' && result.back() != u'\\')
			{
				result += u'/';
			}
			result += path2;

			for (Char16& c : result)
			{
				if (c == u'\\')
				{
					c = u'/';
				}
			}

			return result;
		}

		/// [EN] Reads a dependency's bytes from disk and registers it (dedup by
		///      full path) into the collector, unless the path is empty.
		/// [JP] 依存関係のバイト列をディスクから読み込み、collectorへ登録する
		///      (フルパスで重複排除)。パスが空なら何もしない。
		void CollectDependency(const std::u16string& materialPath, const Char16* relativePath, FlatMap<std::u16string, DynamicArray<Byte>>& collector)
		{
			if (relativePath == nullptr || relativePath[0] == u'\0')
			{
				return;
			}

			std::u16string fullPath = CombinePath(materialPath, relativePath);
			if (collector.contains(fullPath))
			{
				return;
			}

			std::filesystem::path diskPath(fullPath);
			if (!std::filesystem::exists(diskPath))
			{
				return;
			}

			std::ifstream ifs(diskPath, std::ios::binary);
			DynamicArray<Byte> bytes((std::istreambuf_iterator<Char>(ifs)), std::istreambuf_iterator<Char>());
			collector.insert({ fullPath, std::move(bytes) });
		}

		/// [EN] Enumerates every texture/model/material/wave/curve an already-created
		///      effect references and collects their bytes, deduplicated by full path.
		/// [JP] 既に生成済みのエフェクトが参照する全テクスチャ/モデル/マテリアル/
		///      wave/curveを列挙し、フルパスで重複排除しながらバイト列を集める。
		FlatMap<std::u16string, DynamicArray<Byte>> CollectDependencies(const Effekseer::EffectRef& effect, const std::u16string& materialPath)
		{
			FlatMap<std::u16string, DynamicArray<Byte>> collector;

			for (Int32 index = 0; index < effect->GetColorImageCount(); index++)
			{
				CollectDependency(materialPath, effect->GetColorImagePath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetNormalImageCount(); index++)
			{
				CollectDependency(materialPath, effect->GetNormalImagePath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetDistortionImageCount(); index++)
			{
				CollectDependency(materialPath, effect->GetDistortionImagePath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetWaveCount(); index++)
			{
				CollectDependency(materialPath, effect->GetWavePath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetModelCount(); index++)
			{
				CollectDependency(materialPath, effect->GetModelPath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetMaterialCount(); index++)
			{
				CollectDependency(materialPath, effect->GetMaterialPath(index), collector);
			}
			for (Int32 index = 0; index < effect->GetCurveCount(); index++)
			{
				CollectDependency(materialPath, effect->GetCurvePath(index), collector);
			}

			return collector;
		}
	}

	/**
	* [EN]
	* Creates an effect from the ".effekseer" cache or ".efkefc" source
	* at filePath and returns its handle, or a null handle when it cannot
	* be read or created. A cache registers its bundled files with the
	* manager's file interface first. Nothing is written. thread is
	* unused: the effect needs no per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の ".effekseer" キャッシュか ".efkefc" ソースからエフェクトを
	* 作り、そのハンドルを返す。読めない、または作れない場合は null の
	* ハンドルを返す。キャッシュは、同梱したファイルを先にマネージャの
	* ファイルインターフェースへ登録する。何も書き出さない。thread は
	* 使わない（エフェクトはスレッドごとの状態を持たないため）。
	*/
	Handle<EffekseerEffect> EffekseerLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		std::filesystem::path cachePath = path;
		cachePath.replace_extension(".effekseer");

		std::u16string materialPath = path.parent_path().u16string();
		for (Char16& c : materialPath)
		{
			if (c == u'\\')
			{
				c = u'/';
			}
		}
		if (!materialPath.empty() && materialPath.back() != u'/')
		{
			materialPath += u'/';
		}

		EffekseerManager& manager = Gateway::GetEffekseerManager();
		Effekseer::EffectRef effect;

		if (path.extension() == ".effekseer" && std::filesystem::exists(cachePath))
		{
			BinaryInputArchive archive;
			if (archive.Read(String(cachePath.string())))
			{
				EffekseerCache cache;
				cache.Serialize(archive);

				for (EffekseerDependency& dependency : cache.dependencies_)
				{
					manager.GetFileInterface().Register(dependency.path_, std::move(dependency.data_));
				}

				effect = Effekseer::Effect::Create(manager.GetManager(), cache.data_.data(), static_cast<Int32>(cache.data_.size()), 1.0f, materialPath.c_str());
			}
			else
			{
				SC_LOG_WARNING("Effekseerキャッシュの読み込みに失敗しました。ソースから再生成します: {}", filePath.c_str());
				effect = nullptr;
			}
		}

		if (effect == nullptr)
		{
			if (path.extension() != ".efkefc")
			{
				SC_LOG_ERROR("Effekseerエフェクトのロードに失敗しました。拡張子が.efkefc/.effekseerではありません: {}", filePath.c_str());
				return Handle<EffekseerEffect>::null();
			}
			if (!std::filesystem::exists(path))
			{
				SC_LOG_ERROR("Effekseerエフェクトのロードに失敗しました。ファイルが存在しません: {}", filePath.c_str());
				return Handle<EffekseerEffect>::null();
			}

			std::ifstream ifs(path, std::ios::binary);
			DynamicArray<Byte> bytes((std::istreambuf_iterator<Char>(ifs)), std::istreambuf_iterator<Char>());
			if (bytes.empty())
			{
				SC_LOG_ERROR("Effekseerエフェクトのロードに失敗しました。ファイルの読み込みに失敗、または空です: {}", filePath.c_str());
				return Handle<EffekseerEffect>::null();
			}

			effect = Effekseer::Effect::Create(manager.GetManager(), bytes.data(), static_cast<Int32>(bytes.size()), 1.0f, materialPath.c_str());
			if (effect == nullptr)
			{
				SC_LOG_ERROR("Effekseer::Effect::Createに失敗しました: {} (materialPath: {})", filePath.c_str(), std::filesystem::path(materialPath).string());
				return Handle<EffekseerEffect>::null();
			}
		}

		Handle<EffekseerEffect> handle = pool_.Create();
		EffekseerEffect* slot = pool_.Get(handle);
		if (!slot)
		{
			return Handle<EffekseerEffect>::null();
		}

		slot->effect_ = effect;
		return handle;
	}

	/**
	* [EN]
	* Destroys the effect handle names.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle が示すエフェクトを破棄する。
	*/
	void EffekseerLoader::Clear(Handle<EffekseerEffect>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Creates the effect from the ".efkefc" source at sourcePath to learn
	* which files it reads, then writes the source and every one of
	* those files into the ".effekseer" cache at cachePath. Returns false
	* when the source cannot be read or created, or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* sourcePath の ".efkefc" ソースからエフェクトを作って、それが読む
	* ファイルを調べ、ソースとそれらの全ファイルを cachePath の
	* ".effekseer" キャッシュへ書く。ソースを読めない、作れない、または
	* 書き込みに失敗した場合は false を返す。
	*/
	Bool EffekseerLoader::Bake(String sourcePath, String cachePath)
	{
		std::filesystem::path path(sourcePath.c_str());
		if (path.extension() != ".efkefc" || !std::filesystem::exists(path))
		{
			return false;
		}

		/// [EN] The effect resolves the files it reads relative to its own folder, so that folder is its material path.
		/// [JP] エフェクトは読むファイルを自分のフォルダからの相対で解決するため、そのフォルダをマテリアルのパスにする。
		std::u16string materialPath = path.parent_path().u16string();
		for (Char16& c : materialPath)
		{
			if (c == u'\\')
			{
				c = u'/';
			}
		}
		if (!materialPath.empty() && materialPath.back() != u'/')
		{
			materialPath += u'/';
		}

		std::ifstream ifs(path, std::ios::binary);
		DynamicArray<Byte> bytes((std::istreambuf_iterator<Char>(ifs)), std::istreambuf_iterator<Char>());
		if (bytes.empty())
		{
			return false;
		}

		/// [EN] Only a created effect can tell which files it reads, so it is created here just to be asked.
		/// [JP] どのファイルを読むかは作ったエフェクトにしか分からないため、尋ねるためだけにここで作る。
		Effekseer::EffectRef effect = Effekseer::Effect::Create(Gateway::GetEffekseerManager().GetManager(), bytes.data(), static_cast<Int32>(bytes.size()), 1.0f, materialPath.c_str());
		if (effect == nullptr)
		{
			return false;
		}

		FlatMap<std::u16string, DynamicArray<Byte>> collected = CollectDependencies(effect, materialPath);

		EffekseerCache cache;
		cache.data_ = bytes;
		for (auto& [dependencyPath, dependencyData] : collected)
		{
			EffekseerDependency dependency;
			dependency.path_ = dependencyPath;
			dependency.data_ = dependencyData;
			cache.dependencies_.push_back(std::move(dependency));
		}

		BinaryOutputArchive archive;
		cache.Serialize(archive);
		return archive.Write(cachePath);
	}
}

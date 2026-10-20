#pragma once
#include <FoundationEngine/Utility/ResourcePtr.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>

namespace SeedCore
{
	class ResourceCache;
	class D3D12CommandQueue;
	class BC7CompressShader;

	/// [EN] Identifies which kind of asset a given AssetRecord entry represents, driving both file-extension classification during Scan and which resource manager handles loading/unloading it.
	/// [JP] 各 AssetRecord エントリがどの種類のアセットを表すかを識別する。Scan 時のファイル拡張子分類と、読み込み/解放を担当するリソースマネージャの両方を決定する。
	enum class AssetType
	{
		/// [EN] Image texture (.png/.jpg/.dds/...).
		/// [JP] 画像テクスチャ（.png/.jpg/.dds/...）。
		Texture,

		/// [EN] 3D model (.gltf/.glb/.fbx/.crister).
		/// [JP] 3Dモデル（.gltf/.glb/.fbx/.crister）。
		Model,

		/// [EN] Particle/visual effect (.efkefc/.effekseer/.zephyr).
		/// [JP] パーティクル/ビジュアルエフェクト（.efkefc/.effekseer/.zephyr）。
		Effect,

		/// [EN] Audio (.wav/.mp3/.acb/.awb/.audio).
		/// [JP] オーディオ（.wav/.mp3/.acb/.awb/.audio）。
		Audio,

		/// [EN] Font (.ttf/.otf/.ttc).
		/// [JP] フォント（.ttf/.otf/.ttc）。
		Font,

		/// [EN] Movie (.mp4/.movie).
		/// [JP] ムービー（.mp4/.movie）。
		Movie,

		/// [EN] Animation clip (.animation).
		/// [JP] アニメーションクリップ（.animation）。
		Animation,

		/// [EN] Actor prefab (.prefab).
		/// [JP] Actor プレハブ（.prefab）。
		Prefab,

		/// [EN] Scene (.scene).
		/// [JP] シーン（.scene）。
		Scene,

		/// [EN] Skybox/environment map (.hdr/.skymap).
		/// [JP] スカイボックス/環境マップ（.hdr/.skymap）。
		Skymap,

		/// [EN] Baked mesh collision geometry (.collision).
		/// [JP] 焼き込み済みの衝突ジオメトリ（.collision）。
		MeshCollision,

		/// [EN] Standalone material (.material).
		/// [JP] 単体マテリアル（.material）。
		Material,

		/// [EN] Standalone skeleton rig (.skeleton): sockets + root bone.
		/// [JP] 単体スケルトンリグ（.skeleton）: ソケット + ルートボーン。
		Skeleton,

		/// [EN] Extension not recognized by Scan.
		/// [JP] Scan が認識しない拡張子。
		Unknown,
	};

	/**
	* [EN]
	* A single discovered project asset: its project-relative and
	* absolute filesystem paths, classified type, stable GUID-derived
	* ID, and whether it is currently loaded into a resource manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 発見された単一のプロジェクトアセット: プロジェクト相対パスと
	* 絶対ファイルシステムパス、分類された種類、GUID から導出された
	* 安定的な ID、および現在リソースマネージャへ読み込まれているか。
	*/
	struct AssetRecord
	{
		/// [EN] Path relative to the project root, with forward slashes.
		/// [JP] プロジェクトルートからの相対パス（スラッシュ区切り）。
		String path_;

		/// [EN] Absolute filesystem path, with forward slashes.
		/// [JP] 絶対ファイルシステムパス（スラッシュ区切り）。
		String fullpath_;

		/// [EN] The classified asset type.
		/// [JP] 分類済みのアセット種別。
		AssetType type_ = AssetType::Unknown;

		/// [EN] Stable ID derived from the asset's .meta GUID (or a path hash for extensions that skip .meta files).
		/// [JP] アセットの .meta GUID から導出される安定的な ID（.meta ファイルを持たない拡張子の場合はパスハッシュ）。
		Uint32 assetID_ = 0;

		/// [EN] Whether this asset is currently loaded into its resource manager.
		/// [JP] このアセットが現在、対応するリソースマネージャへ読み込まれているかどうか。
		Bool isLoaded_ = false;
	};

	/**
	* [EN]
	* Serializable sidecar metadata stored in an asset's .meta file:
	* primarily its stable GUID, which survives file moves/renames
	* (unlike a path-derived hash).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセットの .meta ファイルに格納される、シリアライズ可能な
	* サイドカーメタデータ。主にその安定的な GUID を保持し、これは
	* （パス由来のハッシュとは異なり）ファイルの移動/リネームを経ても
	* 変化しない。
	*/
	struct AssetMeta
	{
		/// [EN] Meta file format version, for future migration.
		/// [JP] メタファイルフォーマットのバージョン。将来のマイグレーション用。
		Uint32 version_ = 2;

		/// [EN] Stable identifier for this asset, persisted across file moves/renames.
		/// [JP] このアセットの安定的な識別子。ファイルの移動/リネームを跨いで永続化される。
		Uint32 guid_ = 0;

		/// [EN] Axis convention of this Model asset: for .gltf/.glb it is applied on (re)import, for a .crister-only asset it records what is baked into the file.
		///      Unused for other asset types.
		/// [JP] この Model アセットの軸の規約。.gltf/.glb では(再)インポート時に適用し、.crister のみのアセットではファイルに焼き込まれた規約を記録する。
		///      Model 以外のアセットでは使わない。
		AxisConvention axisConvention_;

		Matrix modelTransform_ = Matrix::Identity;

		/**
		* [EN]
		* Serialization hook: reads/writes version_, guid_, axisConvention_,
		* and modelTransform_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック: version_、guid_、axisConvention_、
		* modelTransform_ を読み書きする。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("version", version_);
			archive.Field("guid", guid_);
			archive.TryField("axis_convention", axisConvention_);
			archive.TryField("model_transform", modelTransform_);
		}
	};

	/**
	* [EN]
	* Everything a resource manager may need to load or release one
	* asset, gathered into a single argument so the Asset interface is
	* the same for every asset type. A manager reads only the fields
	* its own type needs; the rest are null on paths that don't have
	* them (a release pass carries no device/queue/shader, for example).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* リソースマネージャがアセット1件を読み込む/解放するのに必要となり得る
	* ものを1つの引数へまとめたもの。これにより Asset インターフェースは
	* 全アセット種別で同一の形になる。各マネージャは自分の種別が必要とする
	* フィールドだけを読む。それ以外は、持っていない経路では null になる
	* （例えば解放パスは device/queue/shader を運ばない）。
	*/
	struct AssetContext
	{
		/// [EN] Cache owning the asset table, for looking up the record behind an asset ID.
		/// [JP] アセットテーブルを所有するキャッシュ。アセット ID に対応するレコードを引くために使う。
		ResourceCache& cache_;

		/// [EN] Device used to create GPU resources. Null on paths that create none.
		/// [JP] GPU リソースを生成するためのデバイス。生成を行わない経路では null。
		ID3D12Device* device_ = nullptr;

		/// [EN] Queue used to submit upload work. Null on paths that submit none.
		/// [JP] アップロード処理を提出するためのキュー。提出を行わない経路では null。
		D3D12CommandQueue* cmdQueue_ = nullptr;

		/// [EN] BC7 compression shader used when importing textures. Null on paths that import none.
		/// [JP] テクスチャのインポート時に使う BC7 圧縮シェーダ。インポートを行わない経路では null。
		BC7CompressShader* bc7Shader_ = nullptr;
	};

	/**
	* [EN]
	* The interface every resource manager implements so ResourceCache
	* can drive loading and releasing without naming a single concrete
	* manager: the cache looks a manager up by AssetType and calls these.
	* Managers keep their own typed Load/Unload overloads for callers
	* that already hold the concrete manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全リソースマネージャが実装するインターフェース。これにより
	* ResourceCache は具体的なマネージャを一切名指しせずに読み込みと解放を
	* 駆動できる（キャッシュは AssetType でマネージャを引き、これらを呼ぶ）。
	* 各マネージャは、具体型を既に持っている呼び出し側のために、自分の
	* 型付き Load/Unload オーバーロードも併せ持つ。
	*/
	class SEEDCORE_API Asset
	{
	public:
		/**
		* [EN]
		* Virtual so a manager owned through an Asset-typed ResourcePtr
		* is destroyed as its concrete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Asset 型の ResourcePtr で所有されているマネージャが、具体型として
		* 破棄されるようにするため virtual にしている。
		*/
		virtual ~Asset() = default;

		/**
		* [EN]
		* Loads the asset registered under assetId into this manager.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetId に登録されているアセットを、このマネージャへ読み込む。
		*/
		virtual void Load(const AssetContext& context, Uint32 assetID) = 0;

		/**
		* [EN]
		* Releases the asset registered under assetId from this manager.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetId に登録されているアセットを、このマネージャから解放する。
		*/
		virtual void Unload(Uint32 assetID) = 0;

		/**
		* [EN]
		* Releases whatever this manager still holds after a per-asset
		* Unload pass. Defaults to doing nothing, for managers whose
		* Unload already leaves nothing behind.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット単位の Unload パスの後に、このマネージャがまだ保持して
		* いるものを解放する。既定では何もしない（Unload の時点で何も
		* 残らないマネージャ向け）。
		*/
		virtual void Clear();
	};
}

#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace tinygltf
{
	class Model;
	struct Animation;
}

namespace fbxsdk
{
	class FbxAnimStack;
	class FbxNode;
}

namespace SeedCore
{
	/**
	* [EN]
	* Loads ".animation" clip files into Animation objects. Splitting a
	* model's embedded clips into ".animation" files and writing an edited
	* clip are private and reached only by AnimationResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".animation" クリップファイルを Animation へ読み込む。モデルに埋め込まれた
	* クリップを ".animation" ファイルへ分割する処理と、編集したクリップの
	* 書き出しは private で、AnimationResource からだけ使う。
	*/
	class SEEDCORE_API AnimationLoader :public AssetLoader<Animation>
	{
	private:
		friend class AnimationResource;

	public:
		AnimationLoader() = default;
		~AnimationLoader() = default;

		/**
		* [EN]
		* Reads the ".animation" file at filePath into a new Animation and
		* returns its handle, or a null handle when the file is missing, has
		* another extension or cannot be read. thread is unused: a clip needs
		* no per-thread state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の ".animation" ファイルを新しい Animation へ読み込み、
		* そのハンドルを返す。ファイルが無い、拡張子が違う、読めない場合は
		* null のハンドルを返す。thread は使わない（クリップはスレッドごとの
		* 状態を持たないため）。
		*/
		Handle<Animation> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Destroys the clip handle names.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すクリップを破棄する。
		*/
		void Clear(Handle<Animation>& handle)noexcept;

	private:
		/**
		* [EN]
		* Serializes animation to filePath as a ".animation" file
		* (overwriting any existing one). Returns false when the write
		* failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* animation を ".animation" ファイルとして filePath へ直列化する
		* （既存があれば上書き）。書き込みに失敗した場合は false を返す。
		*/
		Bool Bake(const Animation& animation, String filePath);

		/**
		* [EN]
		* Splits every clip embedded in a model source file into its own
		* "<model dir>/<model stem>.Animations/<clip name>.animation" file,
		* appending the full path of each file written to writtenPaths.
		* glTF/GLB writes one file per glTF animation; FBX writes one file per
		* animation stack (take). axisConvention is the model asset's own
		* convention from its .meta, so FBX clips land in the same space as the
		* Crister nodes they drive. Other extensions write nothing. Returns
		* false when the source cannot be read or a write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルのソースファイルに埋め込まれた全クリップを、それぞれ
		* "<モデルのディレクトリ>/<モデル名>.Animations/<クリップ名>.animation"
		* へ個別に書き出し、書いた各ファイルのフルパスを writtenPaths へ追加する。
		* glTF/GLB は glTF アニメーション1つにつき1ファイル、
		* FBX はアニメーションスタック（テイク）1つにつき1ファイル。
		* axisConvention はモデルアセット自身の .meta の座標系規約で、FBX の
		* クリップを、それが動かす Crister のノードと同じ空間に揃えるために使う。
		* それ以外の拡張子は何も書き出さない。ソースを読めない、または書き
		* 込みに失敗した場合は false を返す。
		*/
		Bool SplitClips(String filePath, const AxisConvention& axisConvention, DynamicArray<String>& writtenPaths);

		void FetchAnimationClip(const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation, Animation& animation);

		/**
		* [EN]
		* Bakes one FBX animation stack into animation by sampling every
		* animated node's local transform once per frame at frameRate.
		* nodeTable is the depth-first node order ModelLoader uses for the
		* Crister's nodes_, so each channel is keyed by the same node index the
		* Crister uses. Only the T/R/S channels that actually carry keys in one
		* of the stack's layers are written; the rest fall back to the rest
		* pose at playback. The stack must already be the scene's current one.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* FBX のアニメーションスタック1つを、アニメーションするノードの
		* ローカルトランスフォームを frameRate で1フレームごとにサンプリング
		* して animation へ焼き込む。nodeTable は ModelLoader が Crister の
		* nodes_ に使うのと同じ深さ優先のノード順で、各チャンネルは Crister と
		* 同じノード番号をキーにする。スタックのいずれかのレイヤーで実際に
		* キーを持つ T/R/S チャンネルだけを書き出し、それ以外は再生時に
		* レストポーズへフォールバックする。スタックは事前にシーンの現在の
		* スタックにしておくこと。
		*/
		void FetchAnimationClip(fbxsdk::FbxAnimStack* fbxStack, const DynamicArray<fbxsdk::FbxNode*>& nodeTable, Float unitScale, const ResolvedAxisConvention& resolved, Double frameRate, Animation& animation);

	};
}

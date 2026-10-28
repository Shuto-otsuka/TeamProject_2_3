#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
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
	class SEEDCORE_API AnimationLoader :public NonCopyable
	{
	public:
		AnimationLoader() = default;
		~AnimationLoader() = default;

		Handle<Animation> Load(String filePath);

		Bool Save(Animation& animation, const std::filesystem::path& filePath);

		Animation* Get(const Handle<Animation>& handle);

		void Clear(Handle<Animation>& handle)noexcept;

		/**
		* [EN]
		* Splits every clip embedded in a model source file into its own
		* "<model dir>/<model stem>.Animations/<clip name>.animation" file.
		* glTF/GLB writes one file per glTF animation; FBX writes one file per
		* animation stack (take). axisConvention is the model asset's own
		* convention from its .meta, so FBX clips land in the same space as the
		* Crister nodes they drive. Other extensions write nothing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルのソースファイルに埋め込まれた全クリップを、それぞれ
		* "<モデルのディレクトリ>/<モデル名>.Animations/<クリップ名>.animation"
		* へ個別に書き出す。glTF/GLB は glTF アニメーション1つにつき1ファイル、
		* FBX はアニメーションスタック（テイク）1つにつき1ファイル。
		* axisConvention はモデルアセット自身の .meta の座標系規約で、FBX の
		* クリップを、それが動かす Crister のノードと同じ空間に揃えるために使う。
		* それ以外の拡張子は何も書き出さない。
		*/
		void SplitClips(String filePath, const AxisConvention& axisConvention);

	private:
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

	private:
		StablePool<Animation> pool_;
	};
}

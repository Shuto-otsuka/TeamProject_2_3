#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	/**
	* [EN]
	* The GPU side of this Crister: its buffers, bindless indices and
	* geometry/texture streaming state. Functions that read the CPU
	* data as well take this Crister as an argument.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister の GPU 側。バッファ、bindless インデックス、
	* ジオメトリ/テクスチャのストリーミング状態を持つ。CPU 側のデータも
	* 読む関数は、この Crister を引数で受け取る。
	*/
	CristerResidency& Crister::Residency()
	{
		return residency_;
	}

	/**
	* [EN]
	* Read-only access to the GPU side of this Crister.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister の GPU 側への読み取り専用のアクセス。
	*/
	const CristerResidency& Crister::Residency()const
	{
		return residency_;
	}

	/**
	* [EN]
	* Returns every Stage (root-node list + name) parsed from the source
	* glTF's scenes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF の scenes から解析された、全 Stage（ルートノード一覧 +
	* 名前）を返す。
	*/
	std::span<const Stage> Crister::Stages()const
	{
		return scene_.stages_; 
	}

	/**
	* [EN]
	* Returns every Node in the flattened node hierarchy, including their
	* local S/R/T and cumulated globalTransform_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 平坦化されたノード階層の全 Node を返す。各ノードのローカル
	* S/R/T と、累積済みの globalTransform_ を含む。
	*/
	std::span<const Node> Crister::Nodes()const
	{
		return scene_.nodes_;
	}

	/**
	* [EN]
	* Returns every KHR_lights_punctual point/spot light resolved from
	* the source glTF, in world space.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解決された、全 KHR_lights_punctual
	* ポイント/スポットライトをワールド空間で返す。
	*/
	std::span<const PunctualLight> Crister::Lights()const
	{
		return scene_.lights_;
	}

	/**
	* [EN]
	* Returns every SubMesh (material + cluster range + optional skin
	* index) this Crister is split into.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Crister が分割されている全 SubMesh（マテリアル + クラスタ
	* 範囲 + 任意のスキンインデックス）を返す。
	*/
	std::span<const SubMesh> Crister::SubMeshes()const
	{
		return geometry_.subMeshes_;
	}

	/**
	* [EN]
	* Returns every Surface (PBR factors + KHR_materials_* extensions +
	* texture indices) parsed from the source glTF.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解析された、全 Surface（PBR ファクタ +
	* KHR_materials_* 拡張 + テクスチャインデックス）を返す。
	*/
	std::span<const Surface> Crister::Surfaces()const
	{
		return material_.surfaces_;
	}

	/**
	* [EN]
	* Returns every embedded Bitmap, indexed by the texture indices in
	* Surface. Each holds the BC7 mip chain baked by
	* CristerCompressor::BakeBitmap().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全ての内蔵 Bitmap を返す。Surface のテクスチャインデックスで
	* 引く。それぞれ CristerCompressor::BakeBitmap() が焼いた BC7 の
	* ミップチェーンを持つ。
	*/
	std::span<const Bitmap> Crister::Bitmaps()const
	{
		return material_.bitmaps_;
	}

	/**
	* [EN]
	* Returns every Skin (joint list + inverse-bind matrices) parsed
	* from the source glTF.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソース glTF から解析された、全 Skin（ジョイント一覧 + 逆バインド
	* 行列）を返す。
	*/
	std::span<const Skin> Crister::Skins()const
	{
		return scene_.skins_;
	}

	/**
	* [EN]
	* Returns every Cluster (one LOD level's meshlet range within a
	* SubMesh) across all SubMeshes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全 SubMesh を通した、全 Cluster（SubMesh 内の1 LOD レベルぶんの
	* meshlet 範囲）を返す。
	*/
	std::span<const Cluster> Crister::Clusters()const
	{
		return geometry_.clusters_;
	}

	/**
	* [EN]
	* Returns the index into Stages() of the stage rendered by default
	* when no stage is explicitly selected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 明示的にステージが選択されていない時にデフォルトで描画される、
	* Stages() 内のインデックスを返す。
	*/
	Int Crister::DefaultStage()const
	{
		return scene_.defaultStage_;
	}

	/**
	* [EN]
	* Linear search for the first Node whose name_ matches name, or -1
	* if none does. glTF node names are not guaranteed unique, so this
	* returns the first match in nodes_ order.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* name_ が name と一致する最初の Node を線形探索する、無ければ -1。
	* glTF のノード名は一意性が保証されないため、nodes_ の順で最初に
	* 見つかったものを返す。
	*/
	Int Crister::FindNodeIndex(const std::string& name)const
	{
		for (Size nodeIndex = 0; nodeIndex < scene_.nodes_.size(); nodeIndex++)
		{
			if (scene_.nodes_[nodeIndex].name_ == name)
			{
				return static_cast<Int>(nodeIndex);
			}
		}
		return -1;
	}

	/**
	* [EN]
	* Minimum corner of the dequantisation AABB for CompressedVertex
	* positions (see the struct comment). Passed to the shaders through
	* ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex 位置の逆量子化 AABB の最小コーナー（構造体コメント
	* 参照）。ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector3 Crister::PositionMin()const
	{
		return geometry_.positionMin_;
	}

	/**
	* [EN]
	* Extent (max - min) of the dequantisation AABB for CompressedVertex
	* positions. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex 位置の逆量子化 AABB の大きさ（max - min）。
	* ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector3 Crister::PositionExtent()const
	{
		return geometry_.positionExtent_;
	}

	/**
	* [EN]
	* Minimum corner of the dequantisation AABB for CompressedVertex
	* texcoords. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex テクスチャ座標の逆量子化 AABB の最小コーナー。
	* ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector2 Crister::TexcoordMin()const
	{
		return geometry_.texcoordMin_;
	}

	/**
	* [EN]
	* Extent (max - min) of the dequantisation AABB for CompressedVertex
	* texcoords. Passed to the shaders through ModelStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CompressedVertex テクスチャ座標の逆量子化 AABB の大きさ
	* （max - min）。ModelStructuredBuffer 経由でシェーダに渡す。
	*/
	Vector2 Crister::TexcoordExtent()const
	{
		return geometry_.texcoordExtent_;
	}

	/**
	* [EN]
	* Walks stages_[defaultStage_]'s node tree depth-first and
	* recomputes every Node::globalTransform_ from local S/R/T. Used by
	* ModelLoader while importing and by CristerConverter after a
	* conversion.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* stages_[defaultStage_]のノードツリーを深さ優先で走査し、ローカル
	* S/R/T から全 Node::globalTransform_ を再計算する。ModelLoader が
	* インポート中に、CristerConverter が変換の後に使う。
	*/
	void Crister::CumulateTransforms()
	{
		/// [EN] Without a valid default stage there is no root to start from.
		/// [JP] 正しいデフォルトステージが無ければ、始めるルートが無い。
		if (scene_.defaultStage_ < 0 || static_cast<Size>(scene_.defaultStage_) >= scene_.stages_.size())
		{
			return;
		}

		/// [EN] The top of the stack is the global transform of the parent of the node being visited.
		/// [JP] スタックの一番上が、訪れているノードの親のグローバル変換。
		std::stack<Matrix> parentGlobalTransforms;
		std::function<void(Int)> traverse = [&](Int nodeIndex)
			{
				if (nodeIndex < 0 || static_cast<Size>(nodeIndex) >= scene_.nodes_.size())
				{
					return;
				}

				Node& node = scene_.nodes_[nodeIndex];
				Matrix scale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix rotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix translation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				/// [EN] Local S/R/T in row-vector order, then onto the parent's global transform.
				/// [JP] ローカルの S/R/T を行ベクトルの順で掛け、親のグローバル変換に重ねる。
				Matrix localTransform = scale * rotation * translation;
				node.globalTransform_ = localTransform * parentGlobalTransforms.top();

				/// [EN] Each child sees this node's global transform as its parent's.
				/// [JP] 各子は、このノードのグローバル変換を親のものとして受け取る。
				for (Int childIndex : node.children_)
				{
					parentGlobalTransforms.push(node.globalTransform_);
					traverse(childIndex);
					parentGlobalTransforms.pop();
				}
			};

		/// [EN] Every root starts under the identity.
		/// [JP] 各ルートは単位行列の下から始める。
		for (Int nodeIndex : scene_.stages_[scene_.defaultStage_].nodes_)
		{
			parentGlobalTransforms.push(Matrix::Identity);
			traverse(nodeIndex);
			parentGlobalTransforms.pop();
		}
	}
}

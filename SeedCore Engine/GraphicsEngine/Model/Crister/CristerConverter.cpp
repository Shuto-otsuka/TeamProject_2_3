#include <GraphicsEngine/Model/Crister/CristerConverter.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/Model/Crister/CristerQuantizer.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Re-converts an already-baked Crister (no source glTF required) from
	* one axis convention to another, in place: decodes every quantised
	* vertex/skin back to full precision, applies deltaBasis to
	* positions/normals/tangents/node transforms/skin inverse-bind
	* matrices/light positions-directions/meshlet bounds, optionally
	* reverses triangle winding, recomputes the quantisation AABBs from
	* the transformed data, re-quantises via BakeMesh(), and
	* re-serialises the result to cristerPath. Does NOT touch GPU
	* resources or bindless indices — the caller must force a reload
	* (Unload then Load) afterward. Returns false if this Crister has no
	* compressed vertex data to convert (e.g. textures-only/degenerate asset).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 既に焼き込み済みの Crister（ソース glTF 不要）を、ある軸コンベンション
	* から別のものへその場で再変換する: 量子化済みの頂点/スキンをフル精度へ
	* デコードし、位置/法線/タンジェント/ノードトランスフォーム/スキン
	* 逆バインド行列/ライト位置・向き/メシュレット境界へ deltaBasis を
	* 適用、必要なら三角形の巻き順を反転、変換後データから量子化 AABB を
	* 再計算し、BakeMesh() で再量子化して cristerPath へ再シリアライズ
	* する。GPU リソースや bindless インデックスは触らない —
	* 呼び出し側が後で強制リロード（Unload → Load）すること。この
	* Crister に変換対象の量子化済み頂点データが無い場合（テクスチャのみ
	* 等の縮退アセット）は false を返す。
	*/
	Bool CristerConverter::ApplyAxisConversion(Crister& crister, const Matrix& deltaBasis, Bool flipWinding, const std::filesystem::path& cristerPath)const
	{
		if (crister.geometry_.compressedVertices_.empty())
		{
			return false;
		}

		/// [EN] A basis with a negative determinant mirrors space, which flips the handedness of every tangent frame.
		/// [JP] 行列式が負の基底は空間を鏡映するので、全ての接線フレームの利き手が反転する。
		Float determinant = deltaBasis.Determinant();
		Bool isMirror = determinant < 0.0f;
		Float tangentSign = isMirror ? -1.0f : 1.0f;

		/// [EN] Decode every quantised vertex back to full precision, then transform position/normal/tangent in place. Texcoord is unaffected by an axis-convention change.
		/// [JP] 量子化済みの全頂点をフル精度へデコードし、位置/法線/タンジェントをその場で変換する。テクスチャ座標は軸コンベンション変更の影響を受けない。
		DynamicArray<Vertex> transformedVertices(crister.geometry_.compressedVertices_.size());
		for (Size vertexIndex = 0; vertexIndex < crister.geometry_.compressedVertices_.size(); vertexIndex++)
		{
			const CompressedVertex& compressed = crister.geometry_.compressedVertices_[vertexIndex];
			Vertex& vertex = transformedVertices[vertexIndex];

			Vertex decoded = crister.DecodeVertex(compressed);
			vertex.position_ = Vector3::Transform(decoded.position_, deltaBasis);
			vertex.texcoord_ = decoded.texcoord_;
			vertex.normal_ = Vector3::Transform(decoded.normal_, deltaBasis);

			Vector3 tangentXyz = Vector3::Transform(Vector3(decoded.tangent_.x, decoded.tangent_.y, decoded.tangent_.z), deltaBasis);
			vertex.tangent_ = Vector4(tangentXyz.x, tangentXyz.y, tangentXyz.z, decoded.tangent_.w * tangentSign);

			/// [EN] Skin attributes do not depend on the axes; they are only decoded so BakeMesh() can re-encode them.
			/// [JP] スキン属性は軸に依存しない。BakeMesh() で符号化し直せるようデコードするだけ。
			if (!crister.geometry_.compressedSkinVertices_.empty())
			{
				crister.DecodeSkin(crister.geometry_.compressedSkinVertices_[vertexIndex], vertex.joints_, vertex.weights_);
			}
		}

		/// [EN] Convert every node's local rotation and translation, then rebuild the global transforms from them.
		/// [JP] 全ノードのローカル回転と平行移動を変換し、そこからグローバル変換を作り直す。
		for (Node& node : crister.scene_.nodes_)
		{
			ConvertRotationByBasis(node.rotation_, deltaBasis);
			ConvertPositionByBasis(node.translation_, deltaBasis);
		}
		crister.CumulateTransforms();

		/// [EN] Inverse bind matrices live in the same model space as the vertices, so they change basis too.
		/// [JP] 逆バインド行列は頂点と同じモデル空間にあるので、これも基底を変える。
		for (Skin& skin : crister.scene_.skins_)
		{
			for (Matrix& inverseBindMatrix : skin.inverseBindMatrices_)
			{
				ConvertMatrixByBasis(inverseBindMatrix, deltaBasis);
			}
		}

		/// [EN] Lights move with the model: position and direction change basis, and the direction is renormalised.
		/// [JP] ライトもモデルと一緒に動く。位置と向きの基底を変え、向きは正規化し直す。
		for (PunctualLight& light : crister.scene_.lights_)
		{
			ConvertPositionByBasis(light.position_, deltaBasis);
			ConvertPositionByBasis(light.direction_, deltaBasis);
			light.direction_.Normalize();
		}

		/// [EN] Meshlet culling bounds keep matching the geometry: the sphere center and the normal cone axis change basis.
		/// [JP] meshlet のカリング用の境界もジオメトリに合わせる。球の中心と法線コーンの軸の基底を変える。
		for (MeshletBound& bound : crister.geometry_.meshletBounds_)
		{
			ConvertPositionByBasis(bound.center_, deltaBasis);
			ConvertPositionByBasis(bound.coneAxis_, deltaBasis);
		}

		/// [EN] Swapping two corners of every triangle reverses its winding, keeping front faces outward after a mirror.
		/// [JP] 全三角形の2つの角を入れ替えると巻き順が逆になり、鏡映の後も表面が外を向いたままになる。
		if (flipWinding)
		{
			Size triangleCount = crister.geometry_.primitiveIndices_.size() / 3;
			for (Size triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
			{
				std::swap(crister.geometry_.primitiveIndices_[triangleIndex * 3 + 1], crister.geometry_.primitiveIndices_[triangleIndex * 3 + 2]);
			}
		}

		/// [EN] Reuse BakeMesh()'s existing quantisation pipeline verbatim: it recomputes positionMin_/positionExtent_/texcoordMin_/texcoordExtent_ from vertices_ and re-populates compressedVertices_/compressedSkinVertices_.
		/// [JP] BakeMesh() の既存の量子化パイプラインをそのまま再利用する: vertices_ から positionMin_/positionExtent_/texcoordMin_/texcoordExtent_ を再計算し、compressedVertices_/compressedSkinVertices_ を再構築する。
		crister.geometry_.vertices_ = std::move(transformedVertices);
		CristerQuantizer quantizer;
		quantizer.BakeMesh(crister);

		/// [EN] Write the converted model back over its .crister cache.
		/// [JP] 変換したモデルを .crister キャッシュへ上書きする。
		BinaryOutputArchive archive;
		crister.Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Applies a global position/rotation(euler degrees)/scale/pivot
	* transform to this Crister's node hierarchy and light
	* positions-directions, then re-serialises to cristerPath. Vertices,
	* skin inverse-bind matrices and meshlet bounds stay in mesh-local
	* space: static SubMeshes are placed by their node's global transform
	* at draw time (SubMeshPlacement) and skinned ones through their
	* joints, so both pick the transform up from the nodes.
	* scale/pivot/rotation compose about pivot first, position is a
	* separate world-space offset applied after. Only root-level nodes
	* (stages_[defaultStage_].nodes_) have their local transform
	* updated - CumulateTransforms() then propagates to every
	* descendant, since post-multiplying the whole transform onto just
	* the root telescopes correctly through the local-transform chain
	* (node.globalTransform_ = local * parentGlobal). Returns false if
	* this Crister has no compressed vertex data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グローバルな位置/回転(オイラー角、度)/スケール/ピボット変換を
	* この Crister のノード階層とライト位置・向きへ適用し、その後
	* cristerPath へ再シリアライズする。頂点・スキン逆バインド行列・
	* メシュレット境界はメッシュローカル空間のまま: 静的 SubMesh は描画時に
	* ノードのグローバル変換で配置され(SubMeshPlacement)、スキン付きは
	* ジョイント経由なので、どちらもノードから変換を受け取る。
	* スケール/ピボット/回転はまずピボットを中心に合成し、position は
	* その後に適用する独立したワールド空間オフセット。ローカル
	* トランスフォームを更新するのはルートノード
	* (stages_[defaultStage_].nodes_)のみ — CumulateTransforms() が
	* 全子孫へ伝播する。ルートだけに変換全体を後乗せすれば、ローカル
	* トランスフォームの連鎖(node.globalTransform_ = local *
	* parentGlobal)を通じて正しく telescope するため。この Crister に
	* 変換対象の量子化済み頂点データが無い場合は false を返す。
	*/
	Bool CristerConverter::ApplyTransformConversion(Crister& crister, Vector3 position, Vector3 rotation, Vector3 scale, Vector3 pivot, const std::filesystem::path& cristerPath)const
	{
		if (crister.geometry_.compressedVertices_.empty())
		{
			return false;
		}

		/// [EN] Away-from-zero clamp: a zero/near-zero axis would make linearBasis singular, and the root nodes' S/R/T could no longer be decomposed back out of the composed transform.
		/// [JP] ゼロから離す方向へのクランプ: 軸が 0/0 近傍だと linearBasis が特異になり、合成した変換からルートノードの S/R/T を分解し直せなくなる。
		Vector3 clampedScale(
			std::abs(scale.x) < 0.0001f ? std::copysign(0.0001f, scale.x) : scale.x,
			std::abs(scale.y) < 0.0001f ? std::copysign(0.0001f, scale.y) : scale.y,
			std::abs(scale.z) < 0.0001f ? std::copysign(0.0001f, scale.z) : scale.z);

		/// [EN] Scale then rotate (Euler degrees, as yaw Y / pitch X / roll Z) about the pivot, then offset by position in world space.
		/// [JP] ピボットを中心にスケールしてから回転し(オイラー角は度、ヨー Y / ピッチ X / ロール Z)、その後ワールド空間で position だけずらす。
		Matrix linearBasis = Matrix::CreateScale(clampedScale.x, clampedScale.y, clampedScale.z) * Matrix::CreateFromYawPitchRoll(ToRadians(rotation.y), ToRadians(rotation.x), ToRadians(rotation.z));
		Matrix fullTransform = Matrix::CreateTranslation(-pivot) * linearBasis * Matrix::CreateTranslation(pivot + position);

		/// [EN] Apply the transform to the root nodes of the default stage only; every descendant inherits it through its parent.
		/// [JP] 変換はデフォルトステージのルートノードにだけかける。子孫は全て親を通して受け継ぐ。
		if (crister.scene_.defaultStage_ >= 0 && static_cast<Size>(crister.scene_.defaultStage_) < crister.scene_.stages_.size())
		{
			for (Int rootNodeIndex : crister.scene_.stages_[crister.scene_.defaultStage_].nodes_)
			{
				if (rootNodeIndex < 0 || static_cast<Size>(rootNodeIndex) >= crister.scene_.nodes_.size())
				{
					continue;
				}

				Node& node = crister.scene_.nodes_[rootNodeIndex];
				Matrix localScale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix localRotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix localTranslation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				/// [EN] Post-multiply the root's local S/R/T by the transform and decompose the result back into S/R/T.
				/// [JP] ルートのローカル S/R/T に変換を後ろから掛け、結果を S/R/T へ分解し直す。
				Matrix newLocal = localScale * localRotation * localTranslation * fullTransform;

				newLocal.Decompose(node.scale_, node.rotation_, node.translation_);
			}
		}

		/// [EN] Propagate the new root transforms down to every node's global transform.
		/// [JP] 新しいルートの変換を、全ノードのグローバル変換へ伝える。
		crister.CumulateTransforms();

		/// [EN] Lights are stored resolved, so they take the transform directly: the full transform for the position, only the linear part for the direction.
		/// [JP] ライトは解決済みの値で持つので、変換を直接かける。位置には全体の変換を、向きには線形部分だけをかける。
		for (PunctualLight& light : crister.scene_.lights_)
		{
			light.position_ = Vector3::Transform(light.position_, fullTransform);
			light.direction_ = Vector3::TransformNormal(light.direction_, linearBasis);
			light.direction_.Normalize();
		}

		/// [EN] Write the converted model back over its .crister cache.
		/// [JP] 変換したモデルを .crister キャッシュへ上書きする。
		BinaryOutputArchive archive;
		crister.Serialize(archive);
		if (!archive.Write(String(cristerPath.string())))
		{
			return false;
		}

		return true;
	}

	/**
	* [EN]
	* Converts every node/vertex/skin in crister from its source axis
	* convention into the engine's fixed LH/Y-up/Z-forward space, using
	* the basis resolved from axisConvention. Falls back to the default
	* AxisConvention if axisConvention resolves to an invalid basis
	* (never silently skips conversion).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* crister 内の全ノード/頂点/スキンを、axisConvention から解決された
	* 基底を用いて、ソースの軸コンベンションからエンジン固定の
	* LH/Y-up/Z-forward 空間へ変換する。axisConvention が無効な基底に
	* 解決された場合はデフォルトの AxisConvention にフォールバックする
	* （変換を黙ってスキップすることはない）。
	*/
	void CristerConverter::ConvertAxisConvention(Crister& crister, const AxisConvention& axisConvention)const
	{
		ResolvedAxisConvention resolved = ResolvedAxisConvention::Resolve(axisConvention);
		if (!resolved.valid_)
		{
			resolved = ResolvedAxisConvention::Resolve(AxisConvention{});
		}

		for (Node& node : crister.scene_.nodes_)
		{
			ConvertRotationByBasis(node.rotation_, resolved.basis_);
			ConvertPositionByBasis(node.translation_, resolved.basis_);
		}
		crister.CumulateTransforms();

		Float tangentSign = resolved.isMirror_ ? -1.0f : 1.0f;
		for (Vertex& vertex : crister.geometry_.vertices_)
		{
			ConvertPositionByBasis(vertex.position_, resolved.basis_);
			ConvertPositionByBasis(vertex.normal_, resolved.basis_);
			ConvertPositionByBasis(vertex.tangent_, resolved.basis_);
			vertex.tangent_.w *= tangentSign;
		}

		for (Skin& skin : crister.scene_.skins_)
		{
			for (Matrix& inverseBindMatrix : skin.inverseBindMatrices_)
			{
				ConvertMatrixByBasis(inverseBindMatrix, resolved.basis_);
			}
		}

		if (resolved.flipWinding_)
		{
			FlipTriangleWinding(crister);
		}
	}

	/**
	* [EN]
	* Change-of-basis for a position: transforms vector in place by basis.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 位置の基底変換。vector を basis でその場変換する。
	*/
	void CristerConverter::ConvertPositionByBasis(Vector3& vector, const Matrix& basis)const
	{
		vector = Vector3::Transform(vector, basis);
	}

	/**
	* [EN]
	* Transforms the .xyz of a Vector4 (e.g. tangent) by basis. .w
	* (handedness sign) is intentionally left untouched here - the caller
	* is responsible for flipping it when the basis is a mirror.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Vector4（タンジェント等）の .xyz を basis で変換する。.w（利き手符号）
	* はここでは意図的に触らない — 基底がミラーの場合の符号反転は
	* 呼び出し側の責任とする。
	*/
	void CristerConverter::ConvertPositionByBasis(Vector4& vector, const Matrix& basis)const
	{
		Vector3 xyz = Vector3::Transform(Vector3(vector.x, vector.y, vector.z), basis);
		vector.x = xyz.x;
		vector.y = xyz.y;
		vector.z = xyz.z;
	}

	/**
	* [EN]
	* Change-of-basis for a rotation: transforms quaternion in place by
	* basis.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 回転の基底変換。quaternion を basis でその場変換する。
	*/
	void CristerConverter::ConvertRotationByBasis(Quaternion& quaternion, const Matrix& basis)const
	{
		/// [EN] Conjugate the rotation by the orthonormal basis (inverse = transpose) so it rotates the same way in the new axes.
		/// [JP] 正規直交な基底(逆行列 = 転置)で回転を挟み、新しい軸でも同じように回るようにする。
		Matrix rotation = Matrix::CreateFromQuaternion(quaternion);
		Matrix converted = basis.Transpose() * rotation * basis;
		quaternion = Quaternion::CreateFromRotationMatrix(converted);
	}

	/**
	* [EN]
	* Change-of-basis for a matrix: transforms matrix in place by basis.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 行列の基底変換。matrix を basis でその場変換する。
	*/
	void CristerConverter::ConvertMatrixByBasis(Matrix& matrix, const Matrix& basis)const
	{
		/// [EN] Conjugate by the orthonormal basis, the same change of basis the rotations get.
		/// [JP] 回転と同じ基底変換として、正規直交な基底で挟む。
		matrix = basis.Transpose() * matrix * basis;
	}

	/**
	* [EN]
	* Reverses triangle winding by swapping the last two corners of every
	* triangle in crister.vertexIndices_ (the flat, pre-meshlet global
	* index list populated by FetchMeshes). Only invoked when the
	* resolved axis conversion explicitly requires a winding flip.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* crister.vertexIndices_（FetchMeshes が生成する、メシュレット化前の
	* フラットなグローバルインデックス列）の全三角形について後ろ2頂点を
	* 入れ替え、巻き順を反転する。解決済みの軸変換が巻き順反転を
	* 明示的に要求する時のみ呼ばれる。
	*/
	void CristerConverter::FlipTriangleWinding(Crister& crister)const
	{
		Size triangleCount = crister.geometry_.vertexIndices_.size() / 3;
		for (Size triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
		{
			std::swap(crister.geometry_.vertexIndices_[triangleIndex * 3 + 1], crister.geometry_.vertexIndices_[triangleIndex * 3 + 2]);
		}
	}
}
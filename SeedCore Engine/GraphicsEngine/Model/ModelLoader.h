#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <GraphicsEngine/Model/Crister/Crister.h>

namespace tinygltf
{
	class Model;
}

namespace fbxsdk
{
	class FbxScene;
	class FbxNode;
	class FbxMesh;
	class FbxSurfaceMaterial;
	class FbxFileTexture;
	class FbxManager;
}

namespace SeedCore
{
	class BindlessHeap;
	class BC7CompressShader;
	class D3D12CommandQueue;

	enum class ModelFormat
	{
		Gltf,
		Fbx,
	};

	struct GltfSource
	{
		const tinygltf::Model* model_ = nullptr;
	};

	struct FbxSource
	{
		fbxsdk::FbxScene* scene_ = nullptr;
		std::string directory_;
		std::string stem_;
		Float unitScale_ = 1.0f;
		DynamicArray<fbxsdk::FbxNode*> nodeTable_;
		DynamicArray<fbxsdk::FbxMesh*> meshTable_;
		mutable DynamicArray<fbxsdk::FbxSurfaceMaterial*> materialTable_;
		mutable DynamicArray<fbxsdk::FbxFileTexture*> textureTable_;
	};

	struct ModelSource
	{
		ModelFormat format_ = ModelFormat::Gltf;
		GltfSource gltf_;
		FbxSource fbx_;
	};

	/**
	* [EN]
	* Loads glTF/GLB 3D models and converts them into the engine's internal
	* Crister format (meshlets + LOD hierarchy + serialisable cache).
	*
	* Responsibilities:
	*   - Parse glTF/GLB via tinygltf (or load from ".crister" binary cache)
	*   - Extract scenes, nodes, meshes, materials, skins, textures
	*   - Generate meshlets (64 vertices / 124 triangles per workgroup)
	*   - Build a multi-level LOD hierarchy using QEM simplification
	*   - Compute per-meshlet bounding spheres and normal cones
	*   - Serialise the result as a ".crister" cache for fast future loads
	*   - Trigger GPU upload via Crister::Upload
	*
	* Crister objects are pooled in a StablePool and accessed via Handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* glTF/GLB 3D モデルを読み込み、エンジン内部の Crister フォーマット
	* （メシュレット + LOD 階層 + シリアライズ可能キャッシュ）に変換する。
	*
	* 担当:
	*   - tinygltf で glTF/GLB を解析（または ".crister" バイナリキャッシュから読み込み）
	*   - シーン、ノード、メッシュ、マテリアル、スキン、テクスチャの抽出
	*   - メシュレット生成（ワークグループあたり 64 頂点 / 124 三角形）
	*   - QEM 簡略化によるマルチレベル LOD 階層の構築
	*   - メシュレットごとの包囲球と法線コーンの計算
	*   - 次回の高速ロードのため ".crister" キャッシュとしてシリアライズ
	*   - Crister::Upload による GPU アップロードの起動
	*
	* Crister オブジェクトは StablePool でプールされ、Handle でアクセスする。
	*/
	class ModelLoader :public NonCopyable
	{
	public:
		ModelLoader() = default;
		~ModelLoader() = default;

		/**
		* [EN]
		* Loads a 3D model from a glTF/GLB file path.
		* Returns a handle to the loaded Crister, or a null handle on failure.
		* If a ".crister" cache exists next to the source file, loads from
		* cache instead of re-parsing the glTF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* glTF/GLB ファイルパスから 3D モデルを読み込む。
		* 読み込まれた Crister へのハンドルを返す。失敗時は null ハンドル。
		* ソースファイルの隣に ".crister" キャッシュがあれば、glTF を再解析
		* せずキャッシュから読み込む。axisConvention は glTF を再解析する
		* 場合のみ適用される（キャッシュ読み込み時は無視される）。
		*/
		Handle<Crister> Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap, BC7CompressShader& bc7Shader, String filePath, const AxisConvention& axisConvention = AxisConvention{}, Bool forExport = false);

		/**
		* [EN]
		* Retrieves a Crister pointer from a handle. Returns nullptr if invalid.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ハンドルから Crister ポインタを取得する。無効なら nullptr を返す。
		*/
		Crister* Get(const Handle<Crister>& handle);

		/**
		* [EN]
		* Releases a Crister and returns the handle to the pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Crister を解放し、ハンドルをプールに返却する。
		*/
		void Clear(Handle<Crister>& handle)noexcept;

	private:
		/**
		* [EN]
		* Extracts scene definitions (root node lists + default scene) from glTF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* glTF からシーン定義（ルートノードリスト + デフォルトシーン）を抽出する。
		*/
		void FetchStages(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Extracts node hierarchy and transforms (S/R/T or matrix) from glTF.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* glTF からノード階層とトランスフォーム（S/R/T または行列）を抽出する。
		*/
		void FetchNodes(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Extracts vertex attributes and index buffers for all mesh primitives.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全メッシュプリミティブの頂点属性とインデックスバッファを抽出する。
		*/
		void FetchMeshes(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Extracts PBR materials and resolves texture → image indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* PBR マテリアルを抽出し、テクスチャ → イメージインデックスを解決する。
		*/
		void FetchMaterials(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Extracts skeleton data (joint lists + inverse bind matrices).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スケルトンデータ（ジョイントリスト + 逆バインド行列）を抽出する。
		*/
		void FetchSkins(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Resolves which skin (if any) each SubMesh belongs to, by walking the
		* glTF nodes that reference a mesh + skin pair.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* mesh + skin のペアを参照する glTF ノードを走査し、各 SubMesh が
		* どのスキンに属するか（属さないか）を解決する。
		*/
		void ResolveSubMeshSkins(const ModelSource& source, Crister& crister);

		/**
		* [EN]
		* Copies decoded image data from glTF into Crister's texture array.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* glTF からデコード済み画像データを Crister のテクスチャ配列にコピーする。
		*/
		void FetchTexture(const ModelSource& source, Crister& crister);

		void ConvertSceneConvention(fbxsdk::FbxScene* scene, fbxsdk::FbxManager* manager);

		void ConvertFlatConvention(ModelSource& source);

		/**
		* [EN]
		* Extracts KHR_lights_punctual point/spot lights, resolving each
		* referencing node's world position/direction. Must run after
		* ConvertAxisConvention (its own CumulateTransforms call) so
		* Node::globalTransform_ is already in engine (LH) space.
		* Directional lights are skipped: this engine's single directional
		* light is authored per-scene, not per-model.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* KHR_lights_punctual のポイント/スポットライトを抽出し、参照ノードの
		* ワールド位置/向きを解決する。ConvertAxisConvention（内部で
		* CumulateTransforms を再実行する）の後に呼ぶこと。Node::globalTransform_
		* が既にエンジン（左手系）空間になっている必要があるため。
		* ディレクショナルライトはスキップする: このエンジンの単一
		* ディレクショナルライトはモデル単位ではなくシーン単位で設定する。
		*/
		void FetchLights(const ModelSource& source, Crister& crister);

	private:
		/// [EN] Object pool for Crister instances. Provides stable pointers and handle-based access.
		/// [JP] Crister インスタンスのオブジェクトプール。安定したポインタとハンドルベースのアクセスを提供する。
		StablePool<Crister> pool_;
	};
}

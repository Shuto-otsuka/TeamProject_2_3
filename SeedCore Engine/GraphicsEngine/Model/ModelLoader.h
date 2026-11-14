#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
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
	class GraphicsProcessor;
	class D3D12Context;

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
	*   - Trigger GPU upload via CristerResidency::Upload
	*
	* Crister objects are pooled in a StablePool and accessed via Handle.
	* Writing the ".crister" cache is private and reached only by
	* ModelResource; parsing a source without baking it, for export, is
	* reached only by ModelExporter.
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
	*   - CristerResidency::Upload による GPU アップロードの起動
	*
	* Crister オブジェクトは StablePool でプールされ、Handle でアクセスする。
	* ".crister" キャッシュの書き出しは private で ModelResource からだけ、
	* 書き出し（エクスポート）用に焼き込まずにソースを解析する処理は
	* ModelExporter からだけ使う。
	*/
	class SEEDCORE_API ModelLoader :public AssetLoader<Crister>
	{
	private:
		friend class ModelResource;
		friend class ModelExporter;

	public:
		/**
		* [EN]
		* Takes the device context models are uploaded with, the bindless
		* heap their buffers and textures are registered in, and the GPU
		* processor that BC7-compresses their textures.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデルをアップロードするデバイスのコンテキスト、バッファと
		* テクスチャの登録先のバインドレスヒープ、テクスチャを BC7 圧縮する
		* GPU の加工器を受け取る。
		*/
		ModelLoader(D3D12Context& context, BindlessHeap& heap, GraphicsProcessor& processor);
		~ModelLoader() = default;

		/**
		* [EN]
		* Loads a 3D model from filePath and uploads it on thread's transfer
		* lane. Returns a handle to the loaded Crister, or a null handle on
		* failure. A ".crister" path is read as the baked cache; a source
		* (glTF/GLB/FBX) is parsed, converted with the axis convention in its
		* own ".meta", and baked in memory. Nothing is written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath から 3D モデルを読み込み、thread の転送レーンで
		* アップロードする。読み込まれた Crister へのハンドルを返す。失敗時は
		* null ハンドル。".crister" は焼き込み済みのキャッシュとして読む。
		* ソース（glTF/GLB/FBX）は解析し、自身の ".meta" にある座標系規約で
		* 変換して、メモリ上で焼き込む。何も書き出さない。
		*/
		Handle<Crister> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Releases a Crister's GPU residency and returns the handle to the
		* pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Crister の GPU 常駐分を解放し、ハンドルをプールに返却する。
		*/
		void Clear(Handle<Crister>& handle)noexcept;

	private:
		/**
		* [EN]
		* The body of Load. When forExport is set, a source stops right after
		* extraction: geometry, materials and images stay uncompressed and
		* LOD-free for ModelExporter, with no GPU upload; a ".crister" is
		* decoded back to full-precision geometry instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Load の本体。forExport のとき、ソースは抽出の直後で止め、ジオメトリ・
		* マテリアル・画像を非圧縮・LOD 無しのまま ModelExporter へ渡し、GPU
		* アップロードもしない。".crister" は代わりにフル精度のジオメトリへ
		* デコードし直す。
		*/
		Handle<Crister> Import(String filePath, LoadThread thread, Bool forExport);

		/**
		* [EN]
		* Serializes crister to filePath as a ".crister" cache (overwriting
		* any existing one). Returns false when the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* crister を ".crister" キャッシュとして filePath へ直列化する
		* （既存があれば上書き）。書き込みに失敗した場合は false を返す。
		*/
		Bool Bake(Crister& crister, String filePath);

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
		/// [EN] Device context models are uploaded with.
		/// [JP] モデルをアップロードするデバイスのコンテキスト。
		D3D12Context& context_;

		/// [EN] Bindless heap the models' buffers and textures are registered in.
		/// [JP] モデルのバッファとテクスチャが登録されるバインドレスヒープ。
		BindlessHeap& heap_;

		/// [EN] GPU processor that BC7-compresses the models' textures.
		/// [JP] モデルのテクスチャを BC7 圧縮する GPU の加工器。
		GraphicsProcessor& processor_;
	};
}

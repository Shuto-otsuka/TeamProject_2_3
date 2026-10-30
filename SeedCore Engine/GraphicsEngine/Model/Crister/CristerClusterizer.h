#pragma once

namespace SeedCore
{
	class Crister;

	/**
	* [EN]
	* Builds the meshlets and the multi-level LOD cluster hierarchy (QEM
	* simplification) of a Crister from its flat global index buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Crister のフラットなグローバルインデックスバッファから、meshlet と
	* マルチレベルの LOD クラスタ階層(QEM 簡略化)を作る。
	*/
	class SEEDCORE_API CristerClusterizer
	{
	public:
		/**
		* [EN]
		* Generates meshlets and multi-level LOD hierarchy (QEM + DAG clusters).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メシュレットとマルチレベル LOD 階層（QEM + DAG クラスター）を生成する。
		*/
		void BuildMeshlets(Crister& crister)const;
	};
}
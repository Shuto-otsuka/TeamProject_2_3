#pragma once
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	/**
	* [EN]
	* The interface every exporter implements: writing an asset out in an
	* external, standard format. It stands beside the loader (which reads
	* files) and the resource (which maps asset IDs and makes cache files),
	* so writing to another tool's format never mixes into either. The call
	* is the same for every asset type; anything type-specific, such as a
	* model's preset, is set on the exporter before Export is called.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全てのエクスポーターが実装するインターフェース。アセットを外部の
	* 標準的な形式で書き出す。ファイルを読むローダーと、アセット ID を
	* 対応付けてキャッシュファイルを作るリソースの横に並べ、他のツールの
	* 形式への書き出しがどちらにも混ざらないようにする。呼び出しは全
	* アセット種別で同じ形で、モデルのプリセットのような種別固有の設定は、
	* Export を呼ぶ前にエクスポーターへ設定しておく。
	*/
	class SEEDCORE_API AssetExporter :public NonCopyable
	{
	public:
		/**
		* [EN]
		* Virtual so an exporter owned through its base is destroyed as its
		* concrete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 基底型で所有されているエクスポーターが、具体型として破棄される
		* ようにするため virtual にしている。
		*/
		virtual ~AssetExporter() = default;

		/**
		* [EN]
		* Writes the asset registered under assetID to outputPath in this
		* exporter's format, using the settings made on the exporter
		* beforehand. Returns whether the export succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID で登録されているアセットを、事前にエクスポーターへした設定を
		* 使って、このエクスポーターの形式で outputPath へ書き出す。成功した
		* かどうかを返す。
		*/
		virtual Bool Export(const AssetContext& context, Uint32 assetID, String outputPath) = 0;
	};
}

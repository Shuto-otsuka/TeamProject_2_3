#pragma once

namespace SeedCore
{
	class World;
	class MovieResource;
	class BindlessHeap;

	class MovieSystem
	{
	public:
		void Update(MovieResource& movieResource, World& world);

		/**
		* [EN]
		* Uploads the frame each loaded video decoded since the last upload
		* into its GPU texture, recording the copies on cmdList. Runs after
		* Update and before the renderers consume the textures.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 読み込み済みの各動画が前回のアップロード以降にデコードした
		* フレームを、その GPU テクスチャへアップロードする。コピーは cmdList
		* に記録する。Update の後、レンダラーがテクスチャを使う前に動かす。
		*/
		void Upload(MovieResource& movieResource, ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap);
	};
}

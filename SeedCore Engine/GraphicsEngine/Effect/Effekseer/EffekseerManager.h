#pragma once
#include <GraphicsEngine/Effect/Effekseer/EffekseerFileInterface.h>

namespace SeedCore
{
	class EffekseerRenderer;

	class EffekseerManager :public NonCopyable
	{
	public:
		EffekseerManager() = default;
		~EffekseerManager() = default;

		Bool Initialize(EffekseerRenderer& renderer, Uint32 instanceMax = 8000);

		void Update(Float deltaTime);

		NODISCARD Effekseer::ManagerRef GetManager()const;

		NODISCARD Float GetTotalTime()const;

		/// [EN] Shared file interface used by the texture/model/material loaders;
		///      EffekseerLoader registers ".effekseer" cache dependencies into it.
		/// [JP] テクスチャ/モデル/マテリアルローダーが共有するファイルインターフェース。
		///      EffekseerLoaderが".effekseer"キャッシュの依存関係をここに登録する。
		NODISCARD EffekseerFileInterface& GetFileInterface();

	private:
		Effekseer::ManagerRef manager_;

		Effekseer::RefPtr<EffekseerFileInterface> fileInterface_;

		Float totalTime_ = 0.0f;
	};
}

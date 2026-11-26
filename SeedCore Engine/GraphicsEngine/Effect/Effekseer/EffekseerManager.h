#pragma once
#include <GraphicsEngine/Effect/Effekseer/EffekseerCacheLoader.h>

namespace SeedCore
{
	class D3D12Context;
	class D3D12CommandList;

	class EffekseerManager :public NonCopyable
	{
	public:
		EffekseerManager() = default;
		~EffekseerManager() = default;

		Bool Initialize(D3D12Context& context);

		void Execute(Float elapsedTime);

		void Finalize();

		void Draw(D3D12CommandList& cmdList, const Matrix& view, const Matrix& projection);

		Effekseer::EffectRef Create(const EffekseerAsset& asset);

		Effekseer::Handle Play(const Effekseer::EffectRef& effect, const Vector3& position);

		void Stop(Effekseer::Handle handle);

		NODISCARD Bool Playing(Effekseer::Handle handle);

		void Transform(Effekseer::Handle handle, const Matrix& matrix);

		void Speed(Effekseer::Handle handle, Float speed);

		void Color(Effekseer::Handle handle, const SeedCore::Color& color);

	public:
		NODISCARD Effekseer::ManagerRef GetManager()const;

		NODISCARD Float GetTotalTime()const;

	private:
		Effekseer::ManagerRef manager_;

		EffekseerRenderer::RendererRef renderer_;

		Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> memoryPool_;

		Effekseer::RefPtr<EffekseerRenderer::CommandList> cmdList_;

		Effekseer::SettingRef cacheSetting_;

		Effekseer::RefPtr<EffekseerTextureLoader> textureLoader_;

		Effekseer::RefPtr<EffekseerModelLoader> modelLoader_;

		Effekseer::RefPtr<EffekseerMaterialLoader> materialLoader_;

		Effekseer::RefPtr<EffekseerCurveLoader> curveLoader_;

		DynamicArray<Effekseer::Handle> handles_;

		Float totalTime_ = 0.0f;
	};
}

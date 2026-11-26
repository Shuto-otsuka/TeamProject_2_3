#pragma once
#include <GraphicsEngine/Effect/Effekseer/EffekseerAsset.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	class EffekseerTextureLoader :public Effekseer::TextureLoader 
	{
	public:
		EffekseerTextureLoader(Effekseer::Backend::GraphicsDeviceRef device);

		~EffekseerTextureLoader()override = default;

		Effekseer::TextureRef Create(const EffekseerTexture& texture, Effekseer::TextureType textureType);

		Effekseer::TextureRef Load(const Char16* path, Effekseer::TextureType textureType)override;

		void Unload(Effekseer::TextureRef data)override;

	private:
		Effekseer::Backend::GraphicsDeviceRef device_;
	};

	class EffekseerModelLoader :public Effekseer::ModelLoader
	{
	public:
		EffekseerModelLoader(Effekseer::Backend::GraphicsDeviceRef device);

		~EffekseerModelLoader()override = default;

		Effekseer::ModelRef Create(const EffekseerModel& model);

		Effekseer::ModelRef Load(const Char16* path)override;

		void Unload(Effekseer::ModelRef data)override;

	private:
		Effekseer::Backend::GraphicsDeviceRef device_;
	};

	class EffekseerMaterialLoader :public Effekseer::MaterialLoader
	{
	public:
		EffekseerMaterialLoader(Effekseer::MaterialLoaderRef loader);

		~EffekseerMaterialLoader()override = default;

		Effekseer::MaterialRef Create(const EffekseerMaterial& material);

		Effekseer::MaterialRef Load(const Char16* path)override;

		void Unload(Effekseer::MaterialRef data)override;

	private:
		Effekseer::MaterialLoaderRef loader_;
	};

	class EffekseerCurveLoader :public Effekseer::CurveLoader
	{
	public:
		EffekseerCurveLoader() = default;

		~EffekseerCurveLoader()override = default;

		Effekseer::CurveRef Create(const EffekseerCurve& curve);

		Effekseer::CurveRef Load(const Char16* path)override;

		void Unload(Effekseer::CurveRef data)override;
	};
}
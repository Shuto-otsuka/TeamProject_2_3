#include <GraphicsEngine/Effect/Effekseer/EffekseerCacheLoader.h>

namespace SeedCore
{
	EffekseerTextureLoader::EffekseerTextureLoader(Effekseer::Backend::GraphicsDeviceRef device) :device_(device)
	{
		/// No Code
	}

	Effekseer::TextureRef EffekseerTextureLoader::Create(const EffekseerTexture& texture, Effekseer::TextureType textureType)
	{
		const Bitmap& bitmap = texture.bitmap_;
		if (bitmap.cacheData_.empty())
		{
			return nullptr;
		}

		Effekseer::Backend::TextureParameter parameter;
		parameter.Format = textureType == Effekseer::TextureType::Color ? Effekseer::Backend::TextureFormatType::R8G8B8A8_UNORM_SRGB : Effekseer::Backend::TextureFormatType::R8G8B8A8_UNORM;
		parameter.Size = { bitmap.width_, bitmap.height_, 1 };
		parameter.MipLevelCount = bitmap.mipCount_;

		Effekseer::CustomVector<Uint8> initialData(bitmap.cacheData_.begin(), bitmap.cacheData_.end());

		Effekseer::TextureRef result = Effekseer::MakeRefPtr<Effekseer::Texture>();
		result->SetBackend(device_->CreateTexture(parameter, initialData));
		return result;
	}

	Effekseer::TextureRef EffekseerTextureLoader::Load(const Char16* path, Effekseer::TextureType textureType)
	{
		return nullptr;
	}

	void EffekseerTextureLoader::Unload(Effekseer::TextureRef data)
	{
		/// No Code
	}
}

namespace SeedCore
{
	EffekseerModelLoader::EffekseerModelLoader(Effekseer::Backend::GraphicsDeviceRef device) :device_(device)
	{
		/// No Code
	}

	Effekseer::ModelRef EffekseerModelLoader::Create(const EffekseerModel& model)
	{
		if (model.frames_.empty())
		{
			return nullptr;
		}

		const EffekseerFrame& frame = model.frames_.front();
		Effekseer::CustomVector<Effekseer::Model::Vertex> vertices(frame.vertices_.size() / sizeof(Effekseer::Model::Vertex));
		std::memcpy(vertices.data(), frame.vertices_.data(), frame.vertices_.size());
		Effekseer::CustomVector<Effekseer::Model::Face> faces(frame.faces_.size() / sizeof(Effekseer::Model::Face));
		std::memcpy(faces.data(), frame.faces_.data(), frame.faces_.size());

		Effekseer::ModelRef result = Effekseer::MakeRefPtr<Effekseer::Model>(vertices, faces);
		result->StoreBufferToGPU(device_.Get());
		return result;
	}

	Effekseer::ModelRef EffekseerModelLoader::Load(const Char16* path)
	{
		return nullptr;
	}

	void EffekseerModelLoader::Unload(Effekseer::ModelRef data)
	{
		/// No Code
	}
}

namespace SeedCore
{
	EffekseerMaterialLoader::EffekseerMaterialLoader(Effekseer::MaterialLoaderRef loader) :loader_(loader)
	{
		/// No Code
	}

	Effekseer::MaterialRef EffekseerMaterialLoader::Create(const EffekseerMaterial& material)
	{
		if (material.data_.empty())
		{
			return nullptr;
		}

		return loader_->Load(material.data_.data(), static_cast<Int32>(material.data_.size()), Effekseer::MaterialFileType::Compiled);
	}

	Effekseer::MaterialRef EffekseerMaterialLoader::Load(const Char16* path)
	{
		return nullptr;
	}

	void EffekseerMaterialLoader::Unload(Effekseer::MaterialRef data)
	{
		loader_->Unload(data);
	}
}

namespace SeedCore
{
	Effekseer::CurveRef EffekseerCurveLoader::Create(const EffekseerCurve& curve)
	{
		if (curve.data_.empty())
		{
			return nullptr;
		}

		return Effekseer::MakeRefPtr<Effekseer::Curve>(curve.data_.data(), static_cast<Int32>(curve.data_.size()));
	}

	Effekseer::CurveRef EffekseerCurveLoader::Load(const Char16* path)
	{
		return nullptr;
	}

	void EffekseerCurveLoader::Unload(Effekseer::CurveRef data)
	{
		/// No Code
	}
}
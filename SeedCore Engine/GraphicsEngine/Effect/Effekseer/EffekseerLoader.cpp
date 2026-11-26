#include <External/DirectXTex/Include/DirectXTex.h>
#include <External/Effekseer/Include/Effekseer/Material/Effekseer.CompiledMaterial.h>
#include <External/Effekseer/Include/EffekseerMaterialCompilerDX12/EffekseerMaterialCompilerDX12.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerAsset.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Handle<EffekseerEffect> EffekseerLoader::Load(String filePath, LoadThread thread)
	{
		Handle<EffekseerEffect> handle = pool_.Create();
		EffekseerEffect* effect = pool_.Get(handle);
		if (!effect)
		{
			return Handle<EffekseerEffect>::null();
		}

		std::string extension = std::filesystem::path(filePath.c_str()).extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		EffekseerAsset asset;
		Bool fetched = extension == ".effekseer" ? Read(filePath, asset) : Import(filePath, asset);
		if (!fetched || !Create(asset, thread, *effect))
		{
			pool_.Destroy(handle);
			return Handle<EffekseerEffect>::null();
		}

		return handle;
	}

	void EffekseerLoader::Clear(Handle<EffekseerEffect>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	Bool EffekseerLoader::Bake(String sourcePath, String cachePath)
	{
		EffekseerAsset asset;
		if (!Import(sourcePath, asset))
		{
			return false;
		}

		return Write(asset, cachePath);
	}

	Bool EffekseerLoader::Import(String sourcePath, EffekseerAsset& asset)
	{
		std::filesystem::path path(sourcePath.c_str());
		std::ifstream ifs(path, std::ios::binary);
		asset.data_.assign(std::istreambuf_iterator<Char>(ifs), std::istreambuf_iterator<Char>());
		if (asset.data_.empty())
		{
			SC_LOG_ERROR("Effekseerのソースを読み込めませんでした: {}", sourcePath.str());
			return false;
		}

		Effekseer::EffectRef source = Gateway::GetEffekseerManager().Create(asset);
		if (source == nullptr)
		{
			SC_LOG_ERROR("Effekseerのソースを解析できませんでした: {}", sourcePath.str());
			return false;
		}

		std::filesystem::path directory = path.parent_path();
		auto readFile = [&directory](const Char16* relativePath)
			{
				DynamicArray<Byte> fileBytes;
				if (relativePath == nullptr || relativePath[0] == u'\0')
				{
					return fileBytes;
				}

				std::filesystem::path diskPath = directory / std::filesystem::path(relativePath);
				std::ifstream file(diskPath, std::ios::binary);
				fileBytes.assign(std::istreambuf_iterator<Char>(file), std::istreambuf_iterator<Char>());
				if (fileBytes.empty())
				{
					SC_LOG_WARNING("Effekseerの依存ファイルを読み込めませんでした: {}", diskPath.string());
				}
				return fileBytes;
			};

		auto importTextures = [&readFile](DynamicArray<EffekseerTexture>& textures, Int32 count, auto pathAt)
			{
				for (Int32 index = 0; index < count; ++index)
				{
					Bitmap& bitmap = textures.emplace_back().bitmap_;
					const Char16* relativePath = pathAt(index);
					DynamicArray<Byte> fileBytes = readFile(relativePath);
					if (fileBytes.empty())
					{
						continue;
					}

					const Uint8* data = reinterpret_cast<const Uint8*>(fileBytes.data());
					DirectX::ScratchImage loadedImage;
					HRESULT hr = DirectX::LoadFromDDSMemory(data, fileBytes.size(), DirectX::DDS_FLAGS_NONE, nullptr, loadedImage);
					if (FAILED(hr))
					{
						hr = DirectX::LoadFromTGAMemory(data, fileBytes.size(), nullptr, loadedImage);
					}
					if (FAILED(hr))
					{
						hr = DirectX::LoadFromWICMemory(data, fileBytes.size(), DirectX::WIC_FLAGS_NONE, nullptr, loadedImage);
					}
					if (FAILED(hr))
					{
						SC_LOG_WARNING("Effekseerのテクスチャをデコードできませんでした: {}", std::filesystem::path(relativePath).string());
						continue;
					}

					DirectX::ScratchImage rgbaImage;
					const DirectX::Image* image = loadedImage.GetImage(0, 0, 0);
					DXGI_FORMAT sourceFormat = loadedImage.GetMetadata().format;
					if (sourceFormat != DXGI_FORMAT_R8G8B8A8_UNORM)
					{
						if (DirectX::IsCompressed(sourceFormat))
						{
							hr = DirectX::Decompress(*image, DXGI_FORMAT_R8G8B8A8_UNORM, rgbaImage);
						}
						else
						{
							hr = DirectX::Convert(*image, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgbaImage);
						}
						if (FAILED(hr))
						{
							SC_LOG_WARNING("Effekseerのテクスチャを RGBA8 に変換できませんでした: {}", std::filesystem::path(relativePath).string());
							continue;
						}
						image = rgbaImage.GetImage(0, 0, 0);
					}

					bitmap.name_ = std::filesystem::path(relativePath).stem().string();
					bitmap.width_ = static_cast<Int>(image->width);
					bitmap.height_ = static_cast<Int>(image->height);
					bitmap.component_ = 4;
					bitmap.bits_ = 8;
					bitmap.mipCount_ = Effekseer::TextureLoaderHelper::GetIsMipmapEnabled(std::u16string(relativePath)) ? 0 : 1;
					bitmap.cacheData_.resize(image->width * image->height * 4);
					for (Size row = 0; row < image->height; ++row)
					{
						std::memcpy(bitmap.cacheData_.data() + row * image->width * 4, image->pixels + row * image->rowPitch, image->width * 4);
					}
				}
			};

		importTextures(asset.colorTextures_, source->GetColorImageCount(), [&source](Int32 index) { return source->GetColorImagePath(index); });
		importTextures(asset.normalTextures_, source->GetNormalImageCount(), [&source](Int32 index) { return source->GetNormalImagePath(index); });
		importTextures(asset.distortionTextures_, source->GetDistortionImageCount(), [&source](Int32 index) { return source->GetDistortionImagePath(index); });

		for (Int32 index = 0; index < source->GetModelCount(); ++index)
		{
			EffekseerModel& imported = asset.models_.emplace_back();
			DynamicArray<Byte> fileBytes = readFile(source->GetModelPath(index));
			if (fileBytes.empty())
			{
				continue;
			}

			Effekseer::Model model(fileBytes.data(), static_cast<Int32>(fileBytes.size()));
			for (Int32 frameIndex = 0; frameIndex < model.GetFrameCount(); ++frameIndex)
			{
				EffekseerFrame& frame = imported.frames_.emplace_back();
				const Byte* vertices = reinterpret_cast<const Byte*>(model.GetVertexes(frameIndex));
				frame.vertices_.assign(vertices, vertices + static_cast<Size>(model.GetVertexCount(frameIndex)) * sizeof(Effekseer::Model::Vertex));
				const Byte* faces = reinterpret_cast<const Byte*>(model.GetFaces(frameIndex));
				frame.faces_.assign(faces, faces + static_cast<Size>(model.GetFaceCount(frameIndex)) * sizeof(Effekseer::Model::Face));
			}
		}

		Effekseer::RefPtr<Effekseer::MaterialCompilerDX12> compiler = Effekseer::MakeRefPtr<Effekseer::MaterialCompilerDX12>();
		for (Int32 index = 0; index < source->GetMaterialCount(); ++index)
		{
			EffekseerMaterial& imported = asset.materials_.emplace_back();
			DynamicArray<Byte> fileBytes = readFile(source->GetMaterialPath(index));
			if (fileBytes.empty())
			{
				continue;
			}

			Effekseer::MaterialFile materialFile;
			if (!materialFile.Load(reinterpret_cast<const uint8_t*>(fileBytes.data()), static_cast<Int32>(fileBytes.size())))
			{
				SC_LOG_WARNING("Effekseerのマテリアルを読み込めませんでした: {}", std::filesystem::path(source->GetMaterialPath(index)).string());
				continue;
			}

			Effekseer::CompiledMaterialBinary* binary = compiler->Compile(&materialFile);
			if (binary == nullptr)
			{
				SC_LOG_WARNING("Effekseerのマテリアルをコンパイルできませんでした: {}", std::filesystem::path(source->GetMaterialPath(index)).string());
				continue;
			}

			auto vertexShader = [binary](Effekseer::MaterialShaderType type)
				{
					return std::vector<Uint8>(binary->GetVertexShaderData(type), binary->GetVertexShaderData(type) + binary->GetVertexShaderSize(type));
				};
			auto pixelShader = [binary](Effekseer::MaterialShaderType type)
				{
					return std::vector<Uint8>(binary->GetPixelShaderData(type), binary->GetPixelShaderData(type) + binary->GetPixelShaderSize(type));
				};

			Effekseer::CompiledMaterial compiled;
			compiled.UpdateData(vertexShader(Effekseer::MaterialShaderType::Standard), pixelShader(Effekseer::MaterialShaderType::Standard), vertexShader(Effekseer::MaterialShaderType::Model), pixelShader(Effekseer::MaterialShaderType::Model), vertexShader(Effekseer::MaterialShaderType::Refraction), pixelShader(Effekseer::MaterialShaderType::Refraction), vertexShader(Effekseer::MaterialShaderType::RefractionModel), pixelShader(Effekseer::MaterialShaderType::RefractionModel), Effekseer::CompiledMaterialPlatformType::DirectX12);
			binary->Release();

			std::vector<Uint8> original(reinterpret_cast<const Uint8*>(fileBytes.data()), reinterpret_cast<const Uint8*>(fileBytes.data()) + fileBytes.size());
			std::vector<Uint8> compiledBytes;
			compiled.Save(compiledBytes, materialFile.GetGUID(), original);
			imported.data_.assign(reinterpret_cast<const Byte*>(compiledBytes.data()), reinterpret_cast<const Byte*>(compiledBytes.data()) + compiledBytes.size());
		}

		for (Int32 index = 0; index < source->GetCurveCount(); ++index)
		{
			asset.curves_.emplace_back().data_ = readFile(source->GetCurvePath(index));
		}

		return true;
	}

	Bool EffekseerLoader::Read(String filePath, EffekseerAsset& asset)
	{
		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			SC_LOG_ERROR("Effekseerキャッシュを読み込めませんでした: {}", filePath.str());
			return false;
		}

		asset.Serialize(archive);
		return true;
	}

	Bool EffekseerLoader::Write(const EffekseerAsset& asset, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<EffekseerAsset&>(asset).Serialize(archive);
		if (!archive.Write(filePath))
		{
			SC_LOG_ERROR("Effekseerキャッシュを書き出せませんでした: {}", filePath.str());
			return false;
		}

		return true;
	}

	Bool EffekseerLoader::Create(const EffekseerAsset& asset, LoadThread thread, EffekseerEffect& effect)
	{
		effect.effect_ = Gateway::GetEffekseerManager().Create(asset);
		if (effect.effect_ == nullptr)
		{
			SC_LOG_ERROR("Effekseerエフェクトを作成できませんでした");
			return false;
		}

		return true;
	}
}
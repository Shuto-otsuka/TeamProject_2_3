#include <External/DirectXTex/Include/DirectXTex.h>
#include <FoundationEngine/Resource/Config/IconConfig.h>

#ifdef _DEBUG
#pragma comment(lib, "../External/DirectXTex/Lib/Debug/DirectXTex.lib")
#else
#pragma comment(lib, "../External/DirectXTex/Lib/Release/DirectXTex.lib")
#endif
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	void IconConfig::Load(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			return;
		}

		BinaryInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			return;
		}

		archive.TryField("icon", icon_);
	}

	void IconConfig::Save(const std::filesystem::path& path)const
	{
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		BinaryOutputArchive archive;
		archive.Field("icon", icon_);
		archive.Write(String(path.string()));
	}

	Bool IconConfig::Import(const std::filesystem::path& imagePath)
	{
		std::string extension = imagePath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Char character) { return static_cast<Char>(std::tolower(static_cast<Uint8>(character))); });

		if (extension == ".ico")
		{
			std::ifstream stream(imagePath, std::ios::binary);
			DynamicArray<Byte> data((std::istreambuf_iterator<Char>(stream)), std::istreambuf_iterator<Char>());

			Uint16 reserved = 0;
			Uint16 type = 0;
			Uint16 count = 0;
			if (data.size() >= 6)
			{
				std::memcpy(&reserved, data.data(), sizeof(Uint16));
				std::memcpy(&type, data.data() + 2, sizeof(Uint16));
				std::memcpy(&count, data.data() + 4, sizeof(Uint16));
			}

			if (reserved != 0 || type != 1 || count == 0 || data.size() < 6 + static_cast<Size>(count) * 16)
			{
				SC_LOG_WARNING("アイコンファイルの形式が不正です: {}", imagePath.string());
				return false;
			}

			icon_ = std::move(data);
			return true;
		}

		Bool comInitializedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

		DynamicArray<Byte> icon;
		Bool success = false;

		do
		{
			DirectX::ScratchImage loadedImage;
			if (FAILED(DirectX::LoadFromWICFile(imagePath.wstring().c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, loadedImage)))
			{
				SC_LOG_WARNING("アイコン用の画像を読み込めませんでした: {}", imagePath.string());
				break;
			}

			DirectX::ScratchImage rgbaImage;
			const DirectX::Image* sourceImage = loadedImage.GetImage(0, 0, 0);
			if (sourceImage->format != DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				if (FAILED(DirectX::Convert(*sourceImage, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgbaImage)))
				{
					SC_LOG_WARNING("アイコン用の画像を RGBA に変換できませんでした: {}", imagePath.string());
					break;
				}
				sourceImage = rgbaImage.GetImage(0, 0, 0);
			}

			Size side = Max(sourceImage->width, sourceImage->height);

			DirectX::ScratchImage squareImage;
			if (FAILED(squareImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, side, side, 1, 1)))
			{
				break;
			}
			std::memset(squareImage.GetPixels(), 0, squareImage.GetPixelsSize());

			if (FAILED(DirectX::CopyRectangle(*sourceImage, DirectX::Rect(0, 0, sourceImage->width, sourceImage->height), *squareImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, (side - sourceImage->width) / 2, (side - sourceImage->height) / 2)))
			{
				break;
			}

			constexpr Uint32 iconSizes[] = { 256, 128, 64, 48, 32, 24, 16 };

			DynamicArray<DynamicArray<Byte>> pngImages;
			for (Uint32 iconSize : iconSizes)
			{
				DirectX::ScratchImage resizedImage;
				if (FAILED(DirectX::Resize(*squareImage.GetImage(0, 0, 0), iconSize, iconSize, DirectX::TEX_FILTER_FANT, resizedImage)))
				{
					break;
				}

				DirectX::Blob pngBlob;
				if (FAILED(DirectX::SaveToWICMemory(*resizedImage.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), pngBlob)))
				{
					break;
				}

				const Byte* pngBegin = reinterpret_cast<const Byte*>(pngBlob.GetBufferPointer());
				pngImages.push_back(DynamicArray<Byte>(pngBegin, pngBegin + pngBlob.GetBufferSize()));
			}

			if (pngImages.size() != std::size(iconSizes))
			{
				SC_LOG_WARNING("アイコンの縮小/PNG 化に失敗しました: {}", imagePath.string());
				break;
			}

			auto append = [&icon](Uint32 value, Size byteCount)
			{
				for (Size byteIndex = 0; byteIndex < byteCount; ++byteIndex)
				{
					icon.push_back(static_cast<Byte>((value >> (byteIndex * 8)) & 0xFF));
				}
			};

			append(0, 2);
			append(1, 2);
			append(static_cast<Uint32>(pngImages.size()), 2);

			Uint32 imageOffset = static_cast<Uint32>(6 + pngImages.size() * 16);
			for (Size imageIndex = 0; imageIndex < pngImages.size(); ++imageIndex)
			{
				Uint32 iconSize = iconSizes[imageIndex];
				append(iconSize >= 256 ? 0 : iconSize, 1);
				append(iconSize >= 256 ? 0 : iconSize, 1);
				append(0, 1);
				append(0, 1);
				append(1, 2);
				append(32, 2);
				append(static_cast<Uint32>(pngImages[imageIndex].size()), 4);
				append(imageOffset, 4);
				imageOffset += static_cast<Uint32>(pngImages[imageIndex].size());
			}

			for (const DynamicArray<Byte>& pngImage : pngImages)
			{
				icon.insert(icon.end(), pngImage.begin(), pngImage.end());
			}

			success = true;
		} while (false);

		if (comInitializedHere)
		{
			CoUninitialize();
		}

		if (!success)
		{
			return false;
		}

		icon_ = std::move(icon);
		return true;
	}
}

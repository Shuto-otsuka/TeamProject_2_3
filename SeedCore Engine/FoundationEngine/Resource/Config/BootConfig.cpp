#include <External/DirectXTex/Include/DirectXTex.h>
#include <FoundationEngine/Resource/Config/BootConfig.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	void BootConfig::Load(const std::filesystem::path& path)
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

		Int32 anchorValue = static_cast<Int32>(anchor_);
		Int32 fillMethodValue = static_cast<Int32>(fillMethod_);

		archive.TryField("backgroundImage", backgroundImage_);
		archive.TryField("barImage", barImage_);
		archive.TryField("frameImage", frameImage_);
		archive.TryField("useBackgroundImage", useBackgroundImage_);
		archive.TryField("useFrame", useFrame_);
		archive.TryField("backgroundColor", backgroundColor_);
		archive.TryField("backgroundTint", backgroundTint_);
		archive.TryField("barTint", barTint_);
		archive.TryField("frameTint", frameTint_);
		archive.TryField("anchor", anchorValue);
		archive.TryField("offset", offset_);
		archive.TryField("width", width_);
		archive.TryField("fillMethod", fillMethodValue);
		archive.TryField("fillOrigin", fillOrigin_);
		archive.TryField("clockwise", clockwise_);
		archive.TryField("wave", wave_);

		anchor_ = static_cast<BootAnchor>(anchorValue);
		fillMethod_ = static_cast<BootFillMethod>(fillMethodValue);
	}

	void BootConfig::Save(const std::filesystem::path& path)const
	{
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		Int32 anchorValue = static_cast<Int32>(anchor_);
		Int32 fillMethodValue = static_cast<Int32>(fillMethod_);

		BinaryOutputArchive archive;
		archive.Field("backgroundImage", backgroundImage_);
		archive.Field("barImage", barImage_);
		archive.Field("frameImage", frameImage_);
		archive.Field("useBackgroundImage", useBackgroundImage_);
		archive.Field("useFrame", useFrame_);
		archive.Field("backgroundColor", backgroundColor_);
		archive.Field("backgroundTint", backgroundTint_);
		archive.Field("barTint", barTint_);
		archive.Field("frameTint", frameTint_);
		archive.Field("anchor", anchorValue);
		archive.Field("offset", offset_);
		archive.Field("width", width_);
		archive.Field("fillMethod", fillMethodValue);
		archive.Field("fillOrigin", fillOrigin_);
		archive.Field("clockwise", clockwise_);
		archive.Field("wave", wave_);
		archive.Write(String(path.string()));
	}

	Bool BootConfig::Import(const std::filesystem::path& imagePath, DynamicArray<Byte>& image)
	{
		std::ifstream stream(imagePath, std::ios::binary);
		if (!stream)
		{
			SC_LOG_WARNING("ローディング画面用の画像を開けませんでした: {}", imagePath.string());
			return false;
		}

		DynamicArray<Byte> data((std::istreambuf_iterator<Char>(stream)), std::istreambuf_iterator<Char>());
		if (data.empty())
		{
			SC_LOG_WARNING("ローディング画面用の画像が空です: {}", imagePath.string());
			return false;
		}

		DirectX::TexMetadata metadata{};
		Bool decodable = SUCCEEDED(DirectX::GetMetadataFromDDSMemory(reinterpret_cast<const Uint8*>(data.data()), data.size(), DirectX::DDS_FLAGS_NONE, metadata));

		if (!decodable)
		{
			Bool comInitializedHere = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
			decodable = SUCCEEDED(DirectX::GetMetadataFromWICMemory(reinterpret_cast<const Uint8*>(data.data()), data.size(), DirectX::WIC_FLAGS_NONE, metadata));
			if (comInitializedHere)
			{
				CoUninitialize();
			}
		}

		if (!decodable)
		{
			SC_LOG_WARNING("ローディング画面用の画像として読み込めない形式です: {}", imagePath.string());
			return false;
		}

		image = std::move(data);
		return true;
	}
}

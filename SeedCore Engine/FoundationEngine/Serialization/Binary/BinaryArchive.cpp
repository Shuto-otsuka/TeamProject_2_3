#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>

namespace SeedCore
{
	class BinaryOutputArchive::Impl final
	{
	public:
		DynamicArray<Byte> body_;
	};

	class BinaryInputArchive::Impl final
	{
	public:
		void SetView(const Byte* data, Size size)
		{
			data_ = data;
			size_ = size;
			index_.clear();

			Size offset = 0;
			while (offset + sizeof(Uint32) * 2 <= size_)
			{
				Uint32 id = 0;
				std::memcpy(&id, data_ + offset, sizeof(Uint32));
				offset += sizeof(Uint32);

				Uint32 size = 0;
				std::memcpy(&size, data_ + offset, sizeof(Uint32));
				offset += sizeof(Uint32);

				if (offset + size > size_)
				{
					SC_LOG_WARNING("バイナリレコードのサイズが不正です。");
					break;
				}

				index_[id] = { offset, size };
				offset += size;
			}
		}

		DynamicArray<Byte> ownedData_;

		std::unordered_map<Uint32, std::pair<Size, Size>> index_;

		const Byte* data_ = nullptr;

		Size size_ = 0;
	};

	BinaryField::BinaryField(const Char* name)
	{
		Uint32 hash = 2166136261u;
		for (const Char* pointer = name; *pointer != '\0'; ++pointer)
		{
			hash ^= static_cast<Uint8>(*pointer);
			hash *= 16777619u;
		}
		value_ = hash;
	}

	Uint32 BinaryField::GetValue()const
	{
		return value_;
	}

	BinaryOutputArchive::BinaryOutputArchive() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	BinaryOutputArchive::~BinaryOutputArchive() = default;

	BinaryOutputArchive::BinaryOutputArchive(BinaryOutputArchive&&)noexcept = default;

	BinaryOutputArchive& BinaryOutputArchive::operator=(BinaryOutputArchive&&)noexcept = default;

	void BinaryOutputArchive::AppendField(const Char* name, BinaryOutputArchive&& payload)
	{
		AppendUint32(BinaryField(name).GetValue());
		AppendUint32(static_cast<Uint32>(payload.impl_->body_.size()));
		impl_->body_.insert(impl_->body_.end(), payload.impl_->body_.begin(), payload.impl_->body_.end());
	}

	void BinaryOutputArchive::AppendUint32(Uint32 value)
	{
		AppendRaw(&value, sizeof(Uint32));
	}

	void BinaryOutputArchive::AppendRaw(const void* data, Size size)
	{
		if (size == 0)
		{
			return;
		}
		const Byte* bytes = static_cast<const Byte*>(data);
		impl_->body_.insert(impl_->body_.end(), bytes, bytes + size);
	}

	void BinaryOutputArchive::AppendSized(BinaryOutputArchive&& payload)
	{
		AppendUint32(static_cast<Uint32>(payload.impl_->body_.size()));
		impl_->body_.insert(impl_->body_.end(), payload.impl_->body_.begin(), payload.impl_->body_.end());
	}

	Bool BinaryOutputArchive::Write(const String& filePath)const
	{
		std::ofstream ofs(filePath.c_str(), std::ios::binary);
		if (!ofs.is_open())
		{
			SC_LOG_WARNING("\"{}\" を書き込み用に開けませんでした。", filePath.c_str());
			return false;
		}

		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));
		DynamicArray<Byte> iv(16);
		std::random_device randomDevice;
		for (Byte& ivByte : iv)
		{
			ivByte = static_cast<Byte>(randomDevice());
		}

		DynamicArray<Byte> ciphertext = Aes256::Encrypt(key, iv, impl_->body_);
		ofs.write(iv.data(), iv.size());
		ofs.write(ciphertext.data(), ciphertext.size());
		return true;
	}

	BinaryInputArchive::BinaryInputArchive() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	BinaryInputArchive::BinaryInputArchive(const Byte* data, Size size) :impl_(MakePtr<Impl>())
	{
		impl_->SetView(data, size);
	}

	BinaryInputArchive::~BinaryInputArchive() = default;

	BinaryInputArchive::BinaryInputArchive(BinaryInputArchive&&)noexcept = default;

	BinaryInputArchive& BinaryInputArchive::operator=(BinaryInputArchive&&)noexcept = default;

	Bool BinaryInputArchive::FindField(const Char* name, BinaryInputArchive& result)const
	{
		auto entry = impl_->index_.find(BinaryField(name).GetValue());
		if (entry == impl_->index_.end())
		{
			return false;
		}

		result = Slice(entry->second.first, entry->second.second);
		return true;
	}

	BinaryInputArchive BinaryInputArchive::Slice(Size offset, Size size)const
	{
		if (offset > impl_->size_ || size > impl_->size_ - offset)
		{
			return BinaryInputArchive();
		}
		const Byte* data = impl_->data_ ? impl_->data_ + offset : nullptr;
		return BinaryInputArchive(data, size);
	}

	Size BinaryInputArchive::DataSize()const
	{
		return impl_->size_;
	}

	Uint32 BinaryInputArchive::ReadUint32(Size offset)const
	{
		Uint32 value = 0;
		ReadRaw(offset, &value, sizeof(Uint32));
		return value;
	}

	Bool BinaryInputArchive::ReadRaw(Size offset, void* destination, Size size)const
	{
		if (offset > impl_->size_ || size > impl_->size_ - offset)
		{
			return false;
		}
		if (size > 0)
		{
			std::memcpy(destination, impl_->data_ + offset, size);
		}
		return true;
	}

	Bool BinaryInputArchive::Has(const Char* name)const
	{
		return impl_->index_.contains(BinaryField(name).GetValue());
	}

	Bool BinaryInputArchive::Read(const String& filePath)
	{
		std::ifstream ifs(filePath.c_str(), std::ios::binary);
		if (!ifs.is_open())
		{
			SC_LOG_WARNING("\"{}\" を読み込み用に開けませんでした。", filePath.c_str());
			return false;
		}

		ifs.seekg(0, std::ios::end);
		Size fileSize = static_cast<Size>(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		DynamicArray<Byte> rawData(fileSize);
		if (fileSize > 0 && !ifs.read(rawData.data(), fileSize))
		{
			SC_LOG_WARNING("\"{}\" の読み込みに失敗しました。", filePath.c_str());
			return false;
		}
		if (rawData.size() < 16)
		{
			SC_LOG_WARNING("\"{}\" の暗号化データが不正です。", filePath.c_str());
			return false;
		}

		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));
		DynamicArray<Byte> iv(rawData.begin(), rawData.begin() + 16);
		DynamicArray<Byte> ciphertext(rawData.begin() + 16, rawData.end());

		impl_->ownedData_ = Aes256::Decrypt(key, iv, ciphertext);
		if (impl_->ownedData_.empty() && !ciphertext.empty())
		{
			SC_LOG_WARNING("\"{}\" の復号に失敗しました。", filePath.c_str());
			return false;
		}

		impl_->SetView(impl_->ownedData_.data(), impl_->ownedData_.size());
		return true;
	}
}

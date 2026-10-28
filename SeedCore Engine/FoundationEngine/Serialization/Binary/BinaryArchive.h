#pragma once
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Serialization/SerializeSupport.h>

namespace SeedCore
{
	template<typename T>
	concept BinarySequenceContainer = requires(T& container)
	{
		typename T::value_type;
		container.begin();
		container.end();
		container.push_back(std::declval<typename T::value_type>());
	} && !std::same_as<T, String> && !std::same_as<T, std::string> && !std::same_as<T, std::u16string>;

	template<typename T>
	concept BinaryAssociativeContainer = requires(T& container)
	{
		typename T::key_type;
		typename T::mapped_type;
		container.begin();
		container.end();
		container.clear();
		container.emplace(std::declval<typename T::key_type>(), std::declval<typename T::mapped_type>());
	};

	template<typename T>
	concept BinaryFixedSizeArray = requires(T& container)
	{
		typename T::value_type;
		container.begin();
		container.end();
		requires std::tuple_size<T>::value >= 0;
	};

	class SEEDCORE_API BinaryField
	{
	public:
		explicit BinaryField(const Char* name);

		Uint32 GetValue()const;

	private:
		Uint32 value_ = 0;
	};

	class SEEDCORE_API BinaryOutputArchive
	{
	public:
		BinaryOutputArchive();
		~BinaryOutputArchive();

		BinaryOutputArchive(BinaryOutputArchive&&)noexcept;
		BinaryOutputArchive& operator=(BinaryOutputArchive&&)noexcept;

		BinaryOutputArchive(const BinaryOutputArchive&) = delete;
		BinaryOutputArchive& operator=(const BinaryOutputArchive&) = delete;

		template<typename T>
		void Field(const Char* name, const T& value)
		{
			BinaryOutputArchive payload;
			WriteInto(payload, value);
			AppendField(name, std::move(payload));
		}

		template<typename T>
		Bool TryField(const Char* name, const T& value)
		{
			Field(name, value);
			return true;
		}

		Bool Write(const String& filePath)const;

	private:
		template<typename T>
		void WriteInto(BinaryOutputArchive& target, const T& value)
		{
			if constexpr (requires { value.Save(target); })
			{
				value.Save(target);
			}
			else if constexpr (requires(T& mutableValue) { mutableValue.Serialize(target); })
			{
				const_cast<T&>(value).Serialize(target);
			}
			else if constexpr (requires { Save(target, value); })
			{
				Save(target, value);
			}
			else if constexpr (BinarySequenceContainer<T>)
			{
				using ElementType = typename T::value_type;

				target.AppendUint32(static_cast<Uint32>(value.size()));
				if constexpr (std::is_trivially_copyable_v<ElementType> && !std::same_as<ElementType, Bool> && !std::same_as<ElementType, String> && requires { value.data(); })
				{
					target.AppendRaw(value.data(), value.size() * sizeof(ElementType));
				}
				else
				{
					for (const auto& element : value)
					{
						BinaryOutputArchive elementArchive;
						if constexpr (std::same_as<ElementType, Bool>)
						{
							Bool boolElement = static_cast<Bool>(element);
							WriteInto(elementArchive, boolElement);
						}
						else
						{
							WriteInto(elementArchive, element);
						}
						target.AppendSized(std::move(elementArchive));
					}
				}
			}
			else if constexpr (BinaryAssociativeContainer<T>)
			{
				target.AppendUint32(static_cast<Uint32>(value.size()));
				for (const auto& entry : value)
				{
					BinaryOutputArchive entryArchive;
					entryArchive.Field("key", entry.first);
					entryArchive.Field("value", entry.second);
					target.AppendSized(std::move(entryArchive));
				}
			}
			else if constexpr (BinaryFixedSizeArray<T>)
			{
				target.AppendUint32(static_cast<Uint32>(value.size()));
				for (const auto& element : value)
				{
					BinaryOutputArchive elementArchive;
					WriteInto(elementArchive, element);
					target.AppendSized(std::move(elementArchive));
				}
			}
			else if constexpr (std::same_as<T, String>)
			{
				std::string raw = value.str();
				target.AppendRaw(raw.data(), raw.size());
			}
			else if constexpr (std::same_as<T, std::string>)
			{
				target.AppendRaw(value.data(), value.size());
			}
			else if constexpr (std::same_as<T, std::u16string>)
			{
				target.AppendRaw(value.data(), value.size() * sizeof(Char16));
			}
			else if constexpr (std::is_trivially_copyable_v<T>)
			{
				target.AppendRaw(&value, sizeof(T));
			}
			else
			{
				static_assert(sizeof(T) == 0, "BinaryOutputArchive: no serialization support for this type, add a free Save/Load overload");
			}
		}

		void AppendField(const Char* name, BinaryOutputArchive&& payload);

		void AppendUint32(Uint32 value);

		void AppendRaw(const void* data, Size size);

		void AppendSized(BinaryOutputArchive&& payload);

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};

	class SEEDCORE_API BinaryInputArchive
	{
	public:
		BinaryInputArchive();
		~BinaryInputArchive();

		BinaryInputArchive(BinaryInputArchive&&)noexcept;
		BinaryInputArchive& operator=(BinaryInputArchive&&)noexcept;

		BinaryInputArchive(const BinaryInputArchive&) = delete;
		BinaryInputArchive& operator=(const BinaryInputArchive&) = delete;

		template<typename T>
		void Field(const Char* name, T& value)
		{
			BinaryInputArchive payload;
			if (!FindField(name, payload))
			{
				SC_LOG_WARNING("バイナリフィールド \"{}\" が見つかりません。", name);
				return;
			}
			ReadFrom(payload, value);
		}

		template<typename T>
		Bool TryField(const Char* name, T& value)
		{
			BinaryInputArchive payload;
			if (!FindField(name, payload))
			{
				return false;
			}
			ReadFrom(payload, value);
			return true;
		}

		Bool Has(const Char* name)const;

		Bool Read(const String& filePath);

	private:
		BinaryInputArchive(const Byte* data, Size size);

		template<typename T>
		void ReadFrom(const BinaryInputArchive& source, T& value)
		{
			if constexpr (requires { value.Load(*this); })
			{
				BinaryInputArchive child = source.Slice(0, source.DataSize());
				value.Load(child);
			}
			else if constexpr (requires { value.Serialize(*this); })
			{
				BinaryInputArchive child = source.Slice(0, source.DataSize());
				value.Serialize(child);
			}
			else if constexpr (requires { Load(*this, value); })
			{
				BinaryInputArchive child = source.Slice(0, source.DataSize());
				Load(child, value);
			}
			else if constexpr (BinarySequenceContainer<T>)
			{
				using ElementType = typename T::value_type;
				if (source.DataSize() < sizeof(Uint32))
				{
					SC_LOG_WARNING("バイナリ配列のヘッダが不正です。初期値を使用します。");
					return;
				}

				Uint32 elementCount = source.ReadUint32(0);
				if constexpr (std::is_trivially_copyable_v<ElementType> && !std::same_as<ElementType, Bool> && !std::same_as<ElementType, String> && requires { value.data(); value.resize(Size(0)); })
				{
					Size expectedBytes = static_cast<Size>(elementCount) * sizeof(ElementType);
					if (source.DataSize() != sizeof(Uint32) + expectedBytes)
					{
						SC_LOG_WARNING("バイナリ配列のサイズが要素数と一致しません。初期値を使用します。");
						return;
					}
					value.resize(elementCount);
					source.ReadRaw(sizeof(Uint32), value.data(), expectedBytes);
				}
				else
				{
					Size offset = sizeof(Uint32);
					value.clear();
					for (Uint32 elementIndex = 0; elementIndex < elementCount; ++elementIndex)
					{
						if (offset + sizeof(Uint32) > source.DataSize())
						{
							SC_LOG_WARNING("バイナリ配列の要素ヘッダが不正です。読み込みを打ち切ります。");
							break;
						}
						Uint32 elementSize = source.ReadUint32(offset);
						offset += sizeof(Uint32);
						if (offset + elementSize > source.DataSize())
						{
							SC_LOG_WARNING("バイナリ配列の要素サイズが不正です。読み込みを打ち切ります。");
							break;
						}
						BinaryInputArchive elementArchive = source.Slice(offset, elementSize);
						typename T::value_type element{};
						ReadFrom(elementArchive, element);
						value.push_back(std::move(element));
						offset += elementSize;
					}
				}
			}
			else if constexpr (BinaryAssociativeContainer<T>)
			{
				if (source.DataSize() < sizeof(Uint32))
				{
					SC_LOG_WARNING("バイナリマップのヘッダが不正です。初期値を使用します。");
					return;
				}
				Uint32 entryCount = source.ReadUint32(0);
				Size offset = sizeof(Uint32);
				value.clear();
				for (Uint32 entryIndex = 0; entryIndex < entryCount; ++entryIndex)
				{
					if (offset + sizeof(Uint32) > source.DataSize())
					{
						SC_LOG_WARNING("バイナリマップの要素ヘッダが不正です。読み込みを打ち切ります。");
						break;
					}
					Uint32 entrySize = source.ReadUint32(offset);
					offset += sizeof(Uint32);
					if (offset + entrySize > source.DataSize())
					{
						SC_LOG_WARNING("バイナリマップの要素サイズが不正です。読み込みを打ち切ります。");
						break;
					}
					BinaryInputArchive entryArchive = source.Slice(offset, entrySize);
					offset += entrySize;
					typename T::key_type key{};
					typename T::mapped_type mapped{};
					entryArchive.TryField("key", key);
					entryArchive.TryField("value", mapped);
					value.emplace(std::move(key), std::move(mapped));
				}
			}
			else if constexpr (BinaryFixedSizeArray<T>)
			{
				if (source.DataSize() < sizeof(Uint32))
				{
					SC_LOG_WARNING("バイナリ配列のヘッダが不正です。初期値を使用します。");
					return;
				}
				Uint32 elementCount = source.ReadUint32(0);
				Size offset = sizeof(Uint32);
				Size elementIndex = 0;
				for (Uint32 sourceIndex = 0; sourceIndex < elementCount; ++sourceIndex)
				{
					if (offset + sizeof(Uint32) > source.DataSize())
					{
						SC_LOG_WARNING("バイナリ配列の要素ヘッダが不正です。読み込みを打ち切ります。");
						break;
					}
					Uint32 elementSize = source.ReadUint32(offset);
					offset += sizeof(Uint32);
					if (offset + elementSize > source.DataSize())
					{
						SC_LOG_WARNING("バイナリ配列の要素サイズが不正です。読み込みを打ち切ります。");
						break;
					}
					if (elementIndex >= std::tuple_size<T>::value)
					{
						SC_LOG_WARNING("バイナリ配列の要素数が固定長配列より多いため、超過分を無視します。");
						break;
					}
					BinaryInputArchive elementArchive = source.Slice(offset, elementSize);
					ReadFrom(elementArchive, value[elementIndex]);
					offset += elementSize;
					++elementIndex;
				}
			}
			else if constexpr (std::same_as<T, String>)
			{
				std::string raw(source.DataSize(), '\0');
				source.ReadRaw(0, raw.data(), raw.size());
				value = String(std::string_view(raw));
			}
			else if constexpr (std::same_as<T, std::string>)
			{
				value.resize(source.DataSize());
				source.ReadRaw(0, value.data(), value.size());
			}
			else if constexpr (std::same_as<T, std::u16string>)
			{
				value.resize(source.DataSize() / sizeof(Char16));
				source.ReadRaw(0, value.data(), value.size() * sizeof(Char16));
			}
			else if constexpr (std::is_trivially_copyable_v<T>)
			{
				if (source.DataSize() != sizeof(T))
				{
					SC_LOG_WARNING("バイナリフィールドのサイズが型と一致しません。初期値を使用します。");
					return;
				}
				source.ReadRaw(0, &value, sizeof(T));
			}
			else
			{
				static_assert(sizeof(T) == 0, "BinaryInputArchive: no serialization support for this type, add a free Save/Load overload");
			}
		}

		Bool FindField(const Char* name, BinaryInputArchive& result)const;

		BinaryInputArchive Slice(Size offset, Size size)const;

		Size DataSize()const;

		Uint32 ReadUint32(Size offset)const;

		Bool ReadRaw(Size offset, void* destination, Size size)const;

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}

#pragma once
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Serialization/SerializeSupport.h>

namespace SeedCore
{
	template<typename T>
	concept JsonSequenceContainer = requires(T& container)
	{
		typename T::value_type;
		container.begin();
		container.end();
		container.push_back(std::declval<typename T::value_type>());
	} && !std::same_as<T, String> && !std::same_as<T, std::string>;

	template<typename T>
	concept AssociativeContainer = requires(T& container)
	{
		typename T::key_type;
		typename T::mapped_type;
		container.begin();
		container.end();
		container.clear();
		container.emplace(std::declval<typename T::key_type>(), std::declval<typename T::mapped_type>());
	};

	template<typename T>
	concept FixedSizeArray = requires(T& container)
	{
		typename T::value_type;
		container.begin();
		container.end();
		requires std::tuple_size<T>::value >= 0;
	};

	class SEEDCORE_API JsonOutputArchive
	{
	public:
		JsonOutputArchive();
		~JsonOutputArchive();

		JsonOutputArchive(JsonOutputArchive&&)noexcept;
		JsonOutputArchive& operator=(JsonOutputArchive&&)noexcept;

		JsonOutputArchive(const JsonOutputArchive&) = delete;
		JsonOutputArchive& operator=(const JsonOutputArchive&) = delete;

		template<typename T>
		void Field(const Char* name, const T& value)
		{
			JsonOutputArchive child;
			WriteInto(child, value);
			SetField(name, std::move(child));
		}

		template<typename T>
		Bool TryField(const Char* name, const T& value)
		{
			Field(name, value);
			return true;
		}

		Bool Write(const String& filePath)const;

		String Dump()const;

	private:
		template<typename T>
		void WriteInto(JsonOutputArchive& target, const T& value)
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
			else if constexpr (JsonSequenceContainer<T>)
			{
				target.MakeArray();
				for (const auto& element : value)
				{
					JsonOutputArchive child;
					if constexpr (std::same_as<typename T::value_type, Bool>)
					{
						Bool boolElement = static_cast<Bool>(element);
						WriteInto(child, boolElement);
					}
					else
					{
						WriteInto(child, element);
					}
					target.Append(std::move(child));
				}
			}
			else if constexpr (AssociativeContainer<T>)
			{
				target.MakeArray();
				for (const auto& entry : value)
				{
					JsonOutputArchive child;
					child.Field("key", entry.first);
					child.Field("value", entry.second);
					target.Append(std::move(child));
				}
			}
			else if constexpr (FixedSizeArray<T>)
			{
				target.MakeArray();
				for (const auto& element : value)
				{
					JsonOutputArchive child;
					WriteInto(child, element);
					target.Append(std::move(child));
				}
			}
			else if constexpr (std::is_array_v<T>)
			{
				target.MakeArray();
				constexpr Size elementCount = std::extent_v<T>;
				for (Size elementIndex = 0; elementIndex < elementCount; ++elementIndex)
				{
					JsonOutputArchive child;
					WriteInto(child, value[elementIndex]);
					target.Append(std::move(child));
				}
			}
			else if constexpr (std::same_as<T, String>)
			{
				target.SetString(value.str());
			}
			else if constexpr (std::same_as<T, std::string>)
			{
				target.SetString(value);
			}
			else if constexpr (std::same_as<T, Bool>)
			{
				target.SetBool(value);
			}
			else if constexpr (std::is_enum_v<T>)
			{
				WriteInto(target, static_cast<std::underlying_type_t<T>>(value));
			}
			else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
			{
				target.SetInt(static_cast<Int64>(value));
			}
			else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
			{
				target.SetUint(static_cast<Uint64>(value));
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				target.SetDouble(static_cast<Double>(value));
			}
			else if constexpr (std::same_as<T, std::nullptr_t>)
			{
				target.SetNull();
			}
			else
			{
				static_assert(sizeof(T) == 0, "JsonOutputArchive: no serialization support for this type, add a free Save/Load overload");
			}
		}

		void SetField(const Char* name, JsonOutputArchive&& child);

		void MakeArray();

		void Append(JsonOutputArchive&& child);

		void SetString(const std::string& value);

		void SetBool(Bool value);

		void SetInt(Int64 value);

		void SetUint(Uint64 value);

		void SetDouble(Double value);

		void SetNull();

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};

	class SEEDCORE_API JsonInputArchive
	{
	public:
		JsonInputArchive();
		~JsonInputArchive();

		JsonInputArchive(JsonInputArchive&&)noexcept;
		JsonInputArchive& operator=(JsonInputArchive&&)noexcept;

		JsonInputArchive(const JsonInputArchive&) = delete;
		JsonInputArchive& operator=(const JsonInputArchive&) = delete;

		template<typename T>
		void Field(const Char* name, T& value)
		{
			JsonInputArchive child;
			if (!FindField(name, child))
			{
				SC_LOG_WARNING("jsonフィールド \"{}\" が見つかりません。", name);
				return;
			}
			ReadFrom(child, value);
		}

		template<typename T>
		Bool TryField(const Char* name, T& value)
		{
			JsonInputArchive child;
			if (!FindField(name, child))
			{
				return false;
			}
			ReadFrom(child, value);
			return true;
		}

		Bool Has(const Char* name)const;

		Bool Read(const String& filePath);

		Bool Parse(const String& json);

	private:
		explicit JsonInputArchive(const void* node);

		template<typename T>
		void ReadFrom(const JsonInputArchive& source, T& value)
		{
			if constexpr (requires { value.Load(*this); })
			{
				JsonInputArchive child = source.View();
				value.Load(child);
			}
			else if constexpr (requires { value.Serialize(*this); })
			{
				JsonInputArchive child = source.View();
				value.Serialize(child);
			}
			else if constexpr (requires { Load(*this, value); })
			{
				JsonInputArchive child = source.View();
				Load(child, value);
			}
			else if constexpr (JsonSequenceContainer<T>)
			{
				value.clear();
				if (source.IsNull())
				{
					return;
				}
				if (!source.IsArray())
				{
					SC_LOG_WARNING("json配列が期待されていましたが異なる型でした。初期値を使用します。");
					return;
				}
				for (Size index = 0; index < source.ArraySize(); ++index)
				{
					JsonInputArchive elementArchive;
					source.ArrayElement(index, elementArchive);
					typename T::value_type element{};
					ReadFrom(elementArchive, element);
					value.push_back(std::move(element));
				}
			}
			else if constexpr (AssociativeContainer<T>)
			{
				value.clear();
				if (source.IsNull())
				{
					return;
				}
				if (!source.IsArray())
				{
					SC_LOG_WARNING("json配列（マップ）が期待されていましたが異なる型でした。初期値を使用します。");
					return;
				}
				for (Size index = 0; index < source.ArraySize(); ++index)
				{
					JsonInputArchive entryArchive;
					source.ArrayElement(index, entryArchive);
					typename T::key_type key{};
					typename T::mapped_type mapped{};
					entryArchive.TryField("key", key);
					entryArchive.TryField("value", mapped);
					value.emplace(std::move(key), std::move(mapped));
				}
			}
			else if constexpr (FixedSizeArray<T>)
			{
				if (!source.IsArray())
				{
					SC_LOG_WARNING("json配列が期待されていましたが異なる型でした。初期値を使用します。");
					return;
				}
				Size count = Min<Size>(source.ArraySize(), std::tuple_size<T>::value);
				if (source.ArraySize() > std::tuple_size<T>::value)
				{
					SC_LOG_WARNING("json配列の要素数が固定長配列より多いため、超過分を無視します。");
				}
				for (Size index = 0; index < count; ++index)
				{
					JsonInputArchive elementArchive;
					source.ArrayElement(index, elementArchive);
					ReadFrom(elementArchive, value[index]);
				}
			}
			else if constexpr (std::is_array_v<T>)
			{
				if (!source.IsArray())
				{
					SC_LOG_WARNING("json配列が期待されていましたが異なる型でした。初期値を使用します。");
					return;
				}
				constexpr Size elementCount = std::extent_v<T>;
				Size count = Min<Size>(source.ArraySize(), elementCount);
				if (source.ArraySize() > elementCount)
				{
					SC_LOG_WARNING("json配列の要素数が固定長配列より多いため、超過分を無視します。");
				}
				for (Size index = 0; index < count; ++index)
				{
					JsonInputArchive elementArchive;
					source.ArrayElement(index, elementArchive);
					ReadFrom(elementArchive, value[index]);
				}
			}
			else if constexpr (std::same_as<T, String>)
			{
				if (!source.GetString(value)) LogValueError();
			}
			else if constexpr (std::same_as<T, std::string>)
			{
				String text;
				if (source.GetString(text)) value = text.str(); else LogValueError();
			}
			else if constexpr (std::same_as<T, Bool>)
			{
				if (!source.GetBool(value)) LogValueError();
			}
			else if constexpr (std::is_enum_v<T>)
			{
				std::underlying_type_t<T> raw = static_cast<std::underlying_type_t<T>>(value);
				ReadFrom(source, raw);
				value = static_cast<T>(raw);
			}
			else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
			{
				Int64 raw = 0;
				if (source.GetInt(raw)) value = static_cast<T>(raw); else LogValueError();
			}
			else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>)
			{
				Uint64 raw = 0;
				if (source.GetUint(raw)) value = static_cast<T>(raw); else LogValueError();
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				Double raw = 0.0;
				if (source.GetDouble(raw)) value = static_cast<T>(raw); else LogValueError();
			}
			else
			{
				static_assert(sizeof(T) == 0, "JsonInputArchive: no serialization support for this type, add a free Save/Load overload");
			}
		}

		static void LogValueError()
		{
			SC_LOG_WARNING("jsonフィールドの解析に失敗しました。初期値を使用します。");
		}

		JsonInputArchive View()const;

		Bool FindField(const Char* name, JsonInputArchive& result)const;

		Bool IsNull()const;

		Bool IsArray()const;

		Size ArraySize()const;

		Bool ArrayElement(Size index, JsonInputArchive& result)const;

		Bool GetString(String& value)const;

		Bool GetBool(Bool& value)const;

		Bool GetInt(Int64& value)const;

		Bool GetUint(Uint64& value)const;

		Bool GetDouble(Double& value)const;

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}

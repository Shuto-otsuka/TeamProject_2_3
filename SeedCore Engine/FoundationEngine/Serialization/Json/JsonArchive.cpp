#include <External/Json/json.hpp>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	class JsonOutputArchive::Impl final
	{
	public:
		nlohmann::json node_;
	};

	class JsonInputArchive::Impl final
	{
	public:
		nlohmann::json ownedNode_;

		const nlohmann::json* node_ = &ownedNode_;
	};

	JsonOutputArchive::JsonOutputArchive() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JsonOutputArchive::~JsonOutputArchive() = default;

	JsonOutputArchive::JsonOutputArchive(JsonOutputArchive&&)noexcept = default;

	JsonOutputArchive& JsonOutputArchive::operator=(JsonOutputArchive&&)noexcept = default;

	void JsonOutputArchive::SetField(const Char* name, JsonOutputArchive&& child)
	{
		impl_->node_[name] = std::move(child.impl_->node_);
	}

	void JsonOutputArchive::MakeArray()
	{
		impl_->node_ = nlohmann::json::array();
	}

	void JsonOutputArchive::Append(JsonOutputArchive&& child)
	{
		impl_->node_.push_back(std::move(child.impl_->node_));
	}

	void JsonOutputArchive::SetString(const std::string& value)
	{
		impl_->node_ = value;
	}

	void JsonOutputArchive::SetBool(Bool value)
	{
		impl_->node_ = value;
	}

	void JsonOutputArchive::SetInt(Int64 value)
	{
		impl_->node_ = value;
	}

	void JsonOutputArchive::SetUint(Uint64 value)
	{
		impl_->node_ = value;
	}

	void JsonOutputArchive::SetDouble(Double value)
	{
		impl_->node_ = value;
	}

	void JsonOutputArchive::SetNull()
	{
		impl_->node_ = nullptr;
	}

	Bool JsonOutputArchive::Write(const String& filePath)const
	{
		std::ofstream ofs(filePath.c_str());
		if (!ofs.is_open())
		{
			SC_LOG_WARNING("\"{}\" を書き込み用に開けませんでした。", filePath.c_str());
			return false;
		}

		ofs << impl_->node_.dump(1, '\t');
		return true;
	}

	String JsonOutputArchive::Dump()const
	{
		return String(impl_->node_.dump(1, '\t'));
	}

	JsonInputArchive::JsonInputArchive() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JsonInputArchive::JsonInputArchive(const void* node) :impl_(MakePtr<Impl>())
	{
		impl_->node_ = static_cast<const nlohmann::json*>(node);
	}

	JsonInputArchive::~JsonInputArchive() = default;

	JsonInputArchive::JsonInputArchive(JsonInputArchive&&)noexcept = default;

	JsonInputArchive& JsonInputArchive::operator=(JsonInputArchive&&)noexcept = default;

	JsonInputArchive JsonInputArchive::View()const
	{
		return JsonInputArchive(impl_->node_);
	}

	Bool JsonInputArchive::FindField(const Char* name, JsonInputArchive& result)const
	{
		if (!impl_->node_->contains(name))
		{
			return false;
		}

		result = JsonInputArchive(&impl_->node_->at(name));
		return true;
	}

	Bool JsonInputArchive::IsNull()const
	{
		return impl_->node_->is_null();
	}

	Bool JsonInputArchive::IsArray()const
	{
		return impl_->node_->is_array();
	}

	Size JsonInputArchive::ArraySize()const
	{
		return impl_->node_->is_array() ? impl_->node_->size() : 0;
	}

	Bool JsonInputArchive::ArrayElement(Size index, JsonInputArchive& result)const
	{
		if (!impl_->node_->is_array() || index >= impl_->node_->size())
		{
			return false;
		}

		result = JsonInputArchive(&impl_->node_->at(index));
		return true;
	}

	Bool JsonInputArchive::GetString(String& value)const
	{
		try
		{
			value = String(std::string_view(impl_->node_->get<std::string>()));
			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}

	Bool JsonInputArchive::GetBool(Bool& value)const
	{
		try
		{
			value = impl_->node_->get<Bool>();
			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}

	Bool JsonInputArchive::GetInt(Int64& value)const
	{
		try
		{
			value = impl_->node_->get<Int64>();
			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}

	Bool JsonInputArchive::GetUint(Uint64& value)const
	{
		try
		{
			value = impl_->node_->get<Uint64>();
			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}

	Bool JsonInputArchive::GetDouble(Double& value)const
	{
		try
		{
			value = impl_->node_->get<Double>();
			return true;
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
	}

	Bool JsonInputArchive::Has(const Char* name)const
	{
		return impl_->node_->contains(name);
	}

	Bool JsonInputArchive::Read(const String& filePath)
	{
		std::ifstream ifs(filePath.c_str());
		if (!ifs.is_open())
		{
			SC_LOG_WARNING("\"{}\" を読み込み用に開けませんでした。", filePath.c_str());
			return false;
		}

		impl_->ownedNode_ = nlohmann::json::parse(ifs, nullptr, false);
		if (impl_->ownedNode_.is_discarded())
		{
			SC_LOG_WARNING("\"{}\" のjsonとしての解析に失敗しました。", filePath.c_str());
			return false;
		}

		impl_->node_ = &impl_->ownedNode_;
		return true;
	}

	Bool JsonInputArchive::Parse(const String& json)
	{
		impl_->ownedNode_ = nlohmann::json::parse(json.str(), nullptr, false);
		if (impl_->ownedNode_.is_discarded())
		{
			SC_LOG_WARNING("jsonとしての解析に失敗しました。");
			return false;
		}

		impl_->node_ = &impl_->ownedNode_;
		return true;
	}
}

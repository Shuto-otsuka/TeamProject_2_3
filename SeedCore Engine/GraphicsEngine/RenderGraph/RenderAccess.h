#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	enum class RenderAccessType :Uint8
	{
		Read,
		History,
		Write,
		Append,
		Constant,
		Target,
		DepthWrite,
		DepthTest,
	};

	class RenderAccess
	{
	public:
		NODISCARD static consteval RenderAccess Read(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Read);
		}

		NODISCARD static consteval RenderAccess History(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::History);
		}

		NODISCARD static consteval RenderAccess Write(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Write);
		}

		NODISCARD static consteval RenderAccess Append(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Append);
		}

		NODISCARD static consteval RenderAccess Constant(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Constant);
		}

		NODISCARD static consteval RenderAccess Target(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Target);
		}

		NODISCARD static consteval RenderAccess DepthWrite(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::DepthWrite);
		}

		NODISCARD static consteval RenderAccess DepthTest(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::DepthTest);
		}

		NODISCARD consteval RenderAccess As(std::string_view alias)const
		{
			RenderAccess access = *this;
			access.alias_ = alias;
			return access;
		}

		NODISCARD constexpr const RenderResource& Resource()const
		{
			return *resource_;
		}

		NODISCARD constexpr RenderAccessType Type()const
		{
			return type_;
		}

		NODISCARD constexpr std::string_view Alias()const
		{
			return alias_;
		}

	private:
		consteval RenderAccess(const RenderResource& resource, RenderAccessType type) :resource_(&resource), type_(type)
		{
			/// No Code
		}

	private:
		const RenderResource* resource_ = nullptr;

		RenderAccessType type_ = RenderAccessType::Read;

		std::string_view alias_;
	};
}
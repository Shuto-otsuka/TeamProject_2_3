#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	enum class RenderAccessType :Uint8
	{
		Read,
		Write,
		Constant,
	};

	class RenderAccess
	{
	public:
		NODISCARD static consteval RenderAccess Read(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Read);
		}

		NODISCARD static consteval RenderAccess Write(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Write);
		}

		NODISCARD static consteval RenderAccess Constant(const RenderResource& resource)
		{
			return RenderAccess(resource, RenderAccessType::Constant);
		}

		NODISCARD constexpr const RenderResource& Resource()const
		{
			return resource_;
		}

		NODISCARD constexpr RenderAccessType Type()const
		{
			return type_;
		}

	private:
		consteval RenderAccess(const RenderResource& resource, RenderAccessType type) :resource_(resource), type_(type)
		{
			/// No Code
		}

	private:
		RenderResource resource_;

		RenderAccessType type_ = RenderAccessType::Read;
	};
}
#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	enum class RenderResourceShape :Uint8
	{
		Texture2D,
		Texture2DArray,
		TextureCube,
		TextureCubeArray,
		Texture3D,
		Buffer,
		Constant,
		AccelerationStructure,
	};

	enum class RenderResourceScale :Uint8
	{
		Render,
		Display,
		Fixed,
	};

	struct RenderResourceDesc
	{
		RenderResourceShape shape_ = RenderResourceShape::Texture2D;

		RenderResourceScale scale_ = RenderResourceScale::Render;

		DXGI_FORMAT format_ = DXGI_FORMAT_UNKNOWN;

		Uint32 divisor_ = 1;

		Uint32 width_ = 0;

		Uint32 height_ = 0;

		Uint32 depth_ = 1;

		Uint32 mipLevels_ = 1;

		Uint32 stride_ = 0;

		Uint32 count_ = 0;

		NODISCARD static consteval RenderResourceDesc ScaledTexture(DXGI_FORMAT format, RenderResourceScale scale = RenderResourceScale::Render, Uint32 divisor = 1, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::Texture2D, scale, format, divisor, 0, 0, 1, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc FixedTexture(DXGI_FORMAT format, Uint32 width, Uint32 height, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::Texture2D, RenderResourceScale::Fixed, format, 1, width, height, 1, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc ScaledTextureArray(DXGI_FORMAT format, Uint32 arraySize, RenderResourceScale scale = RenderResourceScale::Render, Uint32 divisor = 1, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::Texture2DArray, scale, format, divisor, 0, 0, arraySize, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc FixedTextureArray(DXGI_FORMAT format, Uint32 arraySize, Uint32 width, Uint32 height, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::Texture2DArray, RenderResourceScale::Fixed, format, 1, width, height, arraySize, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc Cube(DXGI_FORMAT format, Uint32 size, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::TextureCube, RenderResourceScale::Fixed, format, 1, size, size, 6, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc CubeArray(DXGI_FORMAT format, Uint32 size, Uint32 cubeCount, Uint32 mipLevels = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::TextureCubeArray, RenderResourceScale::Fixed, format, 1, size, size, 6 * cubeCount, mipLevels };
		}

		NODISCARD static consteval RenderResourceDesc Volume(DXGI_FORMAT format, RenderResourceScale scale, Uint32 divisor, Uint32 depth)
		{
			return RenderResourceDesc{ RenderResourceShape::Texture3D, scale, format, divisor, 0, 0, depth, 1 };
		}

		NODISCARD static consteval RenderResourceDesc ScaledBuffer(Uint32 stride, Uint32 count, RenderResourceScale scale, Uint32 divisor, Uint32 depth = 1)
		{
			return RenderResourceDesc{ RenderResourceShape::Buffer, scale, DXGI_FORMAT_UNKNOWN, divisor, 0, 0, depth, 1, stride, count };
		}

		NODISCARD static consteval RenderResourceDesc FixedBuffer(Uint32 stride, Uint32 count)
		{
			return RenderResourceDesc{ RenderResourceShape::Buffer, RenderResourceScale::Fixed, DXGI_FORMAT_UNKNOWN, 1, 0, 0, 1, 1, stride, count };
		}

		NODISCARD static consteval RenderResourceDesc Constant(Uint32 size)
		{
			return RenderResourceDesc{ RenderResourceShape::Constant, RenderResourceScale::Fixed, DXGI_FORMAT_UNKNOWN, 1, 0, 0, 1, 1, size, 1 };
		}

		NODISCARD static consteval RenderResourceDesc AccelerationStructure(Uint32 count)
		{
			return RenderResourceDesc{ RenderResourceShape::AccelerationStructure, RenderResourceScale::Fixed, DXGI_FORMAT_UNKNOWN, 1, 0, 0, 1, 1, 0, count };
		}
	};

	enum class RenderResourceScope :Uint8
	{
		View,
		Frame,
	};

	enum class RenderResourceKind :Uint8
	{
		Contract,
		Internal,
		Accumulation,
		Debug,
	};

	enum class RenderResourceLifetime :Uint8
	{
		Transient,
		Persistent,
		Imported,
		Upload,
	};

	enum class RenderFallback :Uint8
	{
		None,
		Zero,
		One,
		Alpha,
	};

	struct RenderResourceDeclaration
	{
		RenderResource resource_;

		RenderResourceDesc desc_;

		RenderResourceScope scope_ = RenderResourceScope::View;

		RenderResourceKind kind_ = RenderResourceKind::Internal;

		RenderResourceLifetime lifetime_ = RenderResourceLifetime::Transient;

		RenderFallback fallback_ = RenderFallback::None;

		RenderResource passThrough_ = {};
	};
}
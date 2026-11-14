#pragma once

namespace SeedCore
{
	enum class RenderFormat :Uint32
	{
        Unknown = DXGI_FORMAT_UNKNOWN,

        // 32-bit Components

        // RGBA32 (128-bit)
        RGBA32Typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS,
        RGBA32Float = DXGI_FORMAT_R32G32B32A32_FLOAT,
        RGBA32Uint = DXGI_FORMAT_R32G32B32A32_UINT,
        RGBA32Sint = DXGI_FORMAT_R32G32B32A32_SINT,

        // RGB32 (96-bit)
        RGB32Typeless = DXGI_FORMAT_R32G32B32_TYPELESS,
        RGB32Float = DXGI_FORMAT_R32G32B32_FLOAT,
        RGB32Uint = DXGI_FORMAT_R32G32B32_UINT,
        RGB32Sint = DXGI_FORMAT_R32G32B32_SINT,

        // RG32 (64-bit)
        RG32Typeless = DXGI_FORMAT_R32G32_TYPELESS,
        RG32Float = DXGI_FORMAT_R32G32_FLOAT,
        RG32Uint = DXGI_FORMAT_R32G32_UINT,
        RG32Sint = DXGI_FORMAT_R32G32_SINT,

        // R32 (32-bit)
        R32Typeless = DXGI_FORMAT_R32_TYPELESS,
        R32Float = DXGI_FORMAT_R32_FLOAT,
        R32Uint = DXGI_FORMAT_R32_UINT,
        R32Sint = DXGI_FORMAT_R32_SINT,

        // 16-bit Components

        // RGBA16 (64-bit)
        RGBA16Typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS,
        RGBA16Float = DXGI_FORMAT_R16G16B16A16_FLOAT,
        RGBA16Unorm = DXGI_FORMAT_R16G16B16A16_UNORM,
        RGBA16Uint = DXGI_FORMAT_R16G16B16A16_UINT,
        RGBA16Snorm = DXGI_FORMAT_R16G16B16A16_SNORM,
        RGBA16Sint = DXGI_FORMAT_R16G16B16A16_SINT,

        // RG16 (32-bit)
        RG16Typeless = DXGI_FORMAT_R16G16_TYPELESS,
        RG16Float = DXGI_FORMAT_R16G16_FLOAT,
        RG16Unorm = DXGI_FORMAT_R16G16_UNORM,
        RG16Uint = DXGI_FORMAT_R16G16_UINT,
        RG16Snorm = DXGI_FORMAT_R16G16_SNORM,
        RG16Sint = DXGI_FORMAT_R16G16_SINT,

        // R16 (16-bit)
        R16Typeless = DXGI_FORMAT_R16_TYPELESS,
        R16Float = DXGI_FORMAT_R16_FLOAT,
        R16Unorm = DXGI_FORMAT_R16_UNORM,
        R16Uint = DXGI_FORMAT_R16_UINT,
        R16Snorm = DXGI_FORMAT_R16_SNORM,
        R16Sint = DXGI_FORMAT_R16_SINT,

        // 8-bit Components

        // RGBA8 (32-bit)
        RGBA8Typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS,
        RGBA8Unorm = DXGI_FORMAT_R8G8B8A8_UNORM,
        SRGBA8Unorm = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        RGBA8Uint = DXGI_FORMAT_R8G8B8A8_UINT,
        RGBA8Snorm = DXGI_FORMAT_R8G8B8A8_SNORM,
        RGBA8Sint = DXGI_FORMAT_R8G8B8A8_SINT,

        // RG8 (16-bit)
        RG8Typeless = DXGI_FORMAT_R8G8_TYPELESS,
        RG8Unorm = DXGI_FORMAT_R8G8_UNORM,
        RG8Uint = DXGI_FORMAT_R8G8_UINT,
        RG8Snorm = DXGI_FORMAT_R8G8_SNORM,
        RG8Sint = DXGI_FORMAT_R8G8_SINT,

        // R8 (8-bit)
        R8Typeless = DXGI_FORMAT_R8_TYPELESS,
        R8Unorm = DXGI_FORMAT_R8_UNORM,
        R8Uint = DXGI_FORMAT_R8_UINT,
        R8Snorm = DXGI_FORMAT_R8_SNORM,
        R8Sint = DXGI_FORMAT_R8_SINT,

        // Alpha / Single-bit
        A8Unorm = DXGI_FORMAT_A8_UNORM,
        R1Unorm = DXGI_FORMAT_R1_UNORM,

        // Packed Color Formats

        // RGB10A2 (32-bit)
        RGB10A2Typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS,
        RGB10A2Unorm = DXGI_FORMAT_R10G10B10A2_UNORM,
        RGB10A2Uint = DXGI_FORMAT_R10G10B10A2_UINT,
        RGB10XrBiasA2Unorm = DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,

        // Packed HDR (32-bit)
        RG11B10Float = DXGI_FORMAT_R11G11B10_FLOAT,
        RGB9E5SharedExp = DXGI_FORMAT_R9G9B9E5_SHAREDEXP,

        // BGRA8 (32-bit)
        BGRA8Typeless = DXGI_FORMAT_B8G8R8A8_TYPELESS,
        BGRA8Unorm = DXGI_FORMAT_B8G8R8A8_UNORM,
        SBGRA8Unorm = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,

        // BGRX8 (32-bit)
        BGRX8Typeless = DXGI_FORMAT_B8G8R8X8_TYPELESS,
        BGRX8Unorm = DXGI_FORMAT_B8G8R8X8_UNORM,
        SBGRX8Unorm = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,

        // Packed 16-bit
        B5G6R5Unorm = DXGI_FORMAT_B5G6R5_UNORM,
        B5G5R5A1Unorm = DXGI_FORMAT_B5G5R5A1_UNORM,
        B4G4R4A4Unorm = DXGI_FORMAT_B4G4R4A4_UNORM,
        A4B4G4R4Unorm = DXGI_FORMAT_A4B4G4R4_UNORM,

        // Packed Multi-Pixel
        RGBG8Unorm = DXGI_FORMAT_R8G8_B8G8_UNORM,
        GRGB8Unorm = DXGI_FORMAT_G8R8_G8B8_UNORM,

        // Depth / Stencil Formats

        // Depth Only
        D32Float = DXGI_FORMAT_D32_FLOAT,
        D16Unorm = DXGI_FORMAT_D16_UNORM,

        // Depth 32-bit + Stencil 8-bit (64-bit)
        R32G8X24Typeless = DXGI_FORMAT_R32G8X24_TYPELESS,
        D32FloatS8X24Uint = DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        R32FloatX8X24Typeless = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
        X32TypelessG8X24Uint = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,

        // Depth 24-bit + Stencil 8-bit (32-bit)
        R24G8Typeless = DXGI_FORMAT_R24G8_TYPELESS,
        D24UnormS8Uint = DXGI_FORMAT_D24_UNORM_S8_UINT,
        R24UnormX8Typeless = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
        X24TypelessG8Uint = DXGI_FORMAT_X24_TYPELESS_G8_UINT,

        // Block Compression Formats

        // BC1
        BC1Typeless = DXGI_FORMAT_BC1_TYPELESS,
        BC1Unorm = DXGI_FORMAT_BC1_UNORM,
        BC1UnormSrgb = DXGI_FORMAT_BC1_UNORM_SRGB,

        // BC2
        BC2Typeless = DXGI_FORMAT_BC2_TYPELESS,
        BC2Unorm = DXGI_FORMAT_BC2_UNORM,
        BC2UnormSrgb = DXGI_FORMAT_BC2_UNORM_SRGB,

        // BC3
        BC3Typeless = DXGI_FORMAT_BC3_TYPELESS,
        BC3Unorm = DXGI_FORMAT_BC3_UNORM,
        BC3UnormSrgb = DXGI_FORMAT_BC3_UNORM_SRGB,

        // BC4
        BC4Typeless = DXGI_FORMAT_BC4_TYPELESS,
        BC4Unorm = DXGI_FORMAT_BC4_UNORM,
        BC4Snorm = DXGI_FORMAT_BC4_SNORM,

        // BC5
        BC5Typeless = DXGI_FORMAT_BC5_TYPELESS,
        BC5Unorm = DXGI_FORMAT_BC5_UNORM,
        BC5Snorm = DXGI_FORMAT_BC5_SNORM,

        // BC6H
        BC6HTypeless = DXGI_FORMAT_BC6H_TYPELESS,
        BC6HUF16 = DXGI_FORMAT_BC6H_UF16,
        BC6HSF16 = DXGI_FORMAT_BC6H_SF16,

        // BC7
        BC7Typeless = DXGI_FORMAT_BC7_TYPELESS,
        BC7Unorm = DXGI_FORMAT_BC7_UNORM,
        BC7UnormSrgb = DXGI_FORMAT_BC7_UNORM_SRGB,

        // Video / YUV Formats

        // Packed YUV
        AYUV = DXGI_FORMAT_AYUV,
        Y410 = DXGI_FORMAT_Y410,
        Y416 = DXGI_FORMAT_Y416,
        YUY2 = DXGI_FORMAT_YUY2,
        Y210 = DXGI_FORMAT_Y210,
        Y216 = DXGI_FORMAT_Y216,

        // Planar / Semi-Planar YUV
        NV12 = DXGI_FORMAT_NV12,
        NV11 = DXGI_FORMAT_NV11,
        P010 = DXGI_FORMAT_P010,
        P016 = DXGI_FORMAT_P016,
        P208 = DXGI_FORMAT_P208,
        V208 = DXGI_FORMAT_V208,
        V408 = DXGI_FORMAT_V408,
        Opaque420 = DXGI_FORMAT_420_OPAQUE,

        // Palettized Formats
        AI44 = DXGI_FORMAT_AI44,
        IA44 = DXGI_FORMAT_IA44,
        P8 = DXGI_FORMAT_P8,
        A8P8 = DXGI_FORMAT_A8P8,

        // Sampler Feedback Formats
        SamplerFeedbackMinMipOpaque = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
        SamplerFeedbackMipRegionUsedOpaque = DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
	};
}
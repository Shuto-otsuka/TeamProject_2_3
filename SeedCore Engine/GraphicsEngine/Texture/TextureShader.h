#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>

namespace SeedCore
{
	struct TextureUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Texture/TextureDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct TextureExpandDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Texture/TextureDrivenCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Expand";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct TextureSpriteDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Sprite/TextureSpriteMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Sprite/TextureSpriteVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Sprite/TextureSpritePS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 32;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 6;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::AlphaPremultiplied;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct TextureBillboardDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Billboard/TextureBillboardMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Billboard/TextureBillboardVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Texture/Billboard/TextureBillboardPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 32;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 6;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::D32Float;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::AlphaPremultiplied;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOnWriteOffReverseZ;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}

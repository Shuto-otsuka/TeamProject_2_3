#pragma once
#include <GraphicsEngine/RenderGraph/Pipeline/ComputePipeline.h>
#include <GraphicsEngine/RenderGraph/Pipeline/GraphicsPipeline.h>
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	struct PrimitiveSolidStructuredBuffer
	{
		Matrix world_;

		Matrix previousWorld_;

		Uint slot_ = 0;

		Uint flags_ = 0;

		Uint meshletOffset_ = 0;

		Uint meshletCount_ = 0;

		Vector2 texcoordMin_ = { 0.0f, 0.0f };

		Vector2 texcoordExtent_ = { 1.0f, 1.0f };

		Uint indexBufferIndex_ = SC_INVALID;

		Uint primitiveSolidStructuredBufferPadding0_ = 0;

		Uint primitiveSolidStructuredBufferPadding1_ = 0;

		Uint primitiveSolidStructuredBufferPadding2_ = 0;
	};
	SC_STATIC_ASSERT(PrimitiveSolidStructuredBuffer, 176, "Shape/Primitive/Solid/PrimitiveSolid.hlsli");

	struct PrimitiveSolidUpdate
	{
		Uint index_ = 0;

		Uint primitiveSolidUpdatePadding0_ = 0;

		Uint primitiveSolidUpdatePadding1_ = 0;

		Uint primitiveSolidUpdatePadding2_ = 0;

		PrimitiveSolidStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(PrimitiveSolidUpdate, 192, "Shape/Primitive/Solid/PrimitiveSolid.hlsli");

	struct PrimitiveMaterialUpdate
	{
		Uint index_ = 0;

		Uint primitiveMaterialUpdatePadding0_ = 0;

		Uint primitiveMaterialUpdatePadding1_ = 0;

		Uint primitiveMaterialUpdatePadding2_ = 0;

		ModelMaterialStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(PrimitiveMaterialUpdate, 288, "Shape/Primitive/Solid/PrimitiveSolid.hlsli");

	struct PrimitiveSolidConstantBuffer
	{
		Uint solidCount_ = 0;

		Uint transparentBase_ = 0;

		Uint capacity_ = 0;

		Uint vertexBufferIndex_ = SC_INVALID;

		Uint meshletBufferIndex_ = SC_INVALID;

		Uint meshletBoundBufferIndex_ = SC_INVALID;

		Uint vertexIndicesBufferIndex_ = SC_INVALID;

		Uint primitiveIndicesBufferIndex_ = SC_INVALID;

		Uint primitiveSolidConstantBufferPadding0_ = 0;

		Uint primitiveSolidConstantBufferPadding1_ = 0;

		Uint primitiveSolidConstantBufferPadding2_ = 0;

		Uint primitiveSolidConstantBufferPadding3_ = 0;
	};
	SC_STATIC_ASSERT(PrimitiveSolidConstantBuffer, 48, "Shape/Primitive/Solid/PrimitiveSolid.hlsli");

	struct PrimitiveWireframeStructuredBuffer
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		Quaternion rotation_ = Quaternion::Identity;

		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Uint32 shapeKind_ = 0;

		Float headLength_ = 0.0f;

		Float lineWidth_ = 0.0f;
	};
	SC_STATIC_ASSERT(PrimitiveWireframeStructuredBuffer, 68, "Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli");

	struct PrimitiveSolidUpdateDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Update";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct PrimitiveSolidExpandDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Expand";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct PrimitiveSolidRaytracingDescription
	{
		SC_CONSTEX ShaderPath file_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolidCS.hlsl");

		SC_CONSTEX const Char* entryPoint_ = "Raytracing";

		SC_CONSTEX Uint32 threadGroupSizeX_ = 64;

		SC_CONSTEX Uint32 threadGroupSizeY_ = 1;
	};

	struct PrimitiveWireframeDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframeMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframeVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframePS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 1;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 384;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Alpha;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};

	struct PrimitiveCanvasDescription
	{
		SC_CONSTEX ShaderPath meshFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Canvas/PrimitiveCanvasMS.hlsl");

		SC_CONSTEX ShaderPath vertexFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Canvas/PrimitiveCanvasVS.hlsl");

		SC_CONSTEX ShaderPath pixelFile_ = ShaderPath::Engine("GraphicsEngine/Shape/Primitive/Canvas/PrimitiveCanvasPS.hlsl");

		SC_CONSTEX GraphicsFallback fallback_ = GraphicsFallback::Instanced;

		SC_CONSTEX Uint32 instancesPerGroup_ = 1;

		SC_CONSTEX Uint32 fallbackVertexCount_ = 384;

		SC_CONSTEX StaticArray<RenderFormat, 1> targetFormat_ = { RenderFormat::RGBA16Float };

		SC_CONSTEX RenderFormat depthFormat_ = RenderFormat::Unknown;

		SC_CONSTEX BlendStateType blend_ = BlendStateType::Alpha;

		SC_CONSTEX DepthStencilStateType depthStencil_ = DepthStencilStateType::DepthOff;

		SC_CONSTEX RasterizerStateType rasterizer_ = RasterizerStateType::SolidNoneLHS;
	};
}

#pragma once
#include <GraphicsEngine/Texture/TextureFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct TextureFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool TextureFeature::* enableCondition_ = &TextureFeature::updating_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UnitUpdates),
			RenderAccess::Read(QuadUpdates),
			RenderAccess::Write(Units),
			RenderAccess::Write(Quads)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};

	struct TextureFeature::ExpandStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Quads),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(DrawList)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};

	struct TextureFeature::SpriteStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX RenderViewMask viewMask_ = static_cast<RenderViewMask>(static_cast<Uint8>(RenderViewMask::Game) | static_cast<Uint8>(RenderViewMask::Canvas));

		SC_CONSTEX Bool TextureFeature::* enableCondition_ = &TextureFeature::spriting_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Quads),
			RenderAccess::Read(DrawList),
			RenderAccess::Constant(Constants),
			RenderAccess::Target(HUDFeature::Color)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};

	struct TextureFeature::BillboardStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Bool TextureFeature::* enableCondition_ = &TextureFeature::billboarding_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Quads),
			RenderAccess::Read(DrawList),
			RenderAccess::Constant(Constants),
			RenderAccess::Target(RadianceFeature::Color),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};
}

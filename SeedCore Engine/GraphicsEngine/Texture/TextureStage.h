#pragma once
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>
#include <GraphicsEngine/Texture/TextureFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/PostProcess/Outline/OutlineFeature.h>
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

	struct TextureFeature::SubmitStage :RenderStage
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

	struct TextureFeature::DrawStage
	{
		struct SpriteStep;

		struct BillboardStep;

		using Sequence = RenderSequence<SpriteStep, BillboardStep>;
	};

	struct TextureFeature::DrawStage::SpriteStep :RenderStage
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
			RenderAccess::Read(OutlineFeature::Styles),
			RenderAccess::Write(OutlineFeature::Mask),
			RenderAccess::Target(HUDFeature::Color)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};

	struct TextureFeature::DrawStage::BillboardStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Particle;

		SC_CONSTEX Bool TextureFeature::* enableCondition_ = &TextureFeature::billboarding_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Units),
			RenderAccess::Read(Quads),
			RenderAccess::Read(DrawList),
			RenderAccess::Constant(Constants),
			RenderAccess::Read(RaytracingVolumetricLightFeature::FogVolume),
			RenderAccess::Read(OutlineFeature::Styles),
			RenderAccess::Write(OutlineFeature::Mask),
			RenderAccess::Target(RadianceFeature::Color),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(TextureFeature& feature, RenderPass& pass);
	};
}

#pragma once
#include <GraphicsEngine/Debug/DebugViewFeature.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionFeature.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionFeature.h>
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct DebugViewFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::clearing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(RayCount),
			RenderAccess::Write(GeometryFeature::Debug)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};

	struct DebugViewFeature::DrawStage
	{
		struct WireframeStep;

		struct OverdrawStep;

		using Sequence = RenderSequence<WireframeStep, OverdrawStep>;
	};

	struct DebugViewFeature::DrawStage::WireframeStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Diagnostic;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::wireframing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Write(GeometryFeature::Debug),
			RenderAccess::Target(HUDFeature::DisplayColor)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};

	struct DebugViewFeature::DrawStage::OverdrawStep :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Diagnostic;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::overdrawing_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::MorphWeights),
			RenderAccess::Write(GeometryFeature::Debug),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};

	struct DebugViewFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Diagnostic;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::resolving_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(GeometryFeature::Debug),
			RenderAccess::Read(MaterialFeature::SurfaceBaseColor),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Read(MaterialFeature::SurfaceRadiance),
			RenderAccess::Read(MaterialFeature::Mip),
			RenderAccess::Read(LightFeature::ClusterGrid),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Read(RayCount),
			RenderAccess::Read(RaytracingReflectionFeature::Raw),
			RenderAccess::Read(RaytracingReflectionFeature::Radiance),
			RenderAccess::Read(RaytracingReflectionFeature::Reservoir),
			RenderAccess::Read(RaytracingReflectionFeature::Reuse),
			RenderAccess::Read(RaytracingReflectionFeature::Moments),
			RenderAccess::Read(RaytracingReflectionFeature::HistoryLength),
			RenderAccess::Read(RaytracingReflectionFeature::Validity),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Raw),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Radiance),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Reservoir),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Reuse),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Variance),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::HistoryLength),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Validity),
			RenderAccess::Read(RaytracingShadowFeature::Raw),
			RenderAccess::Read(RaytracingShadowFeature::Visibility),
			RenderAccess::Read(RaytracingShadowFeature::Moments),
			RenderAccess::Read(RaytracingShadowFeature::HistoryLength),
			RenderAccess::Read(RaytracingShadowFeature::Validity),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::Raw),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::Openness),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::Variance),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::HistoryLength),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::Validity),
			RenderAccess::Read(RadianceFeature::Color),
			RenderAccess::Read(UpscaleFeature::Color),
			RenderAccess::Read(KawaseBloomFeature::Radiance),
			RenderAccess::Read(AutoExposureFeature::Exposure),
			RenderAccess::Write(HUDFeature::DisplayColor)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};

	struct DebugViewFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Diagnostic;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::bounding_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(CullingFeature::DrawLists),
			RenderAccess::Read(CullingFeature::DrawArguments),
			RenderAccess::Write(HUDFeature::DisplayColor)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};
}

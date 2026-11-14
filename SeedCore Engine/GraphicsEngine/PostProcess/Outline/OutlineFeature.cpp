#include <GraphicsEngine/PostProcess/Outline/OutlineFeature.h>
#include <GraphicsEngine/PostProcess/Outline/OutlineStage.h>
#include <GraphicsEngine/PostProcess/Outline/Outline.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/Selected.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(OutlineFeature, UpdateStage, ResolveStage, ComposeStage);

	void OutlineFeature::Create(RenderDevice& device)
	{
		stylePipeline_.Create(device);
		maskPipeline_.Create(device);
		resolvePipeline_.Create(device);
		canvasPipeline_.Create(device);
		composePipeline_.Create(device);
		styles_.resize(ModelUnit::capacity_);
		pending_.resize(ModelUnit::capacity_);
	}

	void OutlineFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		updates_.clear();
		current_.clear();
		World& world = *scene.world_;

		Query<Read<Active>, Read<Outline>>(world).ForEach([&](EntityID entityID, const Active& active, const Outline& outline)
			{
				if (!active.active_ || entityID.index_ >= ModelUnit::capacity_ || outline.width_ <= 0.0f)
				{
					return;
				}
				OutlineStyleStructuredBuffer& style = pending_[entityID.index_];
				if (style.flags_ == 0)
				{
					current_.push_back(entityID.index_);
				}
				style.color_ = static_cast<Uint>(Clamp(outline.color_.r, 0.0f, 1.0f) * 255.0f + 0.5f) | (static_cast<Uint>(Clamp(outline.color_.g, 0.0f, 1.0f) * 255.0f + 0.5f) << 8) | (static_cast<Uint>(Clamp(outline.color_.b, 0.0f, 1.0f) * 255.0f + 0.5f) << 16) | (static_cast<Uint>(Clamp(outline.color_.a, 0.0f, 1.0f) * 255.0f + 0.5f) << 24);
				style.width_ = outline.width_;
				style.flags_ |= outline.shape_ == OutlineShape::Contour ? 3u : 1u;
			});

		Query<Read<Selected>>(world).ForEach([&](EntityID entityID, const Selected&)
			{
				if (entityID.index_ >= ModelUnit::capacity_)
				{
					return;
				}
				OutlineStyleStructuredBuffer& style = pending_[entityID.index_];
				if (style.flags_ == 0)
				{
					current_.push_back(entityID.index_);
				}
				style.flags_ |= 4u;
			});

		for (Uint32 actor : actors_)
		{
			if (pending_[actor].flags_ == 0 && styles_[actor].flags_ != 0)
			{
				styles_[actor] = OutlineStyleStructuredBuffer{};
				OutlineStyleUpdate& update = updates_.emplace_back();
				update.index_ = actor;
			}
		}
		for (Uint32 actor : current_)
		{
			OutlineStyleStructuredBuffer& style = pending_[actor];
			if (std::memcmp(&style, &styles_[actor], sizeof(OutlineStyleStructuredBuffer)) != 0)
			{
				styles_[actor] = style;
				OutlineStyleUpdate& update = updates_.emplace_back();
				update.index_ = actor;
				update.data_ = style;
			}
			style = OutlineStyleStructuredBuffer{};
		}
		std::swap(actors_, current_);

		updating_ = !updates_.empty();
		outlining_ = !actors_.empty();
	}

	void OutlineFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (updating_)
		{
			upload.Write(Updates, RenderViewKind::Game, std::span<const OutlineStyleUpdate>(updates_));
		}
	}
}

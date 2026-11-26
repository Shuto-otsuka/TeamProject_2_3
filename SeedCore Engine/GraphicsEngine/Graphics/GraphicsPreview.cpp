#include <GraphicsEngine/Graphics/GraphicsPreview.h>

#include <GraphicsEngine/Camera/PreviewCamera.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/Preview/PreviewFeature.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderGraphMemory.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/SceneRenderer.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/Screen/Boot/BootFeature.h>
#include <GraphicsEngine/Screen/Splash/SplashFeature.h>

namespace SeedCore
{
	GraphicsPreview::GraphicsPreview(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	GraphicsPreview::~GraphicsPreview() = default;

	void GraphicsPreview::Finalize()
	{
		for (Slot& slot : slots_)
		{
			RenderGraphMemory::Release(device_, slot.output_);
			slot.renderer_.reset();
		}
	}

	void GraphicsPreview::Add(PreviewSlot slot, const PreviewRequest& request, const PreviewCamera& camera, Float totalTime)
	{
		Slot& entry = slots_[static_cast<Size>(slot)];
		entry.requested_ = true;
		entry.request_ = request;

		Uint32 width = Max(static_cast<Uint32>(camera.Width()), 1u);
		Uint32 height = Max(static_cast<Uint32>(camera.Height()), 1u);

		RenderView& view = entry.view_;
		view = RenderView{};
		view.kind_ = RenderViewKind::Editor;
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = width;
		view.renderHeight_ = height;
		view.displayWidth_ = width;
		view.displayHeight_ = height;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(width), static_cast<Float>(height));
		view.hasActiveCamera_ = true;

		SceneConstantBuffer& scene = view.scene_;
		scene = SceneConstantBuffer{};
		scene.view_ = camera.View();
		scene.inverseView_ = camera.InverseView();
		scene.projection_ = camera.Projection();
		scene.inverseProjection_ = camera.InverseProjection();
		scene.nonJitterProjection_ = camera.NonJitterProjection();
		scene.currentViewProjection_ = camera.CurrentViewProjection();
		scene.previousViewProjection_ = camera.PreviousViewProjection();
		scene.inverseViewProjection_ = camera.InverseViewProjection();
		scene.nonJitterViewProjection_ = camera.NonJitterViewProjection();
		scene.previousNonJitterViewProjection_ = camera.NonJitterViewProjection();
		scene.cameraPosition_ = Vector4(camera.Eye(), 1.0f);
		scene.cameraFocus_ = Vector4(camera.Focus(), 1.0f);
		scene.fieldOfView_ = camera.Fov();
		scene.nearPlane_ = camera.Near();
		scene.farPlane_ = camera.Far();
		scene.totalTime_ = totalTime;
		scene.screenSize_ = Vector2(static_cast<Float>(width), static_cast<Float>(height));
		scene.inverseScreenSize_ = Vector2(1.0f / width, 1.0f / height);
		scene.displaySize_ = scene.screenSize_;
	}

	void GraphicsPreview::Add(const BootRequest& request, Uint32 width, Uint32 height)
	{
		Slot& entry = slots_[static_cast<Size>(PreviewSlot::Boot)];
		entry.requested_ = true;
		entry.boot_ = request;

		width = Max(width, 1u);
		height = Max(height, 1u);

		RenderView& view = entry.view_;
		view = RenderView{};
		view.kind_ = RenderViewKind::Editor;
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = width;
		view.renderHeight_ = height;
		view.displayWidth_ = width;
		view.displayHeight_ = height;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(width), static_cast<Float>(height));
	}

	void GraphicsPreview::Render()
	{
		D3D12CommandList& cmdList = *device_.Context().GetDirectList();
		for (Slot& slot : slots_)
		{
			if (!slot.requested_)
			{
				continue;
			}
			slot.requested_ = false;

			Bool boot = &slot == &slots_[static_cast<Size>(PreviewSlot::Boot)];
			if (!slot.renderer_)
			{
				slot.renderer_ = MakePtr<SceneRenderer>(device_);
				slot.renderer_->Initialize(boot ? RenderFeatureSet::Screen : RenderFeatureSet::Preview, 8ull * 1024 * 1024);
			}

			const RenderView& view = slot.view_;
			RenderGraphAllocation& output = slot.output_;
			if (!output.resource_ || output.width_ != view.displayWidth_ || output.height_ != view.displayHeight_)
			{
				RenderGraphMemory::Release(device_, output);

				RenderGraphResource resource;
				resource.resourceName_ = String(PreviewFeature::Output.Name());
				resource.desc_.shape_ = RenderResourceShape::Texture2D;
				resource.desc_.scale_ = RenderResourceScale::Fixed;
				resource.desc_.format_ = RenderFormat::RGBA8Unorm;
				resource.shaderResource_ = true;
				resource.renderTarget_ = true;
				RenderGraphInstance instance;
				instance.width_ = view.displayWidth_;
				instance.height_ = view.displayHeight_;
				output = RenderGraphMemory::Create(device_, resource, instance);

				StaticArray<Float, 4> clear = { 0.0f, 0.0f, 0.0f, 0.0f };
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList.Get()->ClearRenderTargetView(device_.RenderTargetHeap().CPUHandle(output.renderTargetView_), clear.data(), 0, nullptr);
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				output.state_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}

			DynamicArray<RenderGraphImport> imports;
			RenderScene scene;
			scene.resource_ = &module_.Resources();
			if (boot)
			{
				imports.push_back(RenderGraphImport{ String(BootFeature::Output.Name()), view.kind_, output });
				imports.push_back(RenderGraphImport{ String(SplashFeature::Output.Name()), view.kind_, output });
				scene.Set(slot.boot_);
			}
			else
			{
				imports.push_back(RenderGraphImport{ String(PreviewFeature::Output.Name()), view.kind_, output });
				scene.Set(slot.request_);
			}
			scene.views_ = std::span<const RenderView>(&view, 1);
			scene.imports_ = imports;
			slot.renderer_->Render(scene);
		}
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GraphicsPreview::Display(PreviewSlot slot)const
	{
		const RenderGraphAllocation& output = slots_[static_cast<Size>(slot)].output_;
		if (!output.resource_)
		{
			return D3D12_GPU_DESCRIPTOR_HANDLE{};
		}
		return device_.ResourceHeap().GPUHandle(output.shaderResourceView_);
	}
}

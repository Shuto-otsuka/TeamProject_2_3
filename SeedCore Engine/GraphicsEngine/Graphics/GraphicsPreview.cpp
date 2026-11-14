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
	/**
	* [EN]
	* Keeps the device and the asset module. Nothing touches the GPU until a slot is first asked for.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイスとアセットモジュールを受け取る。スロットが初めて求められるまで GPU には触らない。
	*/
	GraphicsPreview::GraphicsPreview(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	GraphicsPreview::~GraphicsPreview() = default;

	/**
	* [EN]
	* Releases every slot's output texture and renderer; the device must be idle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 全スロットの出力テクスチャとレンダラーを解放する。デバイスは待機済みであること。
	*/
	void GraphicsPreview::Finalize()
	{
		for (Slot& slot : slots_)
		{
			RenderGraphMemory::Release(device_, slot.output_);
			slot.renderer_.reset();
		}
	}

	/**
	* [EN]
	* Asks for slot to be drawn this frame with request, seen from camera at totalTime.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* slot をこのフレームに request で描くよう求める。camera から totalTime の時刻で見る。
	*/
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
		scene.cameraPosition_ = Vector4(camera.Eye().x, camera.Eye().y, camera.Eye().z, 1.0f);
		scene.cameraFocus_ = Vector4(camera.Focus().x, camera.Focus().y, camera.Focus().z, 1.0f);
		scene.fieldOfView_ = camera.Fov();
		scene.nearPlane_ = camera.Near();
		scene.farPlane_ = camera.Far();
		scene.totalTime_ = totalTime;
		scene.screenSize_ = Vector2(static_cast<Float>(width), static_cast<Float>(height));
		scene.inverseScreenSize_ = Vector2(1.0f / width, 1.0f / height);
		scene.displaySize_ = scene.screenSize_;
	}

	/**
	* [EN]
	* Asks for the boot screen slot to be drawn this frame with request, at a screen of width by height.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ブート画面のスロットをこのフレームに request で描くよう求める。画面の大きさは width × height。
	*/
	void GraphicsPreview::Add(const BootRequest& request, Uint32 width, Uint32 height)
	{
		Slot& entry = slots_[static_cast<Size>(PreviewSlot::Boot)];
		entry.requested_ = true;
		entry.boot_ = request;

		width = Max(width, 1u);
		height = Max(height, 1u);

		/// [EN] The boot screen reads only the view's size, so the view carries no camera.
		/// [JP] ブート画面はビューの大きさだけを読むので、ビューにカメラは持たせない。
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

	/**
	* [EN]
	* Draws every slot asked for since the last call; later calls in the same frame do nothing.
	* A slot's renderer is made on its first request and its output texture whenever the view's
	* size changes, cleared once and left ready for the panel to sample.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の呼び出しから求められた全スロットを描く。同じフレームの 2 回目以降は何もしない。
	* スロットのレンダラーは最初の求めで作り、出力テクスチャはビューの大きさが変わるたびに
	* 作り、1 回クリアしてパネルが読める状態にしておく。
	*/
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

			/// [EN] The boot screen slot draws with the Screen feature set, the same features that draw it on the back buffer before the game starts.
			/// [JP] ブート画面のスロットは、ゲーム開始前にバックバッファへ描くのと同じ Screen の組の Feature で描く。
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

	/**
	* [EN]
	* Returns the handle of slot's output texture, or a null handle before it is first drawn.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* slot の出力テクスチャのハンドルを返す。初めて描くまでは空のハンドル。
	*/
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

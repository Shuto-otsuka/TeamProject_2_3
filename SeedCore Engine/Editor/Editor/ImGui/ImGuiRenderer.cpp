#include <Editor/Editor/ImGui/ImGuiRenderer.h>
#include <Editor/Editor/ImGui/ImGuiCommon.h>
#include <FoundationEngine/Interop/ImGuiInstance.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/FrameRing.h>
#include <FoundationEngine/Application/Window.h>

namespace SeedCore
{
	Bool ImGuiRenderer::Initialize(Window& window, Graphics& graphics)
	{
		HWND hwnd = window.Handle();
		ID3D12Device* device = graphics.Device().Context().GetDevice();
		ID3D12CommandQueue* commandQueue = graphics.Device().Context().GetDirectQueue()->GetCommandQueue();
		BindlessHeap* bindlessHeap = &graphics.Device().ResourceHeap();
		Int numberFramesInFlight = static_cast<Int>(FrameRing::frameCount);

		bindlessHeap_ = bindlessHeap;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiInstance::Bind(ImGui::GetCurrentContext());

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		if (!ImGui_ImplWin32_Init(hwnd))
		{
			return false;
		}

		ImGui_ImplDX12_InitInfo initInfo{};
		initInfo.Device = device;
		initInfo.CommandQueue = commandQueue;
		initInfo.NumFramesInFlight = numberFramesInFlight;
		initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
		initInfo.UserData = bindlessHeap;
		initInfo.SrvDescriptorHeap = bindlessHeap->Heap();
		initInfo.SrvDescriptorAllocFn = &ImGuiRenderer::AllocateDescriptor;
		initInfo.SrvDescriptorFreeFn = &ImGuiRenderer::FreeDescriptor;
		if (!ImGui_ImplDX12_Init(&initInfo))
		{
			return false;
		}

		if (!FontConfig(io))
		{
			return false;
		}
		StyleConfig();

		if (!ImGui_ImplDX12_CreateDeviceObjects())
		{
			return false;
		}

		window.Handler(&ImGuiRenderer::HandleMessage);
		return true;
	}

	void ImGuiRenderer::NewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiRenderer::Render(ID3D12GraphicsCommandList* cmdList)
	{
		ImGui::Render();

		ID3D12DescriptorHeap* descHeaps[] = { bindlessHeap_->Heap() };
		cmdList->SetDescriptorHeaps(1, descHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiRenderer::Finalize()
	{
		if (ImGui::GetCurrentContext())
		{
			ImGuiIO& io = ImGui::GetIO();
			io.Fonts->Clear();
		}

		ImGui_ImplDX12_InvalidateDeviceObjects();
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		bindlessHeap_ = nullptr;
	}

	void ImGuiRenderer::DockSpaceBegin(Float topOffset)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + topOffset));
		ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - topOffset));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("MainDockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		dockSpaceID_ = ImGui::GetID("ScDockSpace");
		ImGui::DockSpace(dockSpaceID_, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	}

	void ImGuiRenderer::DockSpaceEnd()
	{
		ImGui::End();
	}

	ImGuiID ImGuiRenderer::DockSpaceID()const
	{
		return dockSpaceID_;
	}

	void ImGuiRenderer::FontScale(Float scale)
	{
		fontScale_ = scale;
		ImGui::GetIO().FontGlobalScale = fontScale_;
	}

	Float ImGuiRenderer::FontScale()const
	{
		return fontScale_;
	}

	LRESULT ImGuiRenderer::HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (ImGui::GetCurrentContext() == nullptr)
		{
			return 0;
		}

		LRESULT result = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

		if (msg == WM_SETFOCUS)
		{
			ImGui::GetIO().ClearInputKeys();
		}

		return result;
	}

	void ImGuiRenderer::AllocateDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
	{
		BindlessHeap* bindlessHeap = static_cast<BindlessHeap*>(info->UserData);
		Uint index = bindlessHeap->AllocateIndex();
		*cpuHandle = bindlessHeap->CPUHandle(index);
		*gpuHandle = bindlessHeap->GPUHandle(index);
	}

	void ImGuiRenderer::FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE)
	{
		BindlessHeap* bindlessHeap = static_cast<BindlessHeap*>(info->UserData);
		bindlessHeap->Release(nullptr, bindlessHeap->Index(cpuHandle));
	}

	Bool ImGuiRenderer::FontConfig(ImGuiIO& io)
	{
		ImFontConfig fontConfig{};
		fontConfig.OversampleH = 1;
		fontConfig.OversampleV = 1;
		fontConfig.PixelSnapH = false;

		const ImWchar* japaneseFullRange = io.Fonts->GetGlyphRangesChineseFull();
		io.Fonts->AddFontFromFileTTF("../External/ImGui/Font/NotoSansJP-Regular.ttf", 17.6f, &fontConfig, japaneseFullRange);
		return io.Fonts->Build();
	}

	void ImGuiRenderer::StyleConfig()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.6f;
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.FramePadding = ImVec2(8.0f, 5.0f);
		style.CellPadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
		style.IndentSpacing = 18.0f;
		style.ScrollbarSize = 11.0f;
		style.ScrollbarPadding = 3.0f;
		style.GrabMinSize = 10.0f;

		style.WindowRounding = 0.0f;
		style.ChildRounding = 2.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 3.0f;
		style.ScrollbarRounding = 2.0f;
		style.GrabRounding = 1.0f;
		style.TabRounding = 2.0f;
		style.ImageRounding = 0.0f;
		style.MenuItemRounding = 1.0f;
		style.SelectableRounding = 0.0f;

		style.WindowBorderSize = 1.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.TabBorderSize = 1.0f;
		style.TabBarBorderSize = 1.0f;
		style.TabBarOverlineSize = 2.0f;
		style.DockingSeparatorSize = 4.0f;
		style.SeparatorSize = 1.0f;
		style.SeparatorTextBorderSize = 1.0f;
		style.SeparatorTextPadding = ImVec2(8.0f, 5.0f);
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.InputTextCursorSize = 1.0f;
		style.DragDropTargetBorderSize = 1.0f;
		style.DragDropTargetRounding = 2.0f;

		const auto color = [](Uint rgb, Float alpha = 1.0f) -> ImVec4
		{
			return ImVec4(static_cast<Float>((rgb >> 16) & 0xFF) / 255.0f, static_cast<Float>((rgb >> 8) & 0xFF) / 255.0f, static_cast<Float>(rgb & 0xFF) / 255.0f, alpha);
		};

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = color(0xE4ECFA);
		colors[ImGuiCol_TextDisabled] = color(0x8392AD);
		colors[ImGuiCol_WindowBg] = color(0x131A27);
		colors[ImGuiCol_ChildBg] = color(0x131A27, 0.0f);
		colors[ImGuiCol_PopupBg] = color(0x1A2438);
		colors[ImGuiCol_Border] = color(0x5C7295);
		colors[ImGuiCol_BorderShadow] = color(0x000000, 0.0f);
		colors[ImGuiCol_FrameBg] = color(0x0D1422);
		colors[ImGuiCol_FrameBgHovered] = color(0x1F2D4B);
		colors[ImGuiCol_FrameBgActive] = color(0x263B71);
		colors[ImGuiCol_TitleBg] = color(0x0D1420);
		colors[ImGuiCol_TitleBgActive] = color(0x1D2A48);
		colors[ImGuiCol_TitleBgCollapsed] = color(0x0D1420);
		colors[ImGuiCol_MenuBarBg] = color(0x141D2E);
		colors[ImGuiCol_ScrollbarBg] = color(0x0D1420);
		colors[ImGuiCol_ScrollbarGrab] = color(0x3B5077);
		colors[ImGuiCol_ScrollbarGrabHovered] = color(0x5773AD);
		colors[ImGuiCol_ScrollbarGrabActive] = color(0x657EFF);
		colors[ImGuiCol_CheckMark] = color(0xA7BCFF);
		colors[ImGuiCol_CheckboxSelectedBg] = color(0x26386B);
		colors[ImGuiCol_SliderGrab] = color(0x657EFF);
		colors[ImGuiCol_SliderGrabActive] = color(0xA7BCFF);
		colors[ImGuiCol_Button] = color(0x1D2941);
		colors[ImGuiCol_ButtonHovered] = color(0x2D4184);
		colors[ImGuiCol_ButtonActive] = color(0x3E52AF);
		colors[ImGuiCol_Header] = color(0x293D78);
		colors[ImGuiCol_HeaderHovered] = color(0x304989);
		colors[ImGuiCol_HeaderActive] = color(0x3E52AF);
		colors[ImGuiCol_Separator] = color(0x4A5E80);
		colors[ImGuiCol_SeparatorHovered] = color(0x657EFF);
		colors[ImGuiCol_SeparatorActive] = color(0xA7BCFF);
		colors[ImGuiCol_ResizeGrip] = color(0x657EFF, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = color(0x657EFF, 0.70f);
		colors[ImGuiCol_ResizeGripActive] = color(0xA7BCFF);
		colors[ImGuiCol_InputTextCursor] = color(0xA7BCFF);
		colors[ImGuiCol_TabHovered] = color(0x2D4184);
		colors[ImGuiCol_Tab] = color(0x121B2D);
		colors[ImGuiCol_TabSelected] = color(0x293D78);
		colors[ImGuiCol_TabSelectedOverline] = color(0x8FA7FF);
		colors[ImGuiCol_TabDimmed] = color(0x101725);
		colors[ImGuiCol_TabDimmedSelected] = color(0x1D2A45);
		colors[ImGuiCol_TabDimmedSelectedOverline] = color(0x5773AD);
		colors[ImGuiCol_DockingPreview] = color(0x657EFF, 0.32f);
		colors[ImGuiCol_DockingEmptyBg] = color(0x080E17);
		colors[ImGuiCol_PlotLines] = color(0x7FA8FF);
		colors[ImGuiCol_PlotLinesHovered] = color(0xB4C9FF);
		colors[ImGuiCol_PlotHistogram] = color(0x657EFF);
		colors[ImGuiCol_PlotHistogramHovered] = color(0xA7BCFF);
		colors[ImGuiCol_TableHeaderBg] = color(0x1C2940);
		colors[ImGuiCol_TableBorderStrong] = color(0x5C7295);
		colors[ImGuiCol_TableBorderLight] = color(0x354765);
		colors[ImGuiCol_TableRowBg] = color(0x131A27, 0.0f);
		colors[ImGuiCol_TableRowBgAlt] = color(0xD5E3FF, 0.025f);
		colors[ImGuiCol_TextLink] = color(0x94B4FF);
		colors[ImGuiCol_TextSelectedBg] = color(0x5A72FF, 0.35f);
		colors[ImGuiCol_TreeLines] = color(0x4A5E80);
		colors[ImGuiCol_DragDropTarget] = color(0xA7BCFF);
		colors[ImGuiCol_DragDropTargetBg] = color(0x657EFF, 0.14f);
		colors[ImGuiCol_UnsavedMarker] = color(0x8FA7FF);
		colors[ImGuiCol_NavCursor] = color(0xA7BCFF);
		colors[ImGuiCol_NavWindowingHighlight] = color(0x94B4FF, 0.80f);
		colors[ImGuiCol_NavWindowingDimBg] = color(0x060A12, 0.55f);
		colors[ImGuiCol_ModalWindowDimBg] = color(0x060A12, 0.72f);
	}
}

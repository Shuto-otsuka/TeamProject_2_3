#pragma once

struct ImGui_ImplDX12_InitInfo;

namespace SeedCore
{
	class BindlessHeap;
	class Graphics;
	class Window;

	class ImGuiRenderer
	{
	public:
		ImGuiRenderer() = default;

		~ImGuiRenderer() = default;

		Bool Initialize(Window& window, Graphics& graphics);

		void NewFrame();

		void Render(ID3D12GraphicsCommandList* cmdList);

		void Finalize();

	public:
		void DockSpaceBegin(Float topOffset = 0.0f);

		void DockSpaceEnd();

		NODISCARD ImGuiID DockSpaceID()const;

	public:
		void FontScale(Float scale);

		Float FontScale()const;

	public:
		static LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	private:
		static void AllocateDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);

		static void FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	private:
		Bool FontConfig(ImGuiIO& io);

		void StyleConfig();

	private:
		BindlessHeap* bindlessHeap_ = nullptr;

		Float fontScale_ = 1.0f;

		ImGuiID dockSpaceID_ = 0;
	};
}

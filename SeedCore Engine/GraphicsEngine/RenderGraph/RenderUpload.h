#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/D3D12/FrameRing.h>

namespace SeedCore
{
	class RenderDevice;
	class StagingBuffer;

	struct RenderUploadEntry
	{
		String resourceName_;

		RenderViewKind view_ = RenderViewKind::Game;

		Uint64 offset_ = 0;

		Uint64 size_ = 0;
	};

	class RenderUpload :public NonTransferable
	{
	public:
		RenderUpload();

		~RenderUpload();

		void Initialize(RenderDevice& device, Uint64 capacity);

		void Reset(Uint32 frame);

		void Write(const RenderResource& resource, RenderViewKind view, std::span<const std::byte> data, Uint64 alignment);

		template<class T>
		void Write(const RenderResource& resource, RenderViewKind view, std::span<const T> values)
		{
			Write(resource, view, std::as_bytes(values), std::lcm<Uint64>(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(T)));
		}

		NODISCARD const RenderUploadEntry* Find(const String& resourceName, RenderViewKind view)const;

		NODISCARD StagingBuffer& Buffer()const;

	private:
		StaticArray<ResourcePtr<StagingBuffer>, FrameRing::frameCount> buffers_;

		DynamicArray<RenderUploadEntry> entries_;

		Uint32 current_ = 0;
	};
}
#pragma once
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>

namespace SeedCore
{
	struct SceneConstantBuffer
	{
		Matrix view_;
		Matrix inverseView_;

		Matrix projection_;
		Matrix inverseProjection_;
		Matrix nonJitterProjection_;

		Matrix currentViewProjection_;
		Matrix previousViewProjection_;
		Matrix inverseViewProjection_;
		Matrix nonJitterViewProjection_;

		Matrix previousNonJitterViewProjection_;

		Vector4 cameraPosition_;
		Vector4 cameraFocus_;

		Float fieldOfView_;
		Float nearPlane_;
		Float farPlane_;
		Uint viewMode_;

		Float totalTime_;
		Float deltaTime_;

		Vector2 displaySize_;

		Vector2 screenSize_;
		Vector2 inverseScreenSize_;

		Uint showFlagsLow_ = 0xFFFFFFFF;
		Uint showFlagsHigh_ = 0xFFFFFFFF;
		Uint debugSource_ = 0;
		Uint viewKind_ = 0;
	};

	class BindlessHeap;

	class SEEDCORE_API SceneConstantSystem
	{
	public:
		SceneConstantSystem(ID3D12Device* device, BindlessHeap* bindlessHeap);
		~SceneConstantSystem();

		void Upload(SceneConstantBuffer buffer);

		Uint GetIndex()const;

	private:
		ResourcePtr<StaticConstantBuffer<SceneConstantBuffer>> sceneConstantBuffer_;

	};
}
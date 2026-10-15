#include <GraphicsEngine/System/SceneConstantSystem.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	SceneConstantSystem::SceneConstantSystem(ID3D12Device* device, BindlessHeap* bindlessHeap)
	{
		sceneConstantBuffer_ = MakePtr<StaticConstantBuffer<SceneConstantBuffer>>(device, bindlessHeap);
	}

	SceneConstantSystem::~SceneConstantSystem()
	{
		if(sceneConstantBuffer_)
		{
			sceneConstantBuffer_.reset();
			sceneConstantBuffer_ = nullptr;
		}
	}

	void SceneConstantSystem::Upload(SceneConstantBuffer buffer)
	{
		sceneConstantBuffer_->Update(buffer);
	}

	/// [EN] Frame-ring buffer: the index changes per frame — never cache it.
	/// [JP] フレームリングバッファ: インデックスは毎フレーム変わる — キャッシュ禁止。
	Uint SceneConstantSystem::GetIndex()const
	{
		return sceneConstantBuffer_->Index();
	}
}
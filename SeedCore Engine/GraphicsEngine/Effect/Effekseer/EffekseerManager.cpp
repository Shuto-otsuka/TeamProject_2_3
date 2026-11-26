#include <External/Effekseer/Include/Effekseer/Model/Effekseer.ProceduralModelGenerator.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	Bool EffekseerManager::Initialize(D3D12Context& context)
	{
		DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		renderer_ = EffekseerRendererDX12::Create(context.GetDevice(), context.GetDirectQueue()->GetCommandQueue(), 3, &renderTargetFormat, 1, DXGI_FORMAT_D32_FLOAT, true, 20000);
		if (renderer_ == nullptr)
		{
			return false;
		}

		memoryPool_ = EffekseerRenderer::CreateSingleFrameMemoryPool(renderer_->GetGraphicsDevice());
		cmdList_ = EffekseerRenderer::CreateCommandList(renderer_->GetGraphicsDevice(), memoryPool_);
		renderer_->SetCommandList(cmdList_);
		renderer_->SetMaintainGammaColorInLinearColorSpace(true);

		manager_ = Effekseer::Manager::Create(20000);
		manager_->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

		manager_->SetSpriteRenderer(renderer_->CreateSpriteRenderer());
		manager_->SetRibbonRenderer(renderer_->CreateRibbonRenderer());
		manager_->SetRingRenderer(renderer_->CreateRingRenderer());
		manager_->SetTrackRenderer(renderer_->CreateTrackRenderer());
		manager_->SetModelRenderer(renderer_->CreateModelRenderer());

		textureLoader_ = Effekseer::MakeRefPtr<EffekseerTextureLoader>(renderer_->GetGraphicsDevice());
		modelLoader_ = Effekseer::MakeRefPtr<EffekseerModelLoader>(renderer_->CreateModelLoader());
		materialLoader_ = Effekseer::MakeRefPtr<EffekseerMaterialLoader>(renderer_->CreateMaterialLoader());
		curveLoader_ = Effekseer::MakeRefPtr<EffekseerCurveLoader>();

		cacheSetting_ = Effekseer::Setting::Create();
		cacheSetting_->SetCoordinateSystem(manager_->GetSetting()->GetCoordinateSystem());
		cacheSetting_->SetProceduralMeshGenerator(manager_->GetSetting()->GetProceduralMeshGenerator());
		cacheSetting_->SetGpuParticleFactory(manager_->GetSetting()->GetGpuParticleFactory());
		cacheSetting_->SetTextureLoader(textureLoader_);
		cacheSetting_->SetModelLoader(modelLoader_);
		cacheSetting_->SetMaterialLoader(materialLoader_);
		cacheSetting_->SetCurveLoader(curveLoader_);

		Gateway::BindEffekseerManager(this);
		return true;
	}

	void EffekseerManager::Execute(Float elapsedTime)
	{
		totalTime_ += elapsedTime;
		memoryPool_->NewFrame();

		SeedCore::erase_if(handles_, [this](Effekseer::Handle handle) { return !manager_->Exists(handle); });

		Int32 total = manager_->GetTotalInstanceCount();
		Size evicted = 0;
		while (total > 18000 && evicted < handles_.size())
		{
			total -= manager_->GetInstanceCount(handles_[evicted]);
			manager_->StopEffect(handles_[evicted]);
			++evicted;
		}
		handles_.erase(handles_.begin(), handles_.begin() + static_cast<std::ptrdiff_t>(evicted));

		Effekseer::Manager::UpdateParameter updateParameter;
		updateParameter.DeltaFrame = elapsedTime * 60.0f;
		manager_->Update(updateParameter);
	}

	void EffekseerManager::Finalize()
	{
		if (manager_ != nullptr)
		{
			manager_->StopAllEffects();
			handles_.clear();
		}
		manager_.Reset();
		cacheSetting_.Reset();
		curveLoader_.Reset();
		materialLoader_.Reset();
		modelLoader_.Reset();
		textureLoader_.Reset();
		cmdList_.Reset();
		memoryPool_.Reset();
		renderer_.Reset();
	}

	void EffekseerManager::Draw(D3D12CommandList& cmdList, const Matrix& view, const Matrix& projection)
	{
		Effekseer::Matrix44 camera;
		Effekseer::Matrix44 lens;
		for (Int row = 0;row < 4;++row)
		{
			for (Int column = 0;column < 4;++column)
			{
				camera.Values[row][column] = view.m[row][column];
				lens.Values[row][column] = projection.m[row][column];
			}
		}

		EffekseerRendererDX12::BeginCommandList(cmdList_, cmdList.Get());
		renderer_->SetCameraMatrix(camera);
		renderer_->SetProjectionMatrix(lens);
		renderer_->BeginRendering();
		manager_->Draw();
		renderer_->EndRendering();
		EffekseerRendererDX12::EndCommandList(cmdList_);
	}

	Effekseer::EffectRef EffekseerManager::Create(const EffekseerAsset& asset)
	{
		Effekseer::EffectRef effect = Effekseer::Effect::Create(cacheSetting_, asset.data_.data(), static_cast<Int32>(asset.data_.size()), 1.0f, nullptr);
		if (effect == nullptr)
		{
			return nullptr;
		}

		auto setTexture = [this, &effect](const DynamicArray<EffekseerTexture>& textures, Effekseer::TextureType textureType)
			{
				for (Int32 index = 0;index < static_cast<Int32>(textures.size());++index)
				{
					effect->SetTexture(index, textureType, textureLoader_->Create(textures[index], textureType));
				}
			};
		setTexture(asset.colorTextures_, Effekseer::TextureType::Color);
		setTexture(asset.normalTextures_, Effekseer::TextureType::Normal);
		setTexture(asset.distortionTextures_, Effekseer::TextureType::Distortion);

		for (Int32 index = 0;index < static_cast<Int32>(asset.models_.size());++index)
		{
			effect->SetModel(index, modelLoader_->Create(asset.models_[index]));
		}
		for (Int32 index = 0;index < static_cast<Int32>(asset.materials_.size());++index)
		{
			effect->SetMaterial(index, materialLoader_->Create(asset.materials_[index]));
		}
		for (Int32 index = 0;index < static_cast<Int32>(asset.curves_.size());++index)
		{
			effect->SetCurve(index, curveLoader_->Create(asset.curves_[index]));
		}

		return effect;
	}

	Effekseer::Handle EffekseerManager::Play(const Effekseer::EffectRef& effect, const Vector3& position)
	{
		Effekseer::Handle handle = manager_->Play(effect, position.x, position.y, position.z);
		if (handle >= 0)
		{
			handles_.push_back(handle);
		}
		return handle;
	}

	void EffekseerManager::Stop(Effekseer::Handle handle, EffectStopFlag flag)
	{
		if (flag == EffectStopFlag::Finish)
		{
			manager_->StopRoot(handle);
			return;
		}
		manager_->StopEffect(handle);
		SeedCore::erase_if(handles_, [handle](Effekseer::Handle candidate) { return candidate == handle; });
	}

	Bool EffekseerManager::Playing(Effekseer::Handle handle)
	{
		return manager_->Exists(handle);
	}

	void EffekseerManager::Transform(Effekseer::Handle handle, const Matrix& matrix)
	{
		Effekseer::Matrix43 value;
		for (Int row = 0;row < 4;++row)
		{
			for (Int column = 0;column < 3;++column)
			{
				value.Value[row][column] = matrix.m[row][column];
			}
		}
		manager_->SetMatrix(handle, value);
	}

	void EffekseerManager::Speed(Effekseer::Handle handle, Float speed)
	{
		manager_->SetSpeed(handle, speed);
	}

	void EffekseerManager::Color(Effekseer::Handle handle, const SeedCore::Color& color)
	{
		auto channel = [](Float value)
			{
				return static_cast<Uint8>(Clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
			};
		manager_->SetAllColor(handle, Effekseer::Color(channel(color.r), channel(color.g), channel(color.b), channel(color.a)));
	}

	Effekseer::ManagerRef EffekseerManager::GetManager()const
	{
		return manager_;
	}

	Float EffekseerManager::GetTotalTime()const
	{
		return totalTime_;
	}
}

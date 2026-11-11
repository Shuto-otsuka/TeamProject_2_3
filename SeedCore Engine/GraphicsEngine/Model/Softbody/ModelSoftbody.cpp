#include <GraphicsEngine/Model/Softbody/ModelSoftbody.h>
#include <GraphicsEngine/Model/ModelUnit.h>
#include <GraphicsEngine/Model/Softbody/SoftbodyMesh.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <PhysicsEngine/Softbody/Softbody.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	ModelSoftbody::ModelSoftbody()
	{
		/// No Code
	}

	ModelSoftbody::~ModelSoftbody() = default;

	void ModelSoftbody::Initialize(RenderDevice& device)
	{
		device_ = &device;
	}

	void ModelSoftbody::Begin()
	{
		opaque_.clear();
		transparent_.clear();
		fur_.clear();
	}

	Bool ModelSoftbody::Write(Uint32 slot, Bool reset, Actor& actor, Crister& crister, const Matrix& world, const Matrix& previousWorld, const ModelMaterialStructuredBuffer& material, Bool selected)
	{
		Softbody* softbody = actor.GetComponent<Softbody>();
		if (!softbody || softbody->BodyID() == ScInvalidBodyID)
		{
			Release(slot);
			return false;
		}
		if (slots_.size() <= slot)
		{
			slots_.resize(static_cast<Size>(slot) + 1);
		}
		if (reset)
		{
			slots_[slot].reset();
		}

		ResourcePtr<SoftbodyMesh>& mesh = slots_[slot];
		if (!mesh)
		{
			mesh = MakePtr<SoftbodyMesh>();
			if (!mesh->Create(device_->Context().GetDevice(), &device_->ResourceHeap(), crister))
			{
				mesh.reset();
				return true;
			}
		}
		mesh->Update(softbody->BodyVertex());

		ModelStructuredBuffer instance{};
		instance.transform_.world_ = world;
		instance.transform_.inverseTransposeWorld_ = world.Invert().Transpose();
		instance.transform_.previousWorld_ = previousWorld;
		instance.shading_ = material.shading_;
		instance.shading_.selected_ = selected ? 1 : 0;
		instance.texture_ = material.texture_;
		instance.extension_ = material.extension_;
		instance.geometry_.vertexBufferIndex_ = mesh->VertexBufferIndex();
		instance.geometry_.meshletBufferIndex_ = mesh->MeshletBufferIndex();
		instance.geometry_.meshletBoundBufferIndex_ = mesh->MeshletBoundBufferIndex();
		instance.geometry_.vertexIndicesBufferIndex_ = mesh->VertexIndicesBufferIndex();
		instance.geometry_.primitiveIndicesBufferIndex_ = mesh->PrimitiveIndicesBufferIndex();
		instance.streaming_.positionMin_ = mesh->PositionMin();
		instance.streaming_.positionExtent_ = mesh->PositionExtent();
		instance.streaming_.texcoordMinU_ = mesh->TexcoordMin().x;
		instance.streaming_.texcoordMinV_ = mesh->TexcoordMin().y;
		instance.streaming_.texcoordExtent_ = mesh->TexcoordExtent();
		instance.streaming_.lodError_ = 0.0f;
		instance.streaming_.lodErrorNext_ = FLT_MAX;
		instance.skining_.skinIndex_ = SC_INVALID;
		instance.skining_.boneOffset_ = 0;
		instance.skining_.skinVertexBufferIndex_ = SC_INVALID;
		instance.morph_.morphDeltaBufferIndex_ = SC_INVALID;
		instance.morph_.vertexMorphSourceBufferIndex_ = SC_INVALID;

		Bool transparent = material.shading_.blend_ != 0;
		Bool fur = !transparent && material.shading_.shadingModel_ == static_cast<Uint>(ShadingModel::Fur);
		constexpr Uint32 maxMeshletsPerInstance = 32;
		for (Uint32 offset = 0;offset < mesh->MeshletCount();offset += maxMeshletsPerInstance)
		{
			instance.geometry_.meshletOffset_ = offset;
			instance.geometry_.meshletCount_ = Min(maxMeshletsPerInstance, mesh->MeshletCount() - offset);
			if (transparent)
			{
				transparent_.push_back(instance);
				continue;
			}
			opaque_.push_back(instance);
			if (fur)
			{
				fur_.push_back(instance);
			}
		}
		return true;
	}

	void ModelSoftbody::Release(Uint32 slot)
	{
		if (slot < slots_.size())
		{
			slots_[slot].reset();
		}
	}

	void ModelSoftbody::End()
	{
		opaque_.resize(Min(opaque_.size(), static_cast<Size>(ModelUnit::transparentBase_)));
		transparent_.resize(Min(transparent_.size(), static_cast<Size>(ModelUnit::furBase_ - ModelUnit::transparentBase_)));
		fur_.resize(Min(fur_.size(), static_cast<Size>(ModelUnit::instanceCapacity_ - ModelUnit::furBase_)));

		instances_.clear();
		instances_.insert(instances_.end(), opaque_.begin(), opaque_.end());
		instances_.insert(instances_.end(), transparent_.begin(), transparent_.end());
		instances_.insert(instances_.end(), fur_.begin(), fur_.end());
	}

	std::span<const ModelStructuredBuffer> ModelSoftbody::Instances()const
	{
		return instances_;
	}

	Uint32 ModelSoftbody::OpaqueCount()const
	{
		return static_cast<Uint32>(opaque_.size());
	}

	Uint32 ModelSoftbody::TransparentCount()const
	{
		return static_cast<Uint32>(transparent_.size());
	}

	Uint32 ModelSoftbody::FurCount()const
	{
		return static_cast<Uint32>(fur_.size());
	}
}

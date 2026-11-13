#include <GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolid.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>
#include <GraphicsEngine/Raytracing/BottomLevelShader.h>
#include <GraphicsEngine/Raytracing/TopLevelShader.h>
#include <GraphicsEngine/RenderGraph/RenderFrame.h>
#include <GraphicsEngine/Texture/TextureStreaming.h>

namespace SeedCore
{
	PrimitiveSolid::PrimitiveSolid()
	{
		/// No Code
	}

	PrimitiveSolid::~PrimitiveSolid() = default;

	void PrimitiveSolid::Create(ID3D12Device* device, BindlessHeap& heap)
	{
		mesh_.Create(device, heap);
	}

	void PrimitiveSolid::Gather(const ShapeList& list, TextureStreaming& textureStreaming, RenderFrame& frame)
	{
		++frame_;
		updates_.clear();
		materials_.clear();

		Surface surface;
		ModelMaterialStructuredBuffer defaults{};
		defaults.texture_.metallic_ = surface.metallic_;
		defaults.texture_.roughness_ = surface.roughness_;
		defaults.texture_.ior_ = surface.khr_.ior_.ior_;
		defaults.texture_.emissiveStrength_ = surface.khr_.emissiveStrength_.emissiveStrength_;
		defaults.texture_.baseColorTextureIndex_ = SC_INVALID;
		defaults.texture_.normalTextureIndex_ = SC_INVALID;
		defaults.texture_.metallicRoughnessTextureIndex_ = SC_INVALID;
		defaults.texture_.emissiveTextureIndex_ = SC_INVALID;
		defaults.texture_.occlusionTextureIndex_ = SC_INVALID;
		defaults.extension_.specularFactor_ = surface.khr_.specular_.specularFactor_;
		defaults.extension_.specularColor_ = Vector3(surface.khr_.specular_.specularColorFactor_[0], surface.khr_.specular_.specularColorFactor_[1], surface.khr_.specular_.specularColorFactor_[2]);
		defaults.extension_.volumeAttenuationDistance_ = surface.khr_.volume_.attenuationDistance_;
		defaults.extension_.volumeAttenuationColor_ = Vector3(surface.khr_.volume_.attenuationColor_[0], surface.khr_.volume_.attenuationColor_[1], surface.khr_.volume_.attenuationColor_[2]);
		defaults.extension_.iridescenceIor_ = surface.khr_.iridescence_.iridescenceIor_;
		defaults.extension_.specularTextureIndex_ = SC_INVALID;
		defaults.extension_.specularColorTextureIndex_ = SC_INVALID;
		defaults.extension_.clearCoatTextureIndex_ = SC_INVALID;
		defaults.extension_.clearCoatRoughnessTextureIndex_ = SC_INVALID;
		defaults.extension_.clearCoatNormalTextureIndex_ = SC_INVALID;
		defaults.extension_.transmissionTextureIndex_ = SC_INVALID;
		defaults.extension_.thicknessTextureIndex_ = SC_INVALID;
		defaults.extension_.sheenColorTextureIndex_ = SC_INVALID;
		defaults.extension_.sheenRoughnessTextureIndex_ = SC_INVALID;
		defaults.extension_.iridescenceTextureIndex_ = SC_INVALID;
		defaults.extension_.iridescenceThicknessTextureIndex_ = SC_INVALID;
		defaults.extension_.anisotropyTextureIndex_ = SC_INVALID;
		defaults.shading_.shadingModel_ = static_cast<Uint>(surface.shadingModel_);
		defaults.shading_.materialMapping_ = static_cast<Uint>(surface.materialMapping_);
		defaults.shading_.tiling_ = surface.tiling_;
		defaults.shading_.triplanarSharpness_ = surface.triplanarSharpness_;
		defaults.shading_.actor_ = SC_INVALID;

		Uint32 count = 0;
		StaticArray<Bool, PrimitiveMesh::kindCount_> requested = {};
		for (std::span<const ShapeDesc> shapes : { list.shapes_, list.colliders_ })
		{
			for (const ShapeDesc& shape : shapes)
			{
				Uint32 slot = mesh_.Slot(shape.kind_);
				if (shape.style_ != ShapeStyle::Solid || shape.space_ != ShapeSpace::World || slot == SC_INVALID || count >= capacity_)
				{
					continue;
				}

				Vector3 scale = shape.dimensions_;
				if (shape.kind_ == ShapeKind::Sphere)
				{
					scale = Vector3(shape.dimensions_.x, shape.dimensions_.x, shape.dimensions_.x);
				}
				else if (shape.kind_ == ShapeKind::Cylinder || shape.kind_ == ShapeKind::Cone)
				{
					scale = Vector3(shape.dimensions_.x, shape.dimensions_.y, shape.dimensions_.x);
				}

				Uint32 index = count;
				++count;
				if (slots_.size() <= index)
				{
					slots_.resize(static_cast<Size>(index) + 1);
				}
				Slot& state = slots_[index];

				PrimitiveSolidStructuredBuffer solid;
				solid.world_ = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(shape.rotation_) * Matrix::CreateTranslation(shape.position_);
				solid.previousWorld_ = state.sent_ && state.solid_.flags_ != 0 && state.solid_.slot_ == slot ? state.solid_.world_ : solid.world_;
				solid.slot_ = slot;
				solid.flags_ = 1;
				solid.meshletOffset_ = mesh_.Meshlets(slot).meshletOffset_;
				solid.meshletCount_ = mesh_.Meshlets(slot).meshletCount_;
				solid.texcoordMin_ = shape.uvOffset_;
				solid.texcoordExtent_ = shape.uvScale_;
				solid.indexBufferIndex_ = mesh_.IndexBufferIndex(slot);

				ModelMaterialStructuredBuffer material = defaults;
				material.texture_.baseColor_ = shape.color_;
				material.texture_.baseColorTextureIndex_ = shape.textureID_ != 0 ? textureStreaming.GetID(shape.textureID_, frame_) : SC_INVALID;
				material.shading_.doubleSided_ = shape.doubleSided_ ? 1 : 0;
				material.shading_.blend_ = shape.color_.a < 1.0f ? 1 : 0;

				if (!state.sent_ || std::memcmp(&solid, &state.solid_, sizeof(PrimitiveSolidStructuredBuffer)) != 0)
				{
					PrimitiveSolidUpdate update;
					update.index_ = index;
					update.data_ = solid;
					updates_.push_back(update);
				}
				if (!state.sent_ || std::memcmp(&material, &state.material_, sizeof(ModelMaterialStructuredBuffer)) != 0)
				{
					PrimitiveMaterialUpdate update;
					update.index_ = index;
					update.data_ = material;
					materials_.push_back(update);
				}
				state.solid_ = solid;
				state.material_ = material;
				state.sent_ = true;
				requested[slot] = true;
			}
		}

		for (Uint32 index = count;index < count_;++index)
		{
			Slot& state = slots_[index];
			if (state.solid_.flags_ == 0)
			{
				continue;
			}
			state.solid_.flags_ = 0;
			PrimitiveSolidUpdate update;
			update.index_ = index;
			update.data_ = state.solid_;
			updates_.push_back(update);
		}
		count_ = count;

		for (Uint32 slot = 0;slot < PrimitiveMesh::kindCount_;++slot)
		{
			if (!requested[slot])
			{
				continue;
			}
			BottomLevelRequest request;
			request.key_ = (3ull << 56) | slot;
			request.geometry_ = mesh_.Geometry(slot);
			frame.Append(request);
		}

		TopLevelRequest request;
		request.count_ = count_;
		frame.Append(request);

		constants_ = PrimitiveSolidConstantBuffer{};
		constants_.solidCount_ = count_;
		constants_.transparentBase_ = GeometryFeature::transparentBase_;
		constants_.capacity_ = GeometryFeature::instanceCapacity_;
		constants_.vertexBufferIndex_ = mesh_.VertexBufferIndex();
		constants_.meshletBufferIndex_ = mesh_.MeshletBufferIndex();
		constants_.meshletBoundBufferIndex_ = mesh_.MeshletBoundBufferIndex();
		constants_.vertexIndicesBufferIndex_ = mesh_.VertexIndicesBufferIndex();
		constants_.primitiveIndicesBufferIndex_ = mesh_.PrimitiveIndicesBufferIndex();
	}

	std::span<const PrimitiveSolidUpdate> PrimitiveSolid::Updates()const
	{
		return updates_;
	}

	std::span<const PrimitiveMaterialUpdate> PrimitiveSolid::Materials()const
	{
		return materials_;
	}

	const PrimitiveSolidConstantBuffer& PrimitiveSolid::Constants()const
	{
		return constants_;
	}

	Uint32 PrimitiveSolid::Count()const
	{
		return count_;
	}
}

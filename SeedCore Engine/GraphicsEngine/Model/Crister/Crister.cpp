#include <GraphicsEngine/Model/Crister/Crister.h>

namespace SeedCore
{
	CristerResidency& Crister::Residency()
	{
		return residency_;
	}

	const CristerResidency& Crister::Residency()const
	{
		return residency_;
	}

	std::span<const Stage> Crister::Stages()const
	{
		return scene_.stages_; 
	}

	std::span<const Node> Crister::Nodes()const
	{
		return scene_.nodes_;
	}

	std::span<const PunctualLight> Crister::Lights()const
	{
		return scene_.lights_;
	}

	std::span<const SubMesh> Crister::SubMeshes()const
	{
		return geometry_.subMeshes_;
	}

	std::span<const Surface> Crister::Surfaces()const
	{
		return material_.surfaces_;
	}

	std::span<const Bitmap> Crister::Bitmaps()const
	{
		return material_.bitmaps_;
	}

	std::span<const Skin> Crister::Skins()const
	{
		return scene_.skins_;
	}

	std::span<const Cluster> Crister::Clusters()const
	{
		return geometry_.clusters_;
	}

	Int Crister::DefaultStage()const
	{
		return scene_.defaultStage_;
	}

	Int Crister::FindNodeIndex(const std::string& name)const
	{
		for (Size nodeIndex = 0; nodeIndex < scene_.nodes_.size(); nodeIndex++)
		{
			if (scene_.nodes_[nodeIndex].name_ == name)
			{
				return static_cast<Int>(nodeIndex);
			}
		}
		return -1;
	}

	Vector3 Crister::PositionMin()const
	{
		return geometry_.positionMin_;
	}

	Vector3 Crister::PositionExtent()const
	{
		return geometry_.positionExtent_;
	}

	Vector2 Crister::TexcoordMin()const
	{
		return geometry_.texcoordMin_;
	}

	Vector2 Crister::TexcoordExtent()const
	{
		return geometry_.texcoordExtent_;
	}

	void Crister::CumulateTransforms()
	{
		if (scene_.defaultStage_ < 0 || static_cast<Size>(scene_.defaultStage_) >= scene_.stages_.size())
		{
			return;
		}

		std::stack<Matrix> parentGlobalTransforms;
		std::function<void(Int)> traverse = [&](Int nodeIndex)
			{
				if (nodeIndex < 0 || static_cast<Size>(nodeIndex) >= scene_.nodes_.size())
				{
					return;
				}

				Node& node = scene_.nodes_[nodeIndex];
				Matrix scale = Matrix::CreateScale(node.scale_.x, node.scale_.y, node.scale_.z);
				Matrix rotation = Matrix::CreateFromQuaternion(node.rotation_);
				Matrix translation = Matrix::CreateTranslation(node.translation_.x, node.translation_.y, node.translation_.z);

				Matrix localTransform = scale * rotation * translation;
				node.globalTransform_ = localTransform * parentGlobalTransforms.top();

				for (Int childIndex : node.children_)
				{
					parentGlobalTransforms.push(node.globalTransform_);
					traverse(childIndex);
					parentGlobalTransforms.pop();
				}
			};

		for (Int nodeIndex : scene_.stages_[scene_.defaultStage_].nodes_)
		{
			parentGlobalTransforms.push(Matrix::Identity);
			traverse(nodeIndex);
			parentGlobalTransforms.pop();
		}
	}
}

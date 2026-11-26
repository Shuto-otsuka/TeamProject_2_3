#pragma once

namespace SeedCore
{
	struct Stage
	{
		std::string name_;

		DynamicArray<Int> nodes_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("nodes", nodes_);
		}
	};

	struct Node
	{
		std::string name_;

		Int mesh_ = -1;

		Int skin_ = -1;

		Int light_ = -1;

		DynamicArray<Int> children_;

		Int parentIndex_ = -1;

		Quaternion rotation_ = { 0,0,0,1 };

		Vector3 scale_ = { 1,1,1 };

		Vector3 translation_ = { 0,0,0 };

		Matrix globalTransform_ = Matrix::Identity;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("name", name_);
			archive.Field("mesh", mesh_);
			archive.Field("skin", skin_);
			archive.Field("light", light_);
			archive.Field("children", children_);
			archive.Field("rotation", rotation_);
			archive.Field("scale", scale_);
			archive.Field("translation", translation_);
			archive.Field("global_transform", globalTransform_);
		}
	};

	struct Skin
	{
		DynamicArray<Matrix> inverseBindMatrices_;

		DynamicArray<Int> joints_;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("inverse_bind_matrices", inverseBindMatrices_);
			archive.Field("joints", joints_);
		}
	};

	struct PunctualLight
	{
		enum class Type : Uint32
		{
			Point = 0,

			Spot = 1,
		};

		Type type_ = Type::Point;

		Vector3 position_ = { 0,0,0 };

		Vector3 direction_ = { 0,0,-1 };

		Float colorRGB_[3] = { 1,1,1 };

		Float intensity_ = 1.0f;

		Float range_ = 0.0f;

		Float innerConeAngle_ = 0.0f;

		Float outerConeAngle_ = 0.785398163f;

		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.Field("type", type_);
			archive.Field("position", position_);
			archive.Field("direction", direction_);
			archive.Field("color_rgb", colorRGB_);
			archive.Field("intensity", intensity_);
			archive.Field("range", range_);
			archive.Field("inner_cone_angle", innerConeAngle_);
			archive.Field("outer_cone_angle", outerConeAngle_);
		}
	};

	struct CristerScene
	{
		DynamicArray<Stage> stages_;

		DynamicArray<Node> nodes_;

		DynamicArray<Skin> skins_;

		DynamicArray<PunctualLight> lights_;

		Int defaultStage_ = 0;
	};
}
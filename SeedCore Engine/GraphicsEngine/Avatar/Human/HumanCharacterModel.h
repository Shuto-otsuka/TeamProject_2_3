#pragma once
namespace SeedCore
{
	constexpr Uint32 humanCharacterMagic = 0x43485353;
	constexpr Uint32 humanCharacterVersion = 3;
	constexpr Uint32 humanCharacterRegionCount = 4;

	struct HumanCharacterRegionRange
	{
		Uint32 firstTriangle_ = 0;
		Uint32 triangleCount_ = 0;
	};

	struct HumanCharacterBone
	{
		Int32 parentIndex_ = -1;
		Float restHead_[3] = { 0.0f, 0.0f, 0.0f };
		Float restTail_[3] = { 0.0f, 0.0f, 0.0f };
	};

	struct HumanCharacterAxis
	{
		Float defaultValue_ = 0.0f;
		Float minValue_ = 0.0f;
		Float maxValue_ = 0.0f;
		DynamicArray<Uint32> vertexIndices_;
		DynamicArray<Vector3> deltas_;
		DynamicArray<Uint32> negativeVertexIndices_;
		DynamicArray<Vector3> negativeDeltas_;
	};

	class SEEDCORE_API HumanCharacterModel :public NonCopyable
	{
	public:
		HumanCharacterModel() = default;
		~HumanCharacterModel() = default;

		Bool Load(String filePath);

		NODISCARD Bool Loaded()const;

		NODISCARD Uint32 VertexCount()const;

		NODISCARD Uint32 BodyVertexCount()const;

		NODISCARD Uint32 TriangleCount()const;

		NODISCARD Uint32 AxisCount()const;

		NODISCARD Uint32 BoneCount()const;

		NODISCARD std::span<const Vector3> Positions()const;

		NODISCARD std::span<const Vector3> Normals()const;

		NODISCARD std::span<const Vector2> Texcoords()const;

		NODISCARD std::span<const Uint32> Triangles()const;

		NODISCARD std::span<const Uint8> Regions()const;

		NODISCARD std::span<const Float> SkinWeights()const;

		NODISCARD std::span<const Uint32> SkinIndices()const;

		NODISCARD const HumanCharacterAxis& Axis(Uint32 index)const;

		NODISCARD const HumanCharacterBone& Bone(Uint32 index)const;

		NODISCARD std::span<const Uint32> JointGroup(Uint32 boneIndex)const;

		NODISCARD const HumanCharacterRegionRange& RegionTriangleRange(Uint32 regionIndex)const;

		NODISCARD static std::span<const Char* const> RegionNames();

		NODISCARD static std::span<const Char* const> RegionLabels();

		NODISCARD static std::span<const Char* const> AxisNames();

		NODISCARD static std::span<const Char* const> AxisLabels();

		NODISCARD static std::span<const Char* const> BoneNames();

	private:
		DynamicArray<Byte> blob_;

		Uint32 vertexCount_ = 0;
		Uint32 bodyVertexCount_ = 0;
		Uint32 triangleCount_ = 0;
		Uint32 boneCount_ = 0;

		std::span<const Vector3> positions_;
		std::span<const Vector3> normals_;
		std::span<const Vector2> texcoords_;
		std::span<const Uint32> triangles_;
		std::span<const Uint8> regions_;
		std::span<const Float> skinWeights_;
		std::span<const Uint32> skinIndices_;
		std::span<const HumanCharacterBone> bones_;

		DynamicArray<HumanCharacterAxis> axes_;
		DynamicArray<DynamicArray<Uint32>> jointGroups_;
		HumanCharacterRegionRange regionRanges_[humanCharacterRegionCount];
	};
}

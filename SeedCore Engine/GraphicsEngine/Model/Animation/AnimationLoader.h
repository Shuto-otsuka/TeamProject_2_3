#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace tinygltf
{
	class Model;
	struct Animation;
}

namespace fbxsdk
{
	class FbxAnimStack;
	class FbxNode;
}

namespace SeedCore
{
	class SEEDCORE_API AnimationLoader :public AssetLoader<Animation>
	{
	private:
		friend class AnimationResource;

	public:
		AnimationLoader() = default;
		~AnimationLoader() = default;

		Handle<Animation> Load(String filePath, LoadThread thread);

		void Clear(Handle<Animation>& handle)noexcept;

	private:
		Bool Bake(const Animation& animation, String filePath);

		Bool SplitClips(String filePath, const AxisConvention& axisConvention, DynamicArray<String>& writtenPaths);

		void FetchAnimationClip(const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation, Animation& animation);

		void FetchAnimationClip(fbxsdk::FbxAnimStack* fbxStack, const DynamicArray<fbxsdk::FbxNode*>& nodeTable, Float unitScale, const ResolvedAxisConvention& resolved, Double frameRate, Animation& animation);

	};
}

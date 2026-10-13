#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace SeedCore
{
	class SEEDCORE_API AnimationLoader :public NonCopyable
	{
	public:
		AnimationLoader() = default;
		~AnimationLoader() = default;

		Handle<Animation> Load(String filePath);

		Bool Save(Animation& animation, const std::filesystem::path& filePath);

		Animation* Get(const Handle<Animation>& handle);

		void Clear(Handle<Animation>& handle)noexcept;

		void SplitClips(String filePath);

	private:
		void FetchAnimationClip(const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation, Animation& animation);

	private:
		StablePool<Animation> pool_;
	};
}

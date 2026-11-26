#pragma once
#include <GraphicsEngine/Camera/PreviewCamera.h>
#include <GraphicsEngine/Camera/PreviewCameraController.h>

namespace SeedCore
{
	struct BootConfig;

	class AvatarMesh;
	class BootTexture;

	struct PreviewViewContext
	{
		PreviewCamera camera_;

		PreviewCameraController cameraController_;

		void Tick(Float deltaTime)
		{
			camera_.Tick(deltaTime);
		}
	};

	struct TimelinePreviewContext : PreviewViewContext
	{
		Bool active_ = false;

		Uint32 meshAssetID_ = 0;

		Uint32 animationAssetID_ = 0;

		Float time_ = 0.0f;

	};

	struct ModelTransformPreviewContext : PreviewViewContext
	{
		Bool active_ = false;

		Uint32 meshAssetID_ = 0;

		Matrix worldMatrix_ = Matrix::Identity;

	};

	struct MaterialPreviewContext : PreviewViewContext
	{
		Bool active_ = false;

		Uint32 meshAssetID_ = 0;

		Uint32 surfaceAssetID_ = 0;

	};

	struct SkeletonControllerPreviewContext : PreviewViewContext
	{
		Bool active_ = false;

		Uint32 meshAssetID_ = 0;

		Int nodeIndex_ = -1;

	};

	struct AvatarPreviewContext : PreviewViewContext
	{
		Bool active_ = false;

		AvatarMesh* mesh_ = nullptr;

		std::span<const Vector3> positions_;

		std::span<const Vector3> normals_;

		Uint32 boneCount_ = 0;

		Uint32 regionTextureIndices_[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

		Uint32 regionCount_ = 0;

	};

	struct BootScreenPreviewContext
	{
		Bool active_ = false;

		const BootTexture* texture_ = nullptr;

		const BootConfig* config_ = nullptr;

		Float progress_ = 0.0f;

		Uint32 width_ = 1;

		Uint32 height_ = 1;
	};

	struct PreviewContext
	{
		void Tick(Float deltaTime)
		{
			timeline_.Tick(deltaTime);
			modelTransform_.Tick(deltaTime);
			material_.Tick(deltaTime);
			skeletonController_.Tick(deltaTime);
			avatar_.Tick(deltaTime);
		}

		TimelinePreviewContext timeline_;

		ModelTransformPreviewContext modelTransform_;

		MaterialPreviewContext material_;

		SkeletonControllerPreviewContext skeletonController_;

		AvatarPreviewContext avatar_;

		BootScreenPreviewContext bootScreen_;
	};
}

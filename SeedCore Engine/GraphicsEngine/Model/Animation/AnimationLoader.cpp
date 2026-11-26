#ifdef __clang__
#define STBI_NO_SIMD
#endif
#include <External/TinyglTF/Include/tiny_gltf.h>
#ifndef FBXSDK_SHARED
#define FBXSDK_SHARED
#endif
#include <External/FBXSDK/Include/fbxsdk.h>
#include <GraphicsEngine/Model/Animation/AnimationLoader.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>

namespace SeedCore
{
	Handle<Animation> AnimationLoader::Load(String filePath, LoadThread thread)
	{
		std::filesystem::path path(filePath.c_str());
		if (path.extension() != ".animation" || !std::filesystem::exists(path))
		{
			return Handle<Animation>::null();
		}

		Handle<Animation> handle = pool_.Create();
		Animation* animation = pool_.Get(handle);
		if (!animation)
		{
			return Handle<Animation>::null();
		}

		BinaryInputArchive archive;
		if (!archive.Read(filePath))
		{
			pool_.Destroy(handle);
			return Handle<Animation>::null();
		}
		animation->Serialize(archive);

		return handle;
	}

	void AnimationLoader::Clear(Handle<Animation>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	Bool AnimationLoader::Bake(const Animation& animation, String filePath)
	{
		BinaryOutputArchive archive;
		const_cast<Animation&>(animation).Serialize(archive);
		return archive.Write(filePath);
	}

	Bool AnimationLoader::SplitClips(String filePath, const AxisConvention& axisConvention, DynamicArray<String>& writtenPaths)
	{
		std::filesystem::path path(filePath.c_str());

		Bool allWritten = true;

		if (path.extension() == ".fbx")
		{
			FbxManager* fbxManager = FbxManager::Create();
			fbxManager->SetIOSettings(FbxIOSettings::Create(fbxManager, IOSROOT));
			FbxScene* fbxScene = FbxScene::Create(fbxManager, "");

			FbxImporter* importer = FbxImporter::Create(fbxManager, "");
			Bool imported = importer->Initialize(path.string().c_str(), -1, fbxManager->GetIOSettings());
			if (imported)
			{
				imported = importer->Import(fbxScene);
			}
			importer->Destroy();

			if (!imported)
			{
				fbxManager->Destroy();
				return false;
			}

			Int stackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();
			if (stackCount == 0)
			{
				fbxManager->Destroy();
				return true;
			}

			Float unitScale = static_cast<Float>(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor()) / 100.0f;

			FbxAxisSystem gltfAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
			gltfAxisSystem.DeepConvertScene(fbxScene);

			DynamicArray<FbxNode*> nodeTable;
			DynamicArray<FbxNode*> nodeStack;
			FbxNode* fbxRoot = fbxScene->GetRootNode();
			for (Int childIndex = fbxRoot->GetChildCount() - 1; childIndex >= 0; childIndex--)
			{
				nodeStack.push_back(fbxRoot->GetChild(childIndex));
			}
			while (!nodeStack.empty())
			{
				FbxNode* fbxNode = nodeStack.back();
				nodeStack.pop_back();
				nodeTable.push_back(fbxNode);

				for (Int childIndex = fbxNode->GetChildCount() - 1; childIndex >= 0; childIndex--)
				{
					nodeStack.push_back(fbxNode->GetChild(childIndex));
				}
			}

			ResolvedAxisConvention resolved = ResolvedAxisConvention::Resolve(axisConvention);
			if (!resolved.valid_)
			{
				resolved = ResolvedAxisConvention::Resolve(AxisConvention{});
			}

			FbxTime::EMode timeMode = fbxScene->GetGlobalSettings().GetTimeMode();
			Double frameRate = (timeMode == FbxTime::eCustom) ? fbxScene->GetGlobalSettings().GetCustomFrameRate() : FbxTime::GetFrameRate(timeMode);
			if (frameRate <= 0.0)
			{
				frameRate = 30.0;
			}

			std::filesystem::path directory = path.parent_path() / (path.stem().string() + ".Animations");

			for (Int stackIndex = 0; stackIndex < stackCount; stackIndex++)
			{
				FbxAnimStack* fbxStack = fbxScene->GetSrcObject<FbxAnimStack>(stackIndex);

				fbxScene->SetCurrentAnimationStack(fbxStack);

				Animation animation;
				FetchAnimationClip(fbxStack, nodeTable, unitScale, resolved, frameRate, animation);
				if (animation.timelines_.empty())
				{
					continue;
				}

				std::string clipName = animation.name_.empty() ? ("Animation" + std::to_string(stackIndex)) : animation.name_;
				for (Char& c : clipName)
				{
					if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
					{
						c = '_';
					}
				}

				std::error_code directoryError;
				std::filesystem::create_directories(directory, directoryError);

				std::filesystem::path clipPath = directory / (clipName + ".animation");
				if (Bake(animation, String(clipPath.string())))
				{
					writtenPaths.push_back(String(clipPath.string()));
				}
				else
				{
					allWritten = false;
				}
			}

			fbxManager->Destroy();
			return allWritten;
		}

		if (path.extension() != ".gltf" && path.extension() != ".glb")
		{
			return true;
		}

		tinygltf::Model model;
		tinygltf::TinyGLTF tinyLoader;
		std::string error, warning;

		Bool result = false;
		std::string pathString = path.string();
		if (path.extension() == ".glb")
		{
			result = tinyLoader.LoadBinaryFromFile(&model, &error, &warning, pathString);
		}
		else
		{
			result = tinyLoader.LoadASCIIFromFile(&model, &error, &warning, pathString);
		}

		if (!result)
		{
			return false;
		}

		if (model.animations.empty())
		{
			return true;
		}

		std::filesystem::path directory = path.parent_path() / (path.stem().string() + ".Animations");
		std::error_code directoryError;
		std::filesystem::create_directories(directory, directoryError);

		for (Size clipIndex = 0; clipIndex < model.animations.size(); ++clipIndex)
		{
			const tinygltf::Animation& gltfAnimation = model.animations[clipIndex];

			Animation animation;
			FetchAnimationClip(model, gltfAnimation, animation);

			std::string clipName = gltfAnimation.name.empty() ? ("Animation" + std::to_string(clipIndex)) : gltfAnimation.name;
			for (Char& c : clipName)
			{
				if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*')
				{
					c = '_';
				}
			}

			std::filesystem::path clipPath = directory / (clipName + ".animation");
			if (Bake(animation, String(clipPath.string())))
			{
				writtenPaths.push_back(String(clipPath.string()));
			}
			else
			{
				allWritten = false;
			}
		}

		return allWritten;
	}

	void AnimationLoader::FetchAnimationClip(const tinygltf::Model& model, const tinygltf::Animation& gltfAnimation, Animation& animation)
	{
		animation.name_ = gltfAnimation.name;

		for (const tinygltf::AnimationSampler& gltfSampler : gltfAnimation.samplers)
		{
			AnimationSampler& sampler = animation.samplers_.emplace_back();
			sampler.input_ = gltfSampler.input;
			sampler.output_ = gltfSampler.output;
			sampler.interpolation_ = gltfSampler.interpolation;
		}

		for (const tinygltf::AnimationChannel& gltfChannel : gltfAnimation.channels)
		{
			AnimationChannel& channel = animation.channels_.emplace_back();
			channel.sampler_ = gltfChannel.sampler;
			channel.targetNode_ = gltfChannel.target_node;
			channel.targetPath_ = gltfChannel.target_path;

			const tinygltf::AnimationSampler& gltfSampler = gltfAnimation.samplers.at(gltfChannel.sampler);

			if (!animation.timelines_.contains(gltfChannel.target_node))
			{
				const tinygltf::Accessor& timeAccessor = model.accessors.at(gltfSampler.input);
				const tinygltf::BufferView& timeBufferView = model.bufferViews.at(timeAccessor.bufferView);
				DynamicArray<Float>& times = animation.timelines_[gltfChannel.target_node];
				times.resize(timeAccessor.count);
				memcpy(times.data(), model.buffers.at(timeBufferView.buffer).data.data() + timeBufferView.byteOffset + timeAccessor.byteOffset, timeAccessor.count * sizeof(Float));
			}

			const tinygltf::Accessor& accessor = model.accessors.at(gltfSampler.output);
			const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
			const Uchar* data = model.buffers.at(bufferView.buffer).data.data() + bufferView.byteOffset + accessor.byteOffset;

			if (gltfChannel.target_path == "rotation" && !animation.rotations_.contains(gltfChannel.target_node))
			{
				DynamicArray<Quaternion>& rotations = animation.rotations_[gltfChannel.target_node];
				rotations.resize(accessor.count);
				memcpy(rotations.data(), data, accessor.count * sizeof(Quaternion));

				for (Quaternion& rotation : rotations)
				{
					rotation.y = -rotation.y;
					rotation.z = -rotation.z;
				}
			}
			else if (gltfChannel.target_path == "scale" && !animation.scales_.contains(gltfChannel.target_node))
			{
				DynamicArray<Vector3>& scales = animation.scales_[gltfChannel.target_node];
				scales.resize(accessor.count);
				memcpy(scales.data(), data, accessor.count * sizeof(Vector3));
			}
			else if (gltfChannel.target_path == "translation" && !animation.translations_.contains(gltfChannel.target_node))
			{
				DynamicArray<Vector3>& translations = animation.translations_[gltfChannel.target_node];
				translations.resize(accessor.count);
				memcpy(translations.data(), data, accessor.count * sizeof(Vector3));

				for (Vector3& translation : translations)
				{
					translation.x = -translation.x;
				}
			}
			else if (gltfChannel.target_path == "weights" && !animation.weights_.contains(gltfChannel.target_node))
			{
				const tinygltf::Node& targetGltfNode = model.nodes.at(gltfChannel.target_node);
				Size targetCount = targetGltfNode.weights.size();
				if (targetCount == 0 && targetGltfNode.mesh >= 0)
				{
					const tinygltf::Mesh& targetGltfMesh = model.meshes.at(targetGltfNode.mesh);
					if (!targetGltfMesh.primitives.empty())
					{
						targetCount = targetGltfMesh.primitives.front().targets.size();
					}
				}

				if (targetCount > 0 && accessor.count % targetCount == 0)
				{
					Size keyframeCount = accessor.count / targetCount;
					const Float* flatWeights = reinterpret_cast<const Float*>(data);

					DynamicArray<DynamicArray<Float>>& weights = animation.weights_[gltfChannel.target_node];
					weights.resize(keyframeCount);
					for (Size keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
					{
						weights[keyframeIndex].assign(flatWeights + keyframeIndex * targetCount, flatWeights + (keyframeIndex + 1) * targetCount);
					}
				}
			}
		}

		Float minTime = FLT_MAX;
		for (const auto& times : animation.timelines_ | std::ranges::views::values)
		{
			if (!times.empty())
			{
				minTime = Min(minTime, times.front());
			}
		}

		if (minTime != FLT_MAX && minTime != 0.0f)
		{
			for (auto& times : animation.timelines_ | std::ranges::views::values)
			{
				for (Float& t : times)
				{
					t -= minTime;
				}
			}
		}

		Float duration = 0.0f;
		for (const auto& times : animation.timelines_ | std::ranges::views::values)
		{
			if (!times.empty())
			{
				duration = Max(duration, times.back());
			}
		}
		animation.duration_ = duration;
	}

	void AnimationLoader::FetchAnimationClip(fbxsdk::FbxAnimStack* fbxStack, const DynamicArray<fbxsdk::FbxNode*>& nodeTable, Float unitScale, const ResolvedAxisConvention& resolved, Double frameRate, Animation& animation)
	{
		animation.name_ = fbxStack->GetName();

		DynamicArray<Bool> translationAnimated(nodeTable.size(), false);
		DynamicArray<Bool> rotationAnimated(nodeTable.size(), false);
		DynamicArray<Bool> scaleAnimated(nodeTable.size(), false);

		FbxTime keyStart = FBXSDK_TIME_INFINITE;
		FbxTime keyStop = FBXSDK_TIME_MINUS_INFINITE;
		Bool anyAnimated = false;

		Int layerCount = fbxStack->GetMemberCount<FbxAnimLayer>();
		for (Size nodeIndex = 0; nodeIndex < nodeTable.size(); nodeIndex++)
		{
			FbxNode* fbxNode = nodeTable[nodeIndex];
			FbxProperty* properties[3] = { &fbxNode->LclTranslation, &fbxNode->LclRotation, &fbxNode->LclScaling };
			DynamicArray<Bool>* animatedFlags[3] = { &translationAnimated, &rotationAnimated, &scaleAnimated };

			for (Size propertyIndex = 0; propertyIndex < 3; propertyIndex++)
			{
				for (Int layerIndex = 0; layerIndex < layerCount; layerIndex++)
				{
					FbxAnimCurveNode* curveNode = properties[propertyIndex]->GetCurveNode(fbxStack->GetMember<FbxAnimLayer>(layerIndex));
					if (!curveNode)
					{
						continue;
					}

					for (Uint32 channelIndex = 0; channelIndex < curveNode->GetChannelsCount(); channelIndex++)
					{
						for (Int curveIndex = 0; curveIndex < curveNode->GetCurveCount(channelIndex); curveIndex++)
						{
							FbxAnimCurve* curve = curveNode->GetCurve(channelIndex, static_cast<Uint32>(curveIndex));
							Int keyCount = curve ? curve->KeyGetCount() : 0;
							if (keyCount == 0)
							{
								continue;
							}

							(*animatedFlags[propertyIndex])[nodeIndex] = true;
							anyAnimated = true;

							FbxTime firstKeyTime = curve->KeyGetTime(0);
							FbxTime lastKeyTime = curve->KeyGetTime(keyCount - 1);
							if (firstKeyTime < keyStart)
							{
								keyStart = firstKeyTime;
							}
							if (lastKeyTime > keyStop)
							{
								keyStop = lastKeyTime;
							}
						}
					}
				}
			}
		}

		if (!anyAnimated)
		{
			return;
		}

		FbxTimeSpan timeSpan = fbxStack->GetLocalTimeSpan();
		FbxTime start = timeSpan.GetStart();
		FbxTime stop = timeSpan.GetStop();
		if (stop <= start)
		{
			start = keyStart;
			stop = keyStop;
		}

		Double startSeconds = start.GetSecondDouble();
		Double durationSeconds = Max(0.0, stop.GetSecondDouble() - startSeconds);
		Size frameCount = static_cast<Size>(std::ceil(durationSeconds * frameRate - 1e-6)) + 1;

		DynamicArray<Float> times;
		times.reserve(frameCount);
		for (Size frameIndex = 0; frameIndex < frameCount; frameIndex++)
		{
			times.push_back(static_cast<Float>(Min(static_cast<Double>(frameIndex) / frameRate, durationSeconds)));
		}

		for (Size frameIndex = 0; frameIndex < frameCount; frameIndex++)
		{
			FbxTime sampleTime;
			sampleTime.SetSecondDouble(startSeconds + static_cast<Double>(times[frameIndex]));

			for (Size nodeIndex = 0; nodeIndex < nodeTable.size(); nodeIndex++)
			{
				if (!translationAnimated[nodeIndex] && !rotationAnimated[nodeIndex] && !scaleAnimated[nodeIndex])
				{
					continue;
				}

				Int key = static_cast<Int>(nodeIndex);
				if (frameIndex == 0)
				{
					animation.timelines_[key] = times;
				}

				FbxAMatrix localTransform = nodeTable[nodeIndex]->EvaluateLocalTransform(sampleTime);

				if (translationAnimated[nodeIndex])
				{
					FbxVector4 localTranslation = localTransform.GetT();
					Vector3 translation = Vector3(static_cast<Float>(localTranslation[0]), static_cast<Float>(localTranslation[1]), static_cast<Float>(localTranslation[2])) * unitScale;

					translation = Vector3::TransformPosition(translation, resolved.basis_);
					animation.translations_[key].push_back(translation);
				}

				if (rotationAnimated[nodeIndex])
				{
					FbxQuaternion localRotation = localTransform.GetQ();
					Quaternion rotation = Quaternion(static_cast<Float>(localRotation[0]), static_cast<Float>(localRotation[1]), static_cast<Float>(localRotation[2]), static_cast<Float>(localRotation[3]));

					Matrix converted = resolved.basis_.Transpose() * Matrix::CreateFromQuaternion(rotation) * resolved.basis_;
					rotation = Quaternion::CreateFromRotationMatrix(converted);

					DynamicArray<Quaternion>& rotations = animation.rotations_[key];
					if (!rotations.empty() && rotations.back().Dot(rotation) < 0.0f)
					{
						rotation = Quaternion(-rotation.x, -rotation.y, -rotation.z, -rotation.w);
					}
					rotations.push_back(rotation);
				}

				if (scaleAnimated[nodeIndex])
				{
					FbxVector4 localScale = localTransform.GetS();
					animation.scales_[key].push_back(Vector3(static_cast<Float>(localScale[0]), static_cast<Float>(localScale[1]), static_cast<Float>(localScale[2])));
				}
			}
		}

		animation.duration_ = static_cast<Float>(durationSeconds);
	}
}

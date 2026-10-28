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
	Handle<Animation> AnimationLoader::Load(String filePath)
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

	Bool AnimationLoader::Save(Animation& animation, const std::filesystem::path& filePath)
	{
		BinaryOutputArchive archive;
		animation.Serialize(archive);
		return archive.Write(String(filePath.string()));
	}

	Animation* AnimationLoader::Get(const Handle<Animation>& handle)
	{
		return pool_.Get(handle);
	}

	void AnimationLoader::Clear(Handle<Animation>& handle)noexcept
	{
		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Splits every clip embedded in a model source file into its own
	* "<model dir>/<model stem>.Animations/<clip name>.animation" file.
	* glTF/GLB writes one file per glTF animation; FBX writes one file per
	* animation stack (take). axisConvention is the model asset's own
	* convention from its .meta, so FBX clips land in the same space as the
	* Crister nodes they drive. Other extensions write nothing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* モデルのソースファイルに埋め込まれた全クリップを、それぞれ
	* "<モデルのディレクトリ>/<モデル名>.Animations/<クリップ名>.animation"
	* へ個別に書き出す。glTF/GLB は glTF アニメーション1つにつき1ファイル、
	* FBX はアニメーションスタック（テイク）1つにつき1ファイル。
	* axisConvention はモデルアセット自身の .meta の座標系規約で、FBX の
	* クリップを、それが動かす Crister のノードと同じ空間に揃えるために使う。
	* それ以外の拡張子は何も書き出さない。
	*/
	void AnimationLoader::SplitClips(String filePath, const AxisConvention& axisConvention)
	{
		std::filesystem::path path(filePath.c_str());

		if (path.extension() == ".fbx")
		{
			/// [EN] Import with the same settings ModelLoader uses, so the scene graph matches the one the Crister was built from.
			/// [JP] ModelLoader と同じ設定でインポートし、Crister の元になったシーングラフと一致させる。
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

			Int stackCount = imported ? fbxScene->GetSrcObjectCount<FbxAnimStack>() : 0;
			if (stackCount == 0)
			{
				fbxManager->Destroy();
				return;
			}

			/// [EN] Source unit to meters, read before the axis conversion exactly as ModelLoader does (centimeters are the FBX base unit).
			/// [JP] ソースの単位からメートルへの倍率。ModelLoader と同じく軸変換の前に読む（FBX の基準単位はセンチメートル）。
			Float unitScale = static_cast<Float>(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor()) / 100.0f;

			/// [EN] Same deep conversion to the glTF axis system (+Y up, right-handed) as ModelLoader::ConvertSceneConvention. It rewrites transforms and animation curves throughout the hierarchy, so the sampled local transforms are in the space the Crister's rest pose was read in. Triangulation is skipped: it only replaces mesh attributes and never changes the node hierarchy.
			/// [JP] ModelLoader::ConvertSceneConvention と同じ、glTF の軸系（+Y 上、右手系）への深い変換。階層全体のトランスフォームとアニメーションカーブを書き換えるため、サンプリングしたローカルトランスフォームは Crister のレストポーズを読んだのと同じ空間になる。三角形化はメッシュ属性を置き換えるだけでノード階層を変えないため省略する。
			FbxAxisSystem gltfAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
			gltfAxisSystem.DeepConvertScene(fbxScene);

			/// [EN] Depth-first, children in declaration order - the same walk as ModelLoader::ConvertFlatConvention, so position N here is Crister node N.
			/// [JP] 深さ優先で子は宣言順 - ModelLoader::ConvertFlatConvention と同じ走査なので、ここでの N 番目が Crister のノード N になる。
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

			/// [EN] The asset's axis convention, resolved with the same invalid-pick fallback as ModelLoader::ConvertAxisConvention.
			/// [JP] アセットの座標系規約。ModelLoader::ConvertAxisConvention と同じく、無効な選択なら既定値へフォールバックして解決する。
			ResolvedAxisConvention resolved = ResolvedAxisConvention::Resolve(axisConvention);
			if (!resolved.valid_)
			{
				resolved = ResolvedAxisConvention::Resolve(AxisConvention{});
			}

			/// [EN] Sample at the scene's own frame rate so every authored key lands on a sample; 30 fps when the file does not state one.
			/// [JP] シーン自身のフレームレートでサンプリングし、作成されたキーがすべてサンプル点に乗るようにする。ファイルに記載が無ければ 30fps。
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

				/// [EN] EvaluateLocalTransform evaluates the scene's current stack; switching it also resets the evaluator's cache.
				/// [JP] EvaluateLocalTransform はシーンの現在のスタックを評価する。切り替えると評価器のキャッシュもリセットされる。
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

				Save(animation, directory / (clipName + ".animation"));
			}

			fbxManager->Destroy();
			return;
		}

		if (path.extension() != ".gltf" && path.extension() != ".glb")
		{
			return;
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
			return;
		}

		if (model.animations.empty())
		{
			return;
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

			BinaryOutputArchive archive;
			animation.Serialize(archive);
			archive.Write(String(clipPath.string()));
		}
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

			/// [EN] Keyed by target NODE index (not the sampler's accessor
			///      index) so Animation::SamplePose can look values up by
			///      node while walking the Crister's node hierarchy.
			/// [JP] サンプラーのaccessorインデックスではなく、対象ノード番号を
			///      キーにする — Animation::SamplePoseがCristerのノード階層を
			///      辿りながらノード番号で値を引けるようにするため。
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

				/// [EN] Same right-handed to left-handed conversion ModelLoader
				///      applies to Node::rotation_ — otherwise animated nodes
				///      end up in a different handedness than static ones.
				/// [JP] ModelLoaderがNode::rotation_に適用しているのと同じ
				///      右手系→左手系変換。これをしないとアニメーションで
				///      動くノードだけ静的ノードと座標系がズレる。
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

				/// [EN] Same right-handed to left-handed conversion ModelLoader
				///      applies to Node::translation_.
				/// [JP] ModelLoaderがNode::translation_に適用しているのと
				///      同じ右手系→左手系変換。
				for (Vector3& translation : translations)
				{
					translation.x = -translation.x;
				}
			}
			else if (gltfChannel.target_path == "weights" && !animation.weights_.contains(gltfChannel.target_node))
			{
				/// [EN] glTF packs a "weights" channel's output accessor as
				///      one flat SCALAR run of keyframeCount * targetCount
				///      floats (all targets for keyframe 0, then all targets
				///      for keyframe 1, ...) rather than nesting it per
				///      keyframe like the vector types above. targetCount
				///      comes from the target node's own mesh — its default
				///      weights (Node::weights) if authored, otherwise its
				///      first primitive's morph target count — since nothing
				///      on the sampler itself records it.
				/// [JP] glTF は "weights" チャンネルの出力アクセサを、上の
				///      ベクター型のようにキーフレームごとにネストせず、
				///      keyframeCount * targetCount 個の float の単一フラット
				///      SCALAR 列として詰める(キーフレーム0の全ターゲット、
				///      続いてキーフレーム1の全ターゲット、...)。targetCount
				///      はサンプラー自体には記録されていないため、対象ノード
				///      自身のメッシュ(記述があればそのデフォルトウェイト
				///      Node::weights、無ければ最初のプリミティブのモーフ
				///      ターゲット数)から取得する。
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

		/// [EN] Some export pipelines keep each clip's keyframe times as
		///      absolute offsets into a shared master timeline instead of
		///      rebasing them to start at 0 (e.g. a clip that was originally
		///      frames 16s-24s on a combined timeline keeps those literal
		///      timestamps even once split into its own file). Rebase every
		///      timeline so this clip's own earliest keyframe becomes time 0 —
		///      otherwise SamplePose would just hold the first keyframe's pose
		///      for however many seconds it originally sat at on the shared
		///      timeline before doing anything.
		/// [JP] エクスポートツールによっては、クリップのキーフレーム時刻を
		///      切り出し元の共有タイムライン上の絶対時刻のまま(0秒に
		///      リベースせず)保持する(例: 元が結合タイムライン上の16秒〜24秒
		///      だったクリップは、個別ファイルに分割されてもその生の時刻の
		///      ままになる)。このクリップの最も早いキーフレームが時刻0に
		///      なるよう、全タイムラインをリベースする — そうしないと
		///      SamplePoseは、共有タイムライン上で最初のキーフレームが
		///      本来あった秒数だけ、そのポーズを保持し続けてしまう。
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

	/**
	* [EN]
	* Bakes one FBX animation stack into animation by sampling every
	* animated node's local transform once per frame at frameRate.
	* nodeTable is the depth-first node order ModelLoader uses for the
	* Crister's nodes_, so each channel is keyed by the same node index the
	* Crister uses. Only the T/R/S channels that actually carry keys in one
	* of the stack's layers are written; the rest fall back to the rest
	* pose at playback. The stack must already be the scene's current one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* FBX のアニメーションスタック1つを、アニメーションするノードの
	* ローカルトランスフォームを frameRate で1フレームごとにサンプリング
	* して animation へ焼き込む。nodeTable は ModelLoader が Crister の
	* nodes_ に使うのと同じ深さ優先のノード順で、各チャンネルは Crister と
	* 同じノード番号をキーにする。スタックのいずれかのレイヤーで実際に
	* キーを持つ T/R/S チャンネルだけを書き出し、それ以外は再生時に
	* レストポーズへフォールバックする。スタックは事前にシーンの現在の
	* スタックにしておくこと。
	*/
	void AnimationLoader::FetchAnimationClip(fbxsdk::FbxAnimStack* fbxStack, const DynamicArray<fbxsdk::FbxNode*>& nodeTable, Float unitScale, const ResolvedAxisConvention& resolved, Double frameRate, Animation& animation)
	{
		animation.name_ = fbxStack->GetName();

		/// [EN] Per node, whether its Lcl Translation / Lcl Rotation / Lcl Scaling carries keys in this stack. A curve node with no keyed curve (exporters often leave empty ones behind) does not count.
		/// [JP] ノードごとに、Lcl Translation / Lcl Rotation / Lcl Scaling がこのスタックでキーを持つか。キー付きカーブを持たないカーブノード（エクスポーターが空のまま残すことが多い）は数えない。
		DynamicArray<Bool> translationAnimated(nodeTable.size(), false);
		DynamicArray<Bool> rotationAnimated(nodeTable.size(), false);
		DynamicArray<Bool> scaleAnimated(nodeTable.size(), false);

		/// [EN] Earliest and latest key over every keyed curve found, the clip range used when the stack carries no time span of its own.
		/// [JP] 見つかったキー付きカーブ全体での最初と最後のキー。スタック自身が時間範囲を持たない場合のクリップ範囲になる。
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

		/// [EN] The stack's local time span is the take's authored playback range; files that leave it empty fall back to the key range.
		/// [JP] スタックのローカル時間範囲は、テイクとして作成された再生範囲。それを空のままにしているファイルはキーの範囲へフォールバックする。
		FbxTimeSpan timeSpan = fbxStack->GetLocalTimeSpan();
		FbxTime start = timeSpan.GetStart();
		FbxTime stop = timeSpan.GetStop();
		if (stop <= start)
		{
			start = keyStart;
			stop = keyStop;
		}

		/// [EN] Clip times are rebased so the range start is time 0; the last sample sits exactly on the range end.
		/// [JP] クリップの時刻は範囲の開始が 0 になるようにリベースする。最後のサンプルはちょうど範囲の終わりに置く。
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

				/// [EN] Decomposed exactly like the rest pose in ModelLoader::FetchNodes (pivots and pre/post rotation folded in), so an unanimated frame reproduces the rest pose.
				/// [JP] ModelLoader::FetchNodes のレストポーズと全く同じ方法で分解する（ピボットやプレ/ポスト回転も含む）。そのためアニメーションしていないフレームはレストポーズと一致する。
				FbxAMatrix localTransform = nodeTable[nodeIndex]->EvaluateLocalTransform(sampleTime);

				if (translationAnimated[nodeIndex])
				{
					FbxVector4 localTranslation = localTransform.GetT();
					Vector3 translation = Vector3(static_cast<Float>(localTranslation[0]), static_cast<Float>(localTranslation[1]), static_cast<Float>(localTranslation[2])) * unitScale;

					/// [EN] Same change of basis ModelLoader::ConvertAxisConvention applies to Node::translation_.
					/// [JP] ModelLoader::ConvertAxisConvention が Node::translation_ に適用するのと同じ基底変換。
					translation = Vector3::Transform(translation, resolved.basis_);
					animation.translations_[key].push_back(translation);
				}

				if (rotationAnimated[nodeIndex])
				{
					FbxQuaternion localRotation = localTransform.GetQ();
					Quaternion rotation = Quaternion(static_cast<Float>(localRotation[0]), static_cast<Float>(localRotation[1]), static_cast<Float>(localRotation[2]), static_cast<Float>(localRotation[3]));

					/// [EN] Same conjugation by the basis (R' = basis^T * R * basis) ModelLoader::ConvertAxisConvention applies to Node::rotation_.
					/// [JP] ModelLoader::ConvertAxisConvention が Node::rotation_ に適用するのと同じ、基底による共役変換 (R' = basis^T * R * basis)。
					Matrix converted = resolved.basis_.Transpose() * Matrix::CreateFromQuaternion(rotation) * resolved.basis_;
					rotation = Quaternion::CreateFromRotationMatrix(converted);

					/// [EN] q and -q are the same rotation; keep each key in the same hemisphere as the previous one so neighbouring keys interpolate along the short arc.
					/// [JP] q と -q は同じ回転。各キーを直前のキーと同じ半球に揃え、隣り合うキー同士が短い弧で補間されるようにする。
					DynamicArray<Quaternion>& rotations = animation.rotations_[key];
					if (!rotations.empty() && rotations.back().Dot(rotation) < 0.0f)
					{
						rotation = Quaternion(-rotation.x, -rotation.y, -rotation.z, -rotation.w);
					}
					rotations.push_back(rotation);
				}

				if (scaleAnimated[nodeIndex])
				{
					/// [EN] Scale is left in source axes, matching Node::scale_ in ModelLoader::ConvertAxisConvention.
					/// [JP] スケールはソースの軸のままにする。ModelLoader::ConvertAxisConvention での Node::scale_ と同じ扱い。
					FbxVector4 localScale = localTransform.GetS();
					animation.scales_[key].push_back(Vector3(static_cast<Float>(localScale[0]), static_cast<Float>(localScale[1]), static_cast<Float>(localScale[2])));
				}
			}
		}

		animation.duration_ = static_cast<Float>(durationSeconds);
	}
}

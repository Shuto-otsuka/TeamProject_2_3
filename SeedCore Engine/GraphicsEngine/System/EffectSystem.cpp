#include <GraphicsEngine/System/EffectSystem.h>
#include <GraphicsEngine/Effect/EffectResource.h>
#include <GraphicsEngine/Effect/EffectAsset.h>
#include <GraphicsEngine/Effect/Effect.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerResource.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerEffect.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	void EffectSystem::Update(EffectResource& effectResource, EffekseerResource& effekseerResource, World& world, Float deltaTime)
	{
		EffekseerManager& effekseerManager = Gateway::GetEffekseerManager();

		Query<Read<Active>, Write<Effect>> query(world);
		query.ForEach([&](EntityID entityID, const Active& active, Effect& effect)
			{
				const EffectAsset* asset = effectResource.Resolve(effectResource.GetHandle(effect.effectID_));
				if (!active.active_ || !asset)
				{
					effect.delta_ = 0.0f;
					std::ranges::fill(effect.spawnCounts_, 0u);
					for (const EffectPlayback& playback : effect.playbacks_)
					{
						if (playback.handle_ >= 0)
						{
							effekseerManager.Stop(playback.handle_);
						}
					}
					effect.playbacks_.clear();
					return;
				}

				effect.delta_ = deltaTime;
				effect.age_ += deltaTime;
				effect.spawnAccumulators_.resize(asset->emitters_.size(), 0.0f);
				effect.spawnCounts_.resize(asset->emitters_.size(), 0);
				for (Size emitterIndex = 0;emitterIndex < asset->emitters_.size();++emitterIndex)
				{
					const EffectNode* receiver = asset->emitters_[emitterIndex].Find(String("Emitter"));
					Float rate = receiver ? receiver->Value(String("Rate")).x : 0.0f;
					effect.spawnAccumulators_[emitterIndex] += rate * deltaTime;
					effect.spawnCounts_[emitterIndex] = static_cast<Uint32>(effect.spawnAccumulators_[emitterIndex]);
					effect.spawnAccumulators_[emitterIndex] -= static_cast<Float>(effect.spawnCounts_[emitterIndex]);
				}

				SeedCore::erase_if(effect.playbacks_, [&](const EffectPlayback& playback)
					{
						Bool alive = playback.emitter_ < asset->emitters_.size() && asset->emitters_[playback.emitter_].enabled_ && std::ranges::any_of(asset->emitters_[playback.emitter_].nodes_, [&playback](const EffectNode& node) { return node.id_ == playback.node_ && node.type_ == String("Effekseer"); });
						if (!alive && playback.handle_ >= 0)
						{
							effekseerManager.Stop(playback.handle_);
						}
						return !alive;
					});

				Actor actor = WorldAccessor::Make(world, entityID);
				Matrix worldMatrix = actor ? actor.WorldMatrix() : Matrix::Identity;

				for (Size emitterIndex = 0;emitterIndex < asset->emitters_.size();++emitterIndex)
				{
					const EffectEmitter& emitter = asset->emitters_[emitterIndex];
					if (!emitter.enabled_)
					{
						continue;
					}

					for (const EffectNode& node : emitter.nodes_)
					{
						if (node.type_ != String("Effekseer"))
						{
							continue;
						}

						auto found = std::ranges::find_if(effect.playbacks_, [emitterIndex, &node](const EffectPlayback& playback) { return playback.emitter_ == emitterIndex && playback.node_ == node.id_; });
						if (found == effect.playbacks_.end())
						{
							EffectPlayback& added = effect.playbacks_.emplace_back();
							added.emitter_ = static_cast<Uint32>(emitterIndex);
							added.node_ = node.id_;
							found = effect.playbacks_.end() - 1;
						}
						EffectPlayback& playback = *found;

						auto parameter = std::ranges::find_if(node.parameters_, [](const EffectParameter& candidate) { return candidate.name_ == String("Asset"); });
						Uint32 assetID = parameter != node.parameters_.end() ? parameter->asset_ : 0;
						if (playback.asset_ != assetID)
						{
							if (playback.handle_ >= 0)
							{
								effekseerManager.Stop(playback.handle_);
							}
							playback.asset_ = assetID;
							playback.handle_ = -1;
							playback.finished_ = false;
						}

						EffekseerEffect* source = effekseerResource.Resolve(effekseerResource.GetHandle(assetID));

						if (!source)
						{
							continue;
						}

						Bool loop = node.Value(String("Loop")).x != 0.0f;
						if (playback.handle_ >= 0 && !effekseerManager.Playing(playback.handle_))
						{
							playback.handle_ = -1;
							playback.finished_ = !loop;
						}
						if (playback.handle_ < 0 && !playback.finished_ && effect.age_ >= node.Value(String("Delay")).x)
						{
							playback.handle_ = effekseerManager.Play(source->Effect(), Vector3(0.0f, 0.0f, 0.0f));
						}
						if (playback.handle_ < 0)
						{
							continue;
						}

						Vector4 offset = node.Value(String("Offset"));
						Vector4 rotation = node.Value(String("Rotation"));
						Vector4 scale = node.Value(String("Scale"));
						Vector4 color = node.Value(String("Color"));
						Matrix local = Matrix::CreateTranslationRotationScale(offset.xyz, Quaternion::CreateFromYawPitchRoll(ToRadians(rotation.y), ToRadians(rotation.x), ToRadians(rotation.z)), scale.xyz);
						effekseerManager.Transform(playback.handle_, local* worldMatrix);
						effekseerManager.Speed(playback.handle_, node.Value(String("Speed")).x);
						effekseerManager.Color(playback.handle_, Color(color));
					}
				}
			});
	}
}
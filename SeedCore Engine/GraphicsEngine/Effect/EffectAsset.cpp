#include <GraphicsEngine/Effect/EffectAsset.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyrNodeRegistry.h>

namespace SeedCore
{
	Vector4 EffectNode::Value(const String& input)const
	{
		auto found = std::ranges::find_if(parameters_, [&input](const EffectParameter& parameter) { return parameter.name_ == input; });
		if (found != parameters_.end())
		{
			return found->value_;
		}

		const ZephyrNodeDefinition* definition = ZephyrNodeRegistry::Find(type_);
		if (!definition)
		{
			return Vector4::Zero;
		}
		auto defined = std::ranges::find_if(definition->inputs_, [&input](const ZephyrNodeInput& candidate) { return candidate.name_ == input; });
		if (defined == definition->inputs_.end())
		{
			return Vector4::Zero;
		}
		return defined->default_;
	}
}

namespace SeedCore
{
	EffectEmitter EffectEmitter::Create(const String& name)
	{
		EffectEmitter emitter;
		emitter.name_ = name;
		for (const ZephyrNodeDefinition& definition : ZephyrNodeRegistry::Nodes())
		{
			if (definition.removable_)
			{
				continue;
			}

			EffectNode& node = emitter.nodes_.emplace_back();
			node.id_ = static_cast<Uint32>(emitter.nodes_.size());
			node.type_ = definition.type_;
			node.position_ = Vector2(0.0f, static_cast<Float>(emitter.nodes_.size() - 1) * 200.0f);
		}
		return emitter;
	}

	const EffectNode* EffectEmitter::Find(const String& type)const
	{
		auto found = std::ranges::find_if(nodes_, [&type](const EffectNode& node) { return node.type_ == type; });
		if (found == nodes_.end())
		{
			return nullptr;
		}
		return &(*found);
	}
}
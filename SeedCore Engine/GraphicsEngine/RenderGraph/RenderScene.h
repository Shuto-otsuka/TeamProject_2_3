#pragma once
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	struct RenderGraphImport;

	class World;
	class GraphicsResourceSystem;

	class RenderTypeKey
	{
	public:
		template<class Type>
		NODISCARD static constexpr const void* Of()
		{
			return &key_<Type>;
		}

	private:
		template<class Type>
		SC_CONSTEX Byte key_ = 0;
	};

	struct RenderScene
	{
		World* world_ = nullptr;

		GraphicsResourceSystem* resource_ = nullptr;

		std::span<const RenderView> views_;

		std::span<const RenderGraphImport> imports_;

		DynamicArray<std::pair<const void*, const void*>> values_;

		template<class Type>
		void Set(const Type& value)
		{
			values_.push_back({ RenderTypeKey::Of<Type>(), &value });
		}

		template<class Type>
		NODISCARD const Type* Find()const
		{
			for (const std::pair<const void*, const void*>& value : values_)
			{
				if (value.first == RenderTypeKey::Of<Type>())
				{
					return static_cast<const Type*>(value.second);
				}
			}
			return nullptr;
		}
	};
}

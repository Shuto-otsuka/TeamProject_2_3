#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	enum class RenderResourceKind :Uint8
	{
		Contract,
		Internal,
		Accumulation,
	};

	enum class RenderFallback :Uint8
	{
		None,
		Zero,
		One,
		Alpha,
	};

	struct RenderResourceDeclaration
	{
		RenderResource resource_;

		RenderResourceKind kind_ = RenderResourceKind::Internal;

		RenderFallback fallback_ = RenderFallback::None;
	};
}
#pragma once
#include <GraphicsEngine/RenderGraph/RenderResource.h>

namespace SeedCore
{
	enum class RenderResourceKind :Uint8
	{
		Contract,
		Internal,
		Accumulation,
		Debug,
	};

	enum class RenderResourceLifetime :Uint8
	{
		Transient,
		Persistent,
		Imported,
		Upload,
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

		RenderResourceLifetime lifetime_ = RenderResourceLifetime::Transient;

		RenderFallback fallback_ = RenderFallback::None;

		RenderResource passThrough_ = {};
	};
}
#pragma once
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	struct EditorContext;

	class ImGuiTexture
	{
	public:
		ImGuiTexture(EditorContext& context);
		~ImGuiTexture() = default;

		NODISCARD Uint64 Icon(const String& name)const;

		NODISCARD Uint64 Icon(const String& name, const String& fallback)const;

	private:
		FlatMap<String, Uint64> icons_;

		Uint64 missing_ = {};

		mutable DynamicArray<String> reported_;

		DynamicArray<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
	};
}

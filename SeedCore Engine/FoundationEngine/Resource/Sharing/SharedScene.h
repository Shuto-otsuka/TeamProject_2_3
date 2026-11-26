#pragma once
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>

namespace SeedCore
{
	struct ScenePart
	{
		String scope_;

		Uint64 revision_ = 0;

		String hash_;

		String data_;

		String driveId_;

		Uint64 size_ = 0;

		Bool deleted_ = false;
	};

	struct ScenePartChange
	{
		String scope_;

		Uint64 baseRevision_ = 0;

		String hash_;

		String data_;

		String driveId_;

		Uint64 size_ = 0;

		Bool deleted_ = false;
	};

	class SEEDCORE_API SharedScene
	{
	public:
		SharedScene(GoogleDocument& document, const String& documentId);

		~SharedScene();

		SharedScene(const SharedScene&) = delete;

		SharedScene& operator=(const SharedScene&) = delete;

		Bool Refresh();

		const DynamicArray<ScenePart>& Parts()const;

		const ScenePart* Find(const String& scope)const;

		Bool Publish(const DynamicArray<ScenePartChange>& changes);

		const String& Error()const;

	private:
		Bool Modify(const DynamicArray<ScenePartChange>& changes);

	private:
		GoogleDocument& document_;

		String documentId_;

		DynamicArray<ScenePart> parts_;

		String error_;
	};
}

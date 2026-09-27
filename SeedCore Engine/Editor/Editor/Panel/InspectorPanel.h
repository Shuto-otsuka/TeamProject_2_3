#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <Editor/Editor/Panel/AddComponentPanel.h>

	namespace SeedCore
{
	struct EditorContext;
	class ImGuiTexture;
	class Actor;

	class InspectorPanel
	{
	public:
		InspectorPanel(EditorContext& context, ImGuiTexture& imguiTexture);
		~InspectorPanel() = default;

		void Draw();

	private:
		void DrawName(Actor actor);

		void DrawTags(Actor actor);

		void DrawLayer(Actor actor);

		void DrawPrefabControls(Actor actor);

		void DrawComponents(Actor actor);

		/// [EN] Shared header/popup/fields block for one component - see the
		///      .cpp definition for why it exists (archetype-layout and
		///      sparse-set components are discovered two different ways but
		///      drawn identically). Returns true if the component was removed
		///      this frame.
		/// [JP] 1コンポーネントぶんのヘッダー/ポップアップ/フィールドの共有
		///      ブロック。存在理由は .cpp の定義コメント参照(アーキタイプ一覧と
		///      スパースセットは見つけ方が違うが描画は同一)。このフレームで
		///      削除されたら true。
		Bool DrawComponentEntry(Actor actor, ComponentID componentID, const String& componentName, void* componentData);

		/**
		* [EN]
		* Draws one header per component whose type this build does not know,
		* labelled "Unknown Component". Its real name and fields appear only
		* once its script is registered and it turns back into the real
		* component; until then it can only be removed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この実行ファイルが型を知らないコンポーネントごとに、
		* 「Unknown Component」という名前でヘッダーを1つずつ描く。本来の名前と
		* フィールドが出るのは、そのスクリプトが登録されて本来のコンポーネントへ
		* 戻ってから。それまでは削除だけができる。
		*/
		void DrawUnknownComponents(Actor actor);

		void DrawReflectedFields(String componentName, void* componentData, ComponentID componentID, Entity entity);

		void DrawFieldList(DynamicArray<FieldInfo>& fields, void* baseData, Entity entity, ComponentID componentID, Size baseOffset);

		void DrawField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset);

		void DrawPayloadField(const FieldInfo& field, void* pointer, Entity entity, ComponentID componentID, Size fieldOffset);

		void DrawPayloadArrayRow(const FieldInfo& field, void* pointer);

		void DrawPayloadArrayAppendSlot(const FieldInfo& field, const DynamicArray<Int>& existingValues, Entity entity, ComponentID componentID);

		void DrawTransform(void* componentData, const Char* label, Bool& linked, Float* previousValues, Entity entity, ComponentID componentID);

		const Char* GetPayloadDropType(PayloadAssetType assetType)const;

		[[nodiscard]] ImTextureID GetComponentIcon(const String& componentName)const;

	private:
		EditorContext& context_;

		AddComponentPanel addComponentPanel_;

		Bool positionLinked_ = false;
		Bool rotationLinked_ = false;
		Bool scaleLinked_ = false;

		Float previousPosition_[3] = {};
		Float previousRotation_[3] = {};
		Float previousScale_[3] = {};

		Bool locked_ = false;
		Actor lockedActor_;

		std::string newTagBuffer_;

		DynamicArray<std::string> layerNameBuffers_;

		ImGuiTexture& imguiTexture_;

		/// [EN] Value of the field currently being dragged/typed into, captured on ImGui::IsItemActivated() and diffed against the field's value on ImGui::IsItemDeactivatedAfterEdit() to build an undo Command. Only one of these is meaningful at a time, since ImGui allows at most one active item.
		/// [JP] 現在ドラッグ/入力中のフィールドの値。ImGui::IsItemActivated()時点で捕捉し、ImGui::IsItemDeactivatedAfterEdit()時点のフィールド値と比較してundo Commandを組み立てる。ImGuiのアクティブアイテムは常に高々1つのため、これらのうち意味を持つのは同時に1つだけ。
		Int pendingOldInt_ = 0;
		Float pendingOldFloat_ = 0.0f;
		Vector2 pendingOldVector2_ = Vector2::Zero;
		Vector3 pendingOldVector3_ = Vector3::Zero;

		/// [EN] Quaternion captured before an Inspector rotation edit, used to restore exact stored orientation on Undo.
		/// [JP] Inspector の回転編集前に捕捉したクォータニオン。Undo 時に保持していた正確な姿勢を復元するために使う。
		Quaternion pendingOldQuaternion_ = Quaternion::Identity;

		/// [EN] Euler degree values retained while the represented Quaternion is unchanged, so the Inspector does not replace an entered equivalent Euler representation with a canonical one.
		/// [JP] Rotation ウィジェットで保持する度数のオイラー値。同一の Quaternion を表示する間は、正規化された別表現へ置き換えないために使う。
		Vector3 pendingRotationDegrees_ = Vector3::Zero;

		/// [EN] Entity for which pendingRotationDegrees_ was captured.
		/// [JP] pendingRotationDegrees_ を取得した Entity。
		EntityID pendingRotationEntity_;

		/// [EN] Quaternion corresponding to pendingRotationDegrees_; an external rotation update invalidates the displayed Euler cache.
		/// [JP] pendingRotationDegrees_ に対応する Quaternion。外部から回転が更新された場合に Euler 表示キャッシュを無効化するために使う。
		Quaternion pendingRotationQuaternion_ = Quaternion::Identity;

		/// [EN] Whether the Inspector currently has a valid Euler display cache for Rotation.
		/// [JP] Inspector が Rotation 用の有効な Euler 表示キャッシュを持っているかどうか。
		Bool hasPendingRotation_ = false;

		Color pendingOldColor_ = Color(0.0f, 0.0f, 0.0f, 0.0f);
		String pendingOldString_;
	};
}

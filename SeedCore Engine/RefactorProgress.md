# リファクタ・コメント付け 進捗

- [x] = 直した / [ ] = まだ。直したファイルの横に、指示の要点を一行で残す。
- 各モジュール見出しの (済/全体) は更新のたびに書き換える。
- 進め方・規約・未解決事項は RefactorHandoff.md を参照。

## チェックポイント

- 2026-09-21：AudioEngine の Audio.h/.cpp、AudioByteStream.h/.cpp まで完了（AudioEngine 4/22）。次は AudioEngine/Audio/AudioListener.h/.cpp（一覧の上から順に進めている）。
- 2026-09-22：PhysicsEngine の Physics.h/.cpp、PhysicsSystem.h/.cpp、Rigidbody.h/.cpp、Softbody.h/.cpp まで完了（PhysicsEngine 46/47、AudioEngine 18/22）。PhysicsEngine の残りは Prelude.cpp のみ。
- 2026-09-22：AudioEngine を再訪。Get/Set を外す改名（MixerSystem / CriManager / Audio）、SetSound → Build、ResolveSounds → ResolveSound、CRI の3ファイルにコメント（AudioEngine 21/22、残りは Prelude.cpp のみ）。
- 2026-09-24：FoundationEngine の JobSystem・Log 全ファイルと、Utility の Array/ArtMap/Bitset/Delegate/DynamicArray/StaticArray/FlatMap/Handle/NonCopyable/NonMovable/NonTransferable/ReadWrite/ResourcePtr/ResourceRef/String/Types に、Sharing と同じ密度でコメント付け（FoundationEngine 97/223）。コードは変更なし。コメント作業中に見つけた実装上の問題は RefactorHandoff.md の「気づいたが未対応」に記録。
- 2026-09-24：同日に見つけたバグを全て修正。JobSystem は Taskflow master と見比べて、抜けていた例外処理（try/catch・明示アンカー・再送出）などを追加（ビルド未確認、詳細は RefactorHandoff.md）。
- 2026-09-24：FoundationEngine の Utility と JobSystem を完了（IndexRange.h は未使用のため削除）。FoundationEngine 99/222。

## FoundationEngine (99/222)

### FoundationEngine/File
- [ ] FoundationEngine/File/FileDialog.cpp
- [ ] FoundationEngine/File/FileDialog.h
- [ ] FoundationEngine/File/FileDirectory.cpp
- [ ] FoundationEngine/File/FileDirectory.h
- [ ] FoundationEngine/File/FileUtility.cpp
- [ ] FoundationEngine/File/FileUtility.h

### FoundationEngine/Input
- [ ] FoundationEngine/Input/Input.cpp
- [ ] FoundationEngine/Input/Input.h
- [ ] FoundationEngine/Input/InputSystem.cpp
- [ ] FoundationEngine/Input/InputSystem.h

### FoundationEngine/Interop
- [ ] FoundationEngine/Interop/ColliderInstance.h

### FoundationEngine/JobSystem
- [x] FoundationEngine/JobSystem/AtomicNotifier.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/FlowBuilder.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/FlowBuilder.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobCommon.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobConcept.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobDeclaretions.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobExecutor.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobExecutor.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobGraph.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobGraph.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobNode.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobNode.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobNodeBase.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobNodeBase.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobRuntime.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobRuntime.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTask.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTask.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTaskflow.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTaskflow.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTopology.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobTopology.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobVector.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobVector.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobWorker.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/JobWorker.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/NonblockingNotifier.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/Semaphore.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/Semaphore.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/WorkerCommon.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/JobSystem/WorkerQueue.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）

### FoundationEngine/Log
- [x] FoundationEngine/Log/AftermathCrashTracker.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/AftermathCrashTracker.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/Assert.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/DxFail.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/Error.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/Exeption.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/LogSystem.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/LogSystem.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/Notice.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/SlFail.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Log/Warning.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）

### FoundationEngine/Math
- [ ] FoundationEngine/Math/Algorithm.h
- [ ] FoundationEngine/Math/Halton.h
- [ ] FoundationEngine/Math/MathCommon.h
- [ ] FoundationEngine/Math/Matrix.h
- [ ] FoundationEngine/Math/Quaternion.h

### FoundationEngine/Math/Random
- [ ] FoundationEngine/Math/Random/Hash.cpp
- [ ] FoundationEngine/Math/Random/Hash.h
- [ ] FoundationEngine/Math/Random/Xorshift.h

### FoundationEngine/Math
- [ ] FoundationEngine/Math/Ray.h
- [ ] FoundationEngine/Math/Vector.h

### FoundationEngine/Memory
- [ ] FoundationEngine/Memory/ChunkAllocator.h

### FoundationEngine/Payload
- [ ] FoundationEngine/Payload/Payload.generated.cpp
- [ ] FoundationEngine/Payload/PayloadRegistry.cpp
- [ ] FoundationEngine/Payload/PayloadRegistry.h

### FoundationEngine/Plugin
- [ ] FoundationEngine/Plugin/PluginHost.cpp
- [ ] FoundationEngine/Plugin/PluginHost.h
- [ ] FoundationEngine/Plugin/PluginModule.cpp
- [ ] FoundationEngine/Plugin/PluginModule.h

### FoundationEngine/Pool
- [ ] FoundationEngine/Pool/InternPool.cpp
- [ ] FoundationEngine/Pool/InternPool.h
- [ ] FoundationEngine/Pool/ObjectPool.cpp
- [ ] FoundationEngine/Pool/ObjectPool.h
- [ ] FoundationEngine/Pool/StablePool.h

### FoundationEngine
- [ ] FoundationEngine/Prelude.cpp
- [ ] FoundationEngine/Prelude.h

### FoundationEngine/Reflection
- [ ] FoundationEngine/Reflection/Reflection.generated.cpp
- [ ] FoundationEngine/Reflection/ReflectionRegistry.cpp
- [ ] FoundationEngine/Reflection/ReflectionRegistry.h

### FoundationEngine/Resource/Asset
- [ ] FoundationEngine/Resource/Asset/Asset.cpp
- [ ] FoundationEngine/Resource/Asset/Asset.h
- [ ] FoundationEngine/Resource/Asset/AxisConvention.cpp
- [ ] FoundationEngine/Resource/Asset/AxisConvention.h

### FoundationEngine/Resource/Config
- [ ] FoundationEngine/Resource/Config/BootConfig.cpp
- [ ] FoundationEngine/Resource/Config/BootConfig.h
- [ ] FoundationEngine/Resource/Config/EditorConfig.cpp
- [ ] FoundationEngine/Resource/Config/EditorConfig.h
- [ ] FoundationEngine/Resource/Config/GameConfig.cpp
- [ ] FoundationEngine/Resource/Config/GameConfig.h
- [ ] FoundationEngine/Resource/Config/IconConfig.cpp
- [ ] FoundationEngine/Resource/Config/IconConfig.h

### FoundationEngine/Resource
- [ ] FoundationEngine/Resource/Gateway.cpp
- [ ] FoundationEngine/Resource/Gateway.h
- [ ] FoundationEngine/Resource/LoaderSystem.cpp
- [ ] FoundationEngine/Resource/LoaderSystem.h

### FoundationEngine/Resource/Prefab
- [ ] FoundationEngine/Resource/Prefab/Prefab.cpp
- [ ] FoundationEngine/Resource/Prefab/Prefab.h
- [ ] FoundationEngine/Resource/Prefab/PrefabPool.cpp
- [ ] FoundationEngine/Resource/Prefab/PrefabPool.h

### FoundationEngine/Resource
- [ ] FoundationEngine/Resource/ResourceCache.cpp
- [ ] FoundationEngine/Resource/ResourceCache.h

### FoundationEngine/Resource/Scene
- [ ] FoundationEngine/Resource/Scene/Scene.cpp
- [ ] FoundationEngine/Resource/Scene/Scene.h
- [ ] FoundationEngine/Resource/Scene/ScenePool.cpp
- [ ] FoundationEngine/Resource/Scene/ScenePool.h
- [ ] FoundationEngine/Resource/Scene/SceneTransitionSystem.cpp
- [ ] FoundationEngine/Resource/Scene/SceneTransitionSystem.h

### FoundationEngine
- [ ] FoundationEngine/SeedScript.h

### FoundationEngine/Serialization/Binary
- [ ] FoundationEngine/Serialization/Binary/BinaryArchive.cpp
- [ ] FoundationEngine/Serialization/Binary/BinaryArchive.h

### FoundationEngine/Serialization/Encryption
- [ ] FoundationEngine/Serialization/Encryption/Aes256.cpp
- [ ] FoundationEngine/Serialization/Encryption/Aes256.h
- [ ] FoundationEngine/Serialization/Encryption/Sha256.cpp
- [ ] FoundationEngine/Serialization/Encryption/Sha256.h

### FoundationEngine/Serialization/Json
- [ ] FoundationEngine/Serialization/Json/JsonArchive.cpp
- [ ] FoundationEngine/Serialization/Json/JsonArchive.h

### FoundationEngine/Serialization
- [ ] FoundationEngine/Serialization/SerializeSupport.h

### FoundationEngine/Time
- [ ] FoundationEngine/Time/GameTimer.cpp
- [ ] FoundationEngine/Time/GameTimer.h
- [ ] FoundationEngine/Time/Timer.cpp
- [ ] FoundationEngine/Time/Timer.h
- [ ] FoundationEngine/Time/WorldTimer.cpp
- [ ] FoundationEngine/Time/WorldTimer.h

### FoundationEngine/Utility
- [x] FoundationEngine/Utility/Array.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/ArtMap.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Bitset.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Bitset.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Bootstrap.h — 手を付けずに完了扱い（Utility 一式をユーザーが完了と判断）
- [x] FoundationEngine/Utility/Delegate.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Delegate.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/DestructiveCopy.h — 手を付けずに完了扱い（Utility 一式をユーザーが完了と判断）
- [x] FoundationEngine/Utility/DynamicArray.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/FlatMap.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Handle.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/NonCopyable.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/NonMovable.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/NonTransferable.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/ReadWrite.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/ResourcePtr.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/ResourceRef.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/StaticArray.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/String.cpp — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/String.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）
- [x] FoundationEngine/Utility/Types.h — 日英コメント付け（関数本体の処理コメントを補強、実装と食い違う既存コメントを修正）

### FoundationEngine/World/Actor
- [ ] FoundationEngine/World/Actor/Actor.cpp
- [ ] FoundationEngine/World/Actor/Actor.h
- [ ] FoundationEngine/World/Actor/Blueprint.cpp
- [ ] FoundationEngine/World/Actor/Blueprint.h

### FoundationEngine/World/Command
- [ ] FoundationEngine/World/Command/ActorCommand.cpp
- [ ] FoundationEngine/World/Command/ActorCommand.h
- [ ] FoundationEngine/World/Command/ArrayFieldCommand.cpp
- [ ] FoundationEngine/World/Command/ArrayFieldCommand.h
- [ ] FoundationEngine/World/Command/Command.h
- [ ] FoundationEngine/World/Command/CommandBuffer.cpp
- [ ] FoundationEngine/World/Command/CommandBuffer.h
- [ ] FoundationEngine/World/Command/ComponentCommand.h
- [ ] FoundationEngine/World/Command/ComponentLifecycleCommand.cpp
- [ ] FoundationEngine/World/Command/ComponentLifecycleCommand.h
- [ ] FoundationEngine/World/Command/CompoundCommand.cpp
- [ ] FoundationEngine/World/Command/CompoundCommand.h
- [ ] FoundationEngine/World/Command/History.cpp
- [ ] FoundationEngine/World/Command/History.h

### FoundationEngine/World/ECS/Archetype
- [x] FoundationEngine/World/ECS/Archetype/Archetype.cpp — 関数本体のコメントを補強
- [x] FoundationEngine/World/ECS/Archetype/Archetype.h
- [x] FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.cpp — 関数本体のコメントを補強
- [x] FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h
- [x] FoundationEngine/World/ECS/Archetype/Chunk.cpp — 定数を camelCase_ に（chunkSize_ など）、関数本体のコメントを補強
- [x] FoundationEngine/World/ECS/Archetype/Chunk.h — private の定数と変数を末尾へ移し区切りを分割、定数を camelCase_ に

### FoundationEngine/World/ECS/Component
- [x] FoundationEngine/World/ECS/Component/Active.h
- [x] FoundationEngine/World/ECS/Component/Awakeable.h
- [x] FoundationEngine/World/ECS/Component/Bounds.h
- [x] FoundationEngine/World/ECS/Component/Collisionable.h
- [x] FoundationEngine/World/ECS/Component/Component.h — 長い /// を4行以内に、EcsID.h を廃止して ComponentID をここへ移動
- [x] FoundationEngine/World/ECS/Component/ComponentBehaviour.cpp — ComponentBase → ComponentBehaviour、ResetLifecycleState → ResetLifecycle（private、WorldSnapshot を friend）、DispatchDestroy を public へ戻し SeedScript で隠す
- [x] FoundationEngine/World/ECS/Component/ComponentBehaviour.h — 同上、関連名も改名（is_component_behaviour_tag など）
- [x] FoundationEngine/World/ECS/Component/ComponentConcept.h — EcsConcept.h から改名
- [x] FoundationEngine/World/ECS/Component/ComponentRegistry.cpp — GetComponentSize/Alignment/List・GetName・GetRegistry の Get を外す、private Registry → MetadataMap、NameToID → NameIndex、並び整理（Unregister を Register の下、private は MetadataMap/NameMap/TypeIndex/NameIndex/InternalID）、.cpp のコメントをヘッダーに合わせる
- [x] FoundationEngine/World/ECS/Component/ComponentRegistry.h — 同上、関数と変数の private を分割、SEED_TRAITS_SPEC のコメントを /** */ に
- [x] FoundationEngine/World/ECS/Component/Destroyable.h
- [x] FoundationEngine/World/ECS/Component/FixedTick.h
- [x] FoundationEngine/World/ECS/Component/InspectorDrawable.h
- [x] FoundationEngine/World/ECS/Component/LateTickable.h
- [x] FoundationEngine/World/ECS/Component/Lifetime.h
- [x] FoundationEngine/World/ECS/Component/Name.h
- [x] FoundationEngine/World/ECS/Component/Position.h
- [x] FoundationEngine/World/ECS/Component/Rotation.h
- [x] FoundationEngine/World/ECS/Component/Scale.h
- [x] FoundationEngine/World/ECS/Component/Spawner.h
- [x] FoundationEngine/World/ECS/Component/Startable.h
- [x] FoundationEngine/World/ECS/Component/Tickable.h
- [x] FoundationEngine/World/ECS/Component/Triggerable.h
- [x] FoundationEngine/World/ECS/Component/Velocity.h

### FoundationEngine/World/ECS/Entity
- [x] FoundationEngine/World/ECS/Entity/Entity.cpp
- [x] FoundationEngine/World/ECS/Entity/Entity.h
- [x] FoundationEngine/World/ECS/Entity/EntityRecord.cpp
- [x] FoundationEngine/World/ECS/Entity/EntityRecord.h

### FoundationEngine/World/ECS/Query
- [x] FoundationEngine/World/ECS/Query/Query.h — 関数本体のコメントを詳しく補強

### FoundationEngine/World/ECS/SparseSet
- [x] FoundationEngine/World/ECS/SparseSet/SparseSet.h — 関数本体のコメントを詳しく補強、SparseSetStorage::data_ を private にして World を friend に

### FoundationEngine/World/ECS/System
- [ ] FoundationEngine/World/ECS/System/LifetimeSystem.cpp
- [ ] FoundationEngine/World/ECS/System/LifetimeSystem.h
- [ ] FoundationEngine/World/ECS/System/MoveSystem.cpp
- [ ] FoundationEngine/World/ECS/System/MoveSystem.h
- [ ] FoundationEngine/World/ECS/System/SpawnerSystem.cpp
- [ ] FoundationEngine/World/ECS/System/SpawnerSystem.h
- [ ] FoundationEngine/World/ECS/System/SystemGraph.cpp
- [ ] FoundationEngine/World/ECS/System/SystemGraph.h
- [ ] FoundationEngine/World/ECS/System/SystemScheduler.cpp
- [ ] FoundationEngine/World/ECS/System/SystemScheduler.h
- [ ] FoundationEngine/World/ECS/System/TransformSystem.cpp
- [ ] FoundationEngine/World/ECS/System/TransformSystem.h

### FoundationEngine/World/Layer
- [ ] FoundationEngine/World/Layer/LayerCollisionMatrix.cpp
- [ ] FoundationEngine/World/Layer/LayerCollisionMatrix.h
- [ ] FoundationEngine/World/Layer/LayerRegistry.cpp
- [ ] FoundationEngine/World/Layer/LayerRegistry.h

### FoundationEngine/World/Tag
- [ ] FoundationEngine/World/Tag/TagRegistry.cpp
- [ ] FoundationEngine/World/Tag/TagRegistry.h

### FoundationEngine/World
- [ ] FoundationEngine/World/World.cpp
- [ ] FoundationEngine/World/World.h
- [ ] FoundationEngine/World/WorldSnapshot.cpp
- [ ] FoundationEngine/World/WorldSnapshot.h

## GraphicsEngine (0/649)

### GraphicsEngine/Avatar/Animal
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterConverter.cpp
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterConverter.h
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterEvaluator.cpp
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterEvaluator.h
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterModel.cpp
- [ ] GraphicsEngine/Avatar/Animal/AnimalCharacterModel.h

### GraphicsEngine/Avatar
- [ ] GraphicsEngine/Avatar/AvatarMesh.cpp
- [ ] GraphicsEngine/Avatar/AvatarMesh.h
- [ ] GraphicsEngine/Avatar/AvatarPreviewPS.hlsl

### GraphicsEngine/Avatar/Human
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterConverter.cpp
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterConverter.h
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterEvaluator.cpp
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterEvaluator.h
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterModel.cpp
- [ ] GraphicsEngine/Avatar/Human/HumanCharacterModel.h

### GraphicsEngine/Camera
- [ ] GraphicsEngine/Camera/Camera.cpp
- [ ] GraphicsEngine/Camera/Camera.h
- [ ] GraphicsEngine/Camera/CameraBrain.cpp
- [ ] GraphicsEngine/Camera/CameraBrain.h
- [ ] GraphicsEngine/Camera/CanvasCamera.cpp
- [ ] GraphicsEngine/Camera/CanvasCamera.h
- [ ] GraphicsEngine/Camera/EditorCamera.cpp
- [ ] GraphicsEngine/Camera/EditorCamera.h
- [ ] GraphicsEngine/Camera/EditorCameraController.cpp
- [ ] GraphicsEngine/Camera/EditorCameraController.h
- [ ] GraphicsEngine/Camera/PreviewCamera.cpp
- [ ] GraphicsEngine/Camera/PreviewCamera.h
- [ ] GraphicsEngine/Camera/PreviewCameraController.cpp
- [ ] GraphicsEngine/Camera/PreviewCameraController.h
- [ ] GraphicsEngine/Camera/ScreenSpace.cpp
- [ ] GraphicsEngine/Camera/ScreenSpace.h

### GraphicsEngine/Constraint
- [ ] GraphicsEngine/Constraint/AttachmentConstraint.cpp
- [ ] GraphicsEngine/Constraint/AttachmentConstraint.h
- [ ] GraphicsEngine/Constraint/IKConstraint.cpp
- [ ] GraphicsEngine/Constraint/IKConstraint.h
- [ ] GraphicsEngine/Constraint/LookAtConstraint.h
- [ ] GraphicsEngine/Constraint/ParentConstraint.h
- [ ] GraphicsEngine/Constraint/PositionConstraint.h
- [ ] GraphicsEngine/Constraint/RotationConstraint.h

### GraphicsEngine/D3D12/Buffer
- [ ] GraphicsEngine/D3D12/Buffer/ArgumentBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/ArgumentBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/Buffer.h
- [ ] GraphicsEngine/D3D12/Buffer/ConstantBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/DepthResizeBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/DepthResizeBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/FrameBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/FrameBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/GeometryBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/GeometryBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/HiZBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/HiZBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/HudlessBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/HudlessBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/IndexBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/IndexBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/ReservoirBuffer.cpp
- [ ] GraphicsEngine/D3D12/Buffer/ReservoirBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/StructuredBuffer.h
- [ ] GraphicsEngine/D3D12/Buffer/VertexBuffer.h

### GraphicsEngine/D3D12/Context
- [ ] GraphicsEngine/D3D12/Context/D3D12Adapter.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Adapter.h
- [ ] GraphicsEngine/D3D12/Context/D3D12Check.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Check.h
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandAllocator.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandAllocator.h
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandList.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandList.h
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandQueue.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12CommandQueue.h
- [ ] GraphicsEngine/D3D12/Context/D3D12Context.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Context.h
- [ ] GraphicsEngine/D3D12/Context/D3D12DebugLayer.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12DebugLayer.h
- [ ] GraphicsEngine/D3D12/Context/D3D12Device.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Device.h
- [ ] GraphicsEngine/D3D12/Context/D3D12Factory.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Factory.h
- [ ] GraphicsEngine/D3D12/Context/D3D12Fence.cpp
- [ ] GraphicsEngine/D3D12/Context/D3D12Fence.h

### GraphicsEngine/D3D12
- [ ] GraphicsEngine/D3D12/D3D12Common.h
- [ ] GraphicsEngine/D3D12/D3D12Types.h

### GraphicsEngine/D3D12/Descriptor
- [ ] GraphicsEngine/D3D12/Descriptor/BindlessHeap.cpp
- [ ] GraphicsEngine/D3D12/Descriptor/BindlessHeap.h
- [ ] GraphicsEngine/D3D12/Descriptor/DescriptorHeap.cpp
- [ ] GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h

### GraphicsEngine/D3D12
- [ ] GraphicsEngine/D3D12/FrameRing.h

### GraphicsEngine/D3D12/PipelineState
- [ ] GraphicsEngine/D3D12/PipelineState/AmplificationShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/AmplificationShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/BlendState.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/BlendState.h
- [ ] GraphicsEngine/D3D12/PipelineState/ComputeShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/ComputeShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/DepthStencilState.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/DepthStencilState.h
- [ ] GraphicsEngine/D3D12/PipelineState/DomainShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/DomainShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/GeometryShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/GeometryShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/HullShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/HullShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/MeshShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/MeshShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/PipelineStateObject.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h
- [ ] GraphicsEngine/D3D12/PipelineState/PixelShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/PixelShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/RasterizerState.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/RasterizerState.h
- [ ] GraphicsEngine/D3D12/PipelineState/RaytracingShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/RaytracingShader.h
- [ ] GraphicsEngine/D3D12/PipelineState/RaytracingStateObject.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/RaytracingStateObject.h
- [ ] GraphicsEngine/D3D12/PipelineState/RootSignature.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/RootSignature.h
- [ ] GraphicsEngine/D3D12/PipelineState/SamplerState.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/SamplerState.h
- [ ] GraphicsEngine/D3D12/PipelineState/VertexShader.cpp
- [ ] GraphicsEngine/D3D12/PipelineState/VertexShader.h

### GraphicsEngine/D3D12/SwapChain
- [ ] GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h
- [ ] GraphicsEngine/D3D12/SwapChain/SwapChain.cpp
- [ ] GraphicsEngine/D3D12/SwapChain/SwapChain.h

### GraphicsEngine/DLSS
- [ ] GraphicsEngine/DLSS/Dlss.hlsli
- [ ] GraphicsEngine/DLSS/DlssBackgroundVelocityCS.hlsl
- [ ] GraphicsEngine/DLSS/DlssBackgroundVelocityShader.cpp
- [ ] GraphicsEngine/DLSS/DlssBackgroundVelocityShader.h
- [ ] GraphicsEngine/DLSS/DlssManager.cpp
- [ ] GraphicsEngine/DLSS/DlssManager.h
- [ ] GraphicsEngine/DLSS/DlssNormalRoughnessCS.hlsl
- [ ] GraphicsEngine/DLSS/DlssNormalRoughnessShader.cpp
- [ ] GraphicsEngine/DLSS/DlssNormalRoughnessShader.h

### GraphicsEngine/Effect/Effekseer
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerCache.h
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerEffect.cpp
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerEffect.h
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerFileInterface.cpp
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerFileInterface.h
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerLoader.cpp
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerLoader.h
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerManager.cpp
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerManager.h
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerResource.cpp
- [ ] GraphicsEngine/Effect/Effekseer/EffekseerResource.h

### GraphicsEngine/Effect/Zephyr
- [ ] GraphicsEngine/Effect/Zephyr/Effect.h
- [ ] GraphicsEngine/Effect/Zephyr/EffectLoader.cpp
- [ ] GraphicsEngine/Effect/Zephyr/EffectLoader.h
- [ ] GraphicsEngine/Effect/Zephyr/EffectResource.cpp
- [ ] GraphicsEngine/Effect/Zephyr/EffectResource.h

### GraphicsEngine/Effect/Zephyr/Module
- [ ] GraphicsEngine/Effect/Zephyr/Module/DragModule.hlsli
- [ ] GraphicsEngine/Effect/Zephyr/Module/GravityModule.hlsli

### GraphicsEngine/Effect/Zephyr
- [ ] GraphicsEngine/Effect/Zephyr/Particle.hlsli
- [ ] GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli
- [ ] GraphicsEngine/Effect/Zephyr/Zephyr.h
- [ ] GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.cpp
- [ ] GraphicsEngine/Effect/Zephyr/ZephyrModuleAssembler.h
- [ ] GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.cpp
- [ ] GraphicsEngine/Effect/Zephyr/ZephyrModuleRegistry.h

### GraphicsEngine/Environment
- [ ] GraphicsEngine/Environment/Weather.h
- [ ] GraphicsEngine/Environment/Weather.hlsli
- [ ] GraphicsEngine/Environment/WeatherParticle.hlsli
- [ ] GraphicsEngine/Environment/WeatherParticleAS.hlsl
- [ ] GraphicsEngine/Environment/WeatherParticleMS.hlsl
- [ ] GraphicsEngine/Environment/WeatherParticlePS.hlsl
- [ ] GraphicsEngine/Environment/WeatherParticleShader.cpp
- [ ] GraphicsEngine/Environment/WeatherParticleShader.h
- [ ] GraphicsEngine/Environment/WeatherParticleSimulateCS.hlsl
- [ ] GraphicsEngine/Environment/WeatherParticleVS.hlsl

### GraphicsEngine/Font/Billboard
- [ ] GraphicsEngine/Font/Billboard/FontBillboardAS.hlsl
- [ ] GraphicsEngine/Font/Billboard/FontBillboardMS.hlsl
- [ ] GraphicsEngine/Font/Billboard/FontBillboardPS.hlsl
- [ ] GraphicsEngine/Font/Billboard/FontBillboardSilhouetteAS.hlsl
- [ ] GraphicsEngine/Font/Billboard/FontBillboardSilhouetteVS.hlsl
- [ ] GraphicsEngine/Font/Billboard/FontBillboardVS.hlsl

### GraphicsEngine/Font
- [ ] GraphicsEngine/Font/Font.cpp
- [ ] GraphicsEngine/Font/Font.h
- [ ] GraphicsEngine/Font/Font.hlsli
- [ ] GraphicsEngine/Font/FontLoader.cpp
- [ ] GraphicsEngine/Font/FontLoader.h
- [ ] GraphicsEngine/Font/FontManager.cpp
- [ ] GraphicsEngine/Font/FontManager.h
- [ ] GraphicsEngine/Font/FontResource.cpp
- [ ] GraphicsEngine/Font/FontResource.h
- [ ] GraphicsEngine/Font/FontShader.cpp
- [ ] GraphicsEngine/Font/FontShader.h
- [ ] GraphicsEngine/Font/FontSilhouettePS.hlsl
- [ ] GraphicsEngine/Font/ShapedText.cpp
- [ ] GraphicsEngine/Font/ShapedText.h

### GraphicsEngine/Font/Sprite
- [ ] GraphicsEngine/Font/Sprite/FontSpriteAS.hlsl
- [ ] GraphicsEngine/Font/Sprite/FontSpriteMS.hlsl
- [ ] GraphicsEngine/Font/Sprite/FontSpritePS.hlsl
- [ ] GraphicsEngine/Font/Sprite/FontSpriteSilhouetteAS.hlsl
- [ ] GraphicsEngine/Font/Sprite/FontSpriteSilhouetteVS.hlsl
- [ ] GraphicsEngine/Font/Sprite/FontSpriteVS.hlsl

### GraphicsEngine/Font
- [ ] GraphicsEngine/Font/Text.h

### GraphicsEngine
- [ ] GraphicsEngine/Graphics.cpp
- [ ] GraphicsEngine/Graphics.h

### GraphicsEngine/Light
- [ ] GraphicsEngine/Light/BidirectionalReflectanceDistributionFunction.hlsli
- [ ] GraphicsEngine/Light/Cluster.hlsli
- [ ] GraphicsEngine/Light/ClusterAssignCS.hlsl
- [ ] GraphicsEngine/Light/DirectionalLight.h
- [ ] GraphicsEngine/Light/Light.hlsli
- [ ] GraphicsEngine/Light/PointLight.h
- [ ] GraphicsEngine/Light/RectangleLight.h
- [ ] GraphicsEngine/Light/SkyLight.h
- [ ] GraphicsEngine/Light/SpotLight.h

### GraphicsEngine/Model/Animation
- [ ] GraphicsEngine/Model/Animation/Animation.cpp
- [ ] GraphicsEngine/Model/Animation/Animation.h
- [ ] GraphicsEngine/Model/Animation/AnimationLoader.cpp
- [ ] GraphicsEngine/Model/Animation/AnimationLoader.h
- [ ] GraphicsEngine/Model/Animation/AnimationResource.cpp
- [ ] GraphicsEngine/Model/Animation/AnimationResource.h
- [ ] GraphicsEngine/Model/Animation/Animator.cpp
- [ ] GraphicsEngine/Model/Animation/Animator.h
- [ ] GraphicsEngine/Model/Animation/AnimatorControllerState.h

### GraphicsEngine/Model/Cluster
- [ ] GraphicsEngine/Model/Cluster/MeshletPS.hlsl
- [ ] GraphicsEngine/Model/Cluster/QuadricErrorMetrics.cpp
- [ ] GraphicsEngine/Model/Cluster/QuadricErrorMetrics.h

### GraphicsEngine/Model/Collision
- [ ] GraphicsEngine/Model/Collision/MeshCollision.cpp
- [ ] GraphicsEngine/Model/Collision/MeshCollision.h
- [ ] GraphicsEngine/Model/Collision/MeshCollisionLoader.cpp
- [ ] GraphicsEngine/Model/Collision/MeshCollisionLoader.h
- [ ] GraphicsEngine/Model/Collision/MeshCollisionResource.cpp
- [ ] GraphicsEngine/Model/Collision/MeshCollisionResource.h

### GraphicsEngine/Model
- [ ] GraphicsEngine/Model/Crister.cpp
- [ ] GraphicsEngine/Model/Crister.h

### GraphicsEngine/Model/Culling
- [ ] GraphicsEngine/Model/Culling/FurShellCullingCS.hlsl
- [ ] GraphicsEngine/Model/Culling/GeometryBufferCullingCS.hlsl
- [ ] GraphicsEngine/Model/Culling/ModelCullingBuffer.cpp
- [ ] GraphicsEngine/Model/Culling/ModelCullingBuffer.h
- [ ] GraphicsEngine/Model/Culling/ModelCullingCS.hlsl
- [ ] GraphicsEngine/Model/Culling/ModelSilhouetteCullingCS.hlsl
- [ ] GraphicsEngine/Model/Culling/ModelTransparentCullingCS.hlsl

### GraphicsEngine/Model/Depth
- [ ] GraphicsEngine/Model/Depth/DepthPrepassMS.hlsl
- [ ] GraphicsEngine/Model/Depth/DepthPrepassPS.hlsl
- [ ] GraphicsEngine/Model/Depth/DepthPrepassVS.hlsl
- [ ] GraphicsEngine/Model/Depth/DepthResizeCS.hlsl
- [ ] GraphicsEngine/Model/Depth/HiZBufferCS.hlsl

### GraphicsEngine/Model/IK
- [ ] GraphicsEngine/Model/IK/FABRIK.cpp
- [ ] GraphicsEngine/Model/IK/FABRIK.h
- [ ] GraphicsEngine/Model/IK/FullBodyIK.cpp
- [ ] GraphicsEngine/Model/IK/FullBodyIK.h
- [ ] GraphicsEngine/Model/IK/IKPose.cpp
- [ ] GraphicsEngine/Model/IK/IKPose.h
- [ ] GraphicsEngine/Model/IK/TreeFABRIK.cpp
- [ ] GraphicsEngine/Model/IK/TreeFABRIK.h
- [ ] GraphicsEngine/Model/IK/TwoBoneIK.cpp
- [ ] GraphicsEngine/Model/IK/TwoBoneIK.h

### GraphicsEngine/Model/Material
- [ ] GraphicsEngine/Model/Material/Material.cpp
- [ ] GraphicsEngine/Model/Material/Material.h
- [ ] GraphicsEngine/Model/Material/MaterialClassifyCS.hlsl
- [ ] GraphicsEngine/Model/Material/MaterialLoader.cpp
- [ ] GraphicsEngine/Model/Material/MaterialLoader.h
- [ ] GraphicsEngine/Model/Material/MaterialPrefixSumCS.hlsl
- [ ] GraphicsEngine/Model/Material/MaterialResolveCS.hlsl
- [ ] GraphicsEngine/Model/Material/MaterialResolveShader.cpp
- [ ] GraphicsEngine/Model/Material/MaterialResolveShader.h
- [ ] GraphicsEngine/Model/Material/MaterialResource.cpp
- [ ] GraphicsEngine/Model/Material/MaterialResource.h
- [ ] GraphicsEngine/Model/Material/MaterialScatterCS.hlsl
- [ ] GraphicsEngine/Model/Material/MaterialSortBuffer.cpp
- [ ] GraphicsEngine/Model/Material/MaterialSortBuffer.h
- [ ] GraphicsEngine/Model/Material/MaterialState.h

### GraphicsEngine/Model
- [ ] GraphicsEngine/Model/Mesh.h
- [ ] GraphicsEngine/Model/Model.hlsli
- [ ] GraphicsEngine/Model/ModelCommon.cpp
- [ ] GraphicsEngine/Model/ModelExporter.cpp
- [ ] GraphicsEngine/Model/ModelExporter.h
- [ ] GraphicsEngine/Model/ModelLoader.cpp
- [ ] GraphicsEngine/Model/ModelLoader.h
- [ ] GraphicsEngine/Model/ModelPreviewPS.hlsl
- [ ] GraphicsEngine/Model/ModelRecord.h
- [ ] GraphicsEngine/Model/ModelResource.cpp
- [ ] GraphicsEngine/Model/ModelResource.h
- [ ] GraphicsEngine/Model/ModelShader.cpp
- [ ] GraphicsEngine/Model/ModelShader.h
- [ ] GraphicsEngine/Model/ModelSilhouetteAS.hlsl
- [ ] GraphicsEngine/Model/ModelSilhouettePS.hlsl

### GraphicsEngine/Model/Morph
- [ ] GraphicsEngine/Model/Morph/MorphBlendCS.hlsl
- [ ] GraphicsEngine/Model/Morph/MorphBlendShader.cpp
- [ ] GraphicsEngine/Model/Morph/MorphBlendShader.h

### GraphicsEngine/Model/Opaque
- [ ] GraphicsEngine/Model/Opaque/DeferredLightingMS.hlsl
- [ ] GraphicsEngine/Model/Opaque/DeferredLightingPS.hlsl
- [ ] GraphicsEngine/Model/Opaque/FurShading.hlsli
- [ ] GraphicsEngine/Model/Opaque/FurShellAS.hlsl
- [ ] GraphicsEngine/Model/Opaque/FurShellMS.hlsl
- [ ] GraphicsEngine/Model/Opaque/FurShellPS.hlsl
- [ ] GraphicsEngine/Model/Opaque/FurShellVS.hlsl
- [ ] GraphicsEngine/Model/Opaque/GeometryBuffer.hlsli
- [ ] GraphicsEngine/Model/Opaque/GeometryBufferAS.hlsl
- [ ] GraphicsEngine/Model/Opaque/ModelAS.hlsl
- [ ] GraphicsEngine/Model/Opaque/PbrShading.hlsli
- [ ] GraphicsEngine/Model/Opaque/PhongShading.hlsli
- [ ] GraphicsEngine/Model/Opaque/SkeletalModelMS.hlsl
- [ ] GraphicsEngine/Model/Opaque/SkeletalModelPS.hlsl
- [ ] GraphicsEngine/Model/Opaque/SkeletalModelVS.hlsl
- [ ] GraphicsEngine/Model/Opaque/StaticModelMS.hlsl
- [ ] GraphicsEngine/Model/Opaque/StaticModelPS.hlsl
- [ ] GraphicsEngine/Model/Opaque/StaticModelVS.hlsl
- [ ] GraphicsEngine/Model/Opaque/ToonShading.hlsli

### GraphicsEngine/Model/Skeleton
- [ ] GraphicsEngine/Model/Skeleton/Skeleton.cpp
- [ ] GraphicsEngine/Model/Skeleton/Skeleton.h
- [ ] GraphicsEngine/Model/Skeleton/SkeletonLoader.cpp
- [ ] GraphicsEngine/Model/Skeleton/SkeletonLoader.h
- [ ] GraphicsEngine/Model/Skeleton/SkeletonResource.cpp
- [ ] GraphicsEngine/Model/Skeleton/SkeletonResource.h
- [ ] GraphicsEngine/Model/Skeleton/SkeletonState.h

### GraphicsEngine/Model/Skin
- [ ] GraphicsEngine/Model/Skin/SkinBlendCS.hlsl
- [ ] GraphicsEngine/Model/Skin/SkinBlendShader.cpp
- [ ] GraphicsEngine/Model/Skin/SkinBlendShader.h

### GraphicsEngine/Model
- [ ] GraphicsEngine/Model/SoftbodyMesh.cpp
- [ ] GraphicsEngine/Model/SoftbodyMesh.h

### GraphicsEngine/Model/Transparent
- [ ] GraphicsEngine/Model/Transparent/ModelTransparentAS.hlsl
- [ ] GraphicsEngine/Model/Transparent/ModelTransparentPS.hlsl
- [ ] GraphicsEngine/Model/Transparent/OITBuffer.cpp
- [ ] GraphicsEngine/Model/Transparent/OITBuffer.h
- [ ] GraphicsEngine/Model/Transparent/OITResolveMS.hlsl
- [ ] GraphicsEngine/Model/Transparent/OITResolvePS.hlsl
- [ ] GraphicsEngine/Model/Transparent/OitShading.hlsli

### GraphicsEngine/Model
- [ ] GraphicsEngine/Model/WireframePS.hlsl

### GraphicsEngine/Movie/Billboard
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardAS.hlsl
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardMS.hlsl
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardPS.hlsl
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardSilhouetteAS.hlsl
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardSilhouetteVS.hlsl
- [ ] GraphicsEngine/Movie/Billboard/MovieBillboardVS.hlsl

### GraphicsEngine/Movie/Fullscreen
- [ ] GraphicsEngine/Movie/Fullscreen/MovieFullscreenAS.hlsl
- [ ] GraphicsEngine/Movie/Fullscreen/MovieFullscreenMS.hlsl
- [ ] GraphicsEngine/Movie/Fullscreen/MovieFullscreenPS.hlsl
- [ ] GraphicsEngine/Movie/Fullscreen/MovieFullscreenVS.hlsl

### GraphicsEngine/Movie
- [ ] GraphicsEngine/Movie/Movie.h
- [ ] GraphicsEngine/Movie/Movie.hlsli
- [ ] GraphicsEngine/Movie/MovieByteStream.cpp
- [ ] GraphicsEngine/Movie/MovieByteStream.h
- [ ] GraphicsEngine/Movie/MovieDecoder.cpp
- [ ] GraphicsEngine/Movie/MovieDecoder.h
- [ ] GraphicsEngine/Movie/MovieLoader.cpp
- [ ] GraphicsEngine/Movie/MovieLoader.h
- [ ] GraphicsEngine/Movie/MovieResource.cpp
- [ ] GraphicsEngine/Movie/MovieResource.h
- [ ] GraphicsEngine/Movie/MovieShader.cpp
- [ ] GraphicsEngine/Movie/MovieShader.h
- [ ] GraphicsEngine/Movie/MovieSilhouettePS.hlsl

### GraphicsEngine/Movie/Sprite
- [ ] GraphicsEngine/Movie/Sprite/MovieSpriteAS.hlsl
- [ ] GraphicsEngine/Movie/Sprite/MovieSpriteMS.hlsl
- [ ] GraphicsEngine/Movie/Sprite/MovieSpritePS.hlsl
- [ ] GraphicsEngine/Movie/Sprite/MovieSpriteSilhouetteAS.hlsl
- [ ] GraphicsEngine/Movie/Sprite/MovieSpriteSilhouetteVS.hlsl
- [ ] GraphicsEngine/Movie/Sprite/MovieSpriteVS.hlsl

### GraphicsEngine/Movie
- [ ] GraphicsEngine/Movie/Video.cpp
- [ ] GraphicsEngine/Movie/Video.h

### GraphicsEngine/PostProcess
- [ ] GraphicsEngine/PostProcess/AnamorphicFlareCS.hlsl
- [ ] GraphicsEngine/PostProcess/AutoExposureAverageCS.hlsl
- [ ] GraphicsEngine/PostProcess/AutoExposureHistogramCS.hlsl
- [ ] GraphicsEngine/PostProcess/BokehCS.hlsl
- [ ] GraphicsEngine/PostProcess/ChromaticAberrationCS.hlsl
- [ ] GraphicsEngine/PostProcess/ColorGradingCS.hlsl
- [ ] GraphicsEngine/PostProcess/DepthOfFieldCS.hlsl
- [ ] GraphicsEngine/PostProcess/FilmGrainCS.hlsl
- [ ] GraphicsEngine/PostProcess/KawaseBloomCS.hlsl
- [ ] GraphicsEngine/PostProcess/LensDistortionCS.hlsl
- [ ] GraphicsEngine/PostProcess/LensFlareCS.hlsl

### GraphicsEngine/PostProcess/PostEffect
- [ ] GraphicsEngine/PostProcess/PostEffect/AnamorphicFlare.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/AnamorphicFlare.h
- [ ] GraphicsEngine/PostProcess/PostEffect/AutoExposure.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/AutoExposure.h
- [ ] GraphicsEngine/PostProcess/PostEffect/Bokeh.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/Bokeh.h
- [ ] GraphicsEngine/PostProcess/PostEffect/ChromaticAberration.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/ChromaticAberration.h
- [ ] GraphicsEngine/PostProcess/PostEffect/ColorGrading.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/ColorGrading.h
- [ ] GraphicsEngine/PostProcess/PostEffect/DepthOfField.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/DepthOfField.h
- [ ] GraphicsEngine/PostProcess/PostEffect/DirectionalBlur.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/DirectionalBlur.h
- [ ] GraphicsEngine/PostProcess/PostEffect/FilmGrain.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/FilmGrain.h
- [ ] GraphicsEngine/PostProcess/PostEffect/KawaseBloom.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/KawaseBloom.h
- [ ] GraphicsEngine/PostProcess/PostEffect/LensDistortion.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/LensDistortion.h
- [ ] GraphicsEngine/PostProcess/PostEffect/LensFlare.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/LensFlare.h
- [ ] GraphicsEngine/PostProcess/PostEffect/MotionBlur.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/MotionBlur.h
- [ ] GraphicsEngine/PostProcess/PostEffect/RadialBlur.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/RadialBlur.h
- [ ] GraphicsEngine/PostProcess/PostEffect/Sharpness.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/Sharpness.h
- [ ] GraphicsEngine/PostProcess/PostEffect/ToneMapping.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/ToneMapping.h
- [ ] GraphicsEngine/PostProcess/PostEffect/Vignette.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/Vignette.h
- [ ] GraphicsEngine/PostProcess/PostEffect/ZoomBlur.cpp
- [ ] GraphicsEngine/PostProcess/PostEffect/ZoomBlur.h

### GraphicsEngine/PostProcess
- [ ] GraphicsEngine/PostProcess/PostProcess.h
- [ ] GraphicsEngine/PostProcess/PostProcess.hlsli
- [ ] GraphicsEngine/PostProcess/SharpnessCS.hlsl
- [ ] GraphicsEngine/PostProcess/ToneMappingCS.hlsl
- [ ] GraphicsEngine/PostProcess/ToneMappingCurves.hlsli
- [ ] GraphicsEngine/PostProcess/VignetteCS.hlsl

### GraphicsEngine
- [ ] GraphicsEngine/Prelude.cpp

### GraphicsEngine/Profiler
- [ ] GraphicsEngine/Profiler/GpuProfiler.cpp
- [ ] GraphicsEngine/Profiler/GpuProfiler.h
- [ ] GraphicsEngine/Profiler/ProfilerStats.h

### GraphicsEngine/Quality
- [ ] GraphicsEngine/Quality/GraphicsQuality.cpp
- [ ] GraphicsEngine/Quality/GraphicsQuality.h
- [ ] GraphicsEngine/Quality/Upscale.h

### GraphicsEngine/Rasterization
- [ ] GraphicsEngine/Rasterization/RasterizationContext.h

### GraphicsEngine/Raytracing/AmbientOcclusion
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusion.hlsli
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionDenoiseCS.hlsl
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionDenoiseShader.cpp
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionDenoiseShader.h
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionRT.hlsl
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionShader.cpp
- [ ] GraphicsEngine/Raytracing/AmbientOcclusion/AmbientOcclusionShader.h

### GraphicsEngine/Raytracing
- [ ] GraphicsEngine/Raytracing/BottomLevelAccelerationStructure.cpp
- [ ] GraphicsEngine/Raytracing/BottomLevelAccelerationStructure.h

### GraphicsEngine/Raytracing/Fog
- [ ] GraphicsEngine/Raytracing/Fog/FogInjectionCS.hlsl

### GraphicsEngine/Raytracing/Froxel
- [ ] GraphicsEngine/Raytracing/Froxel/Froxel.hlsli
- [ ] GraphicsEngine/Raytracing/Froxel/FroxelIntegrationCS.hlsl

### GraphicsEngine/Raytracing/GlobalIllumination
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIllumination.hlsli
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationDenoiseCS.hlsl
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationDenoiseShader.cpp
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationDenoiseShader.h
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationReservoirSpatialCS.hlsl
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationReSTIR.hlsli
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationRT.hlsl
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationShader.cpp
- [ ] GraphicsEngine/Raytracing/GlobalIllumination/GlobalIlluminationShader.h

### GraphicsEngine/Raytracing
- [ ] GraphicsEngine/Raytracing/Raytracing.hlsli
- [ ] GraphicsEngine/Raytracing/RaytracingContext.h
- [ ] GraphicsEngine/Raytracing/RaytracingView.h

### GraphicsEngine/Raytracing/Reflection
- [ ] GraphicsEngine/Raytracing/Reflection/Reflection.hlsli
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionDenoiseCS.hlsl
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionDenoiseShader.cpp
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionDenoiseShader.h
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionReservoirSpatialCS.hlsl
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionReSTIR.hlsli
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionRT.hlsl
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionShader.cpp
- [ ] GraphicsEngine/Raytracing/Reflection/ReflectionShader.h

### GraphicsEngine/Raytracing/Refraction
- [ ] GraphicsEngine/Raytracing/Refraction/Refraction.hlsli
- [ ] GraphicsEngine/Raytracing/Refraction/RefractionRT.hlsl
- [ ] GraphicsEngine/Raytracing/Refraction/RefractionShader.cpp
- [ ] GraphicsEngine/Raytracing/Refraction/RefractionShader.h

### GraphicsEngine/Raytracing/Shadow
- [ ] GraphicsEngine/Raytracing/Shadow/Shadow.hlsli
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowDenoiseCS.hlsl
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowDenoiseShader.cpp
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowDenoiseShader.h
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowRT.hlsl
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowShader.cpp
- [ ] GraphicsEngine/Raytracing/Shadow/ShadowShader.h

### GraphicsEngine/Raytracing/SubsurfaceScattering
- [ ] GraphicsEngine/Raytracing/SubsurfaceScattering/SubsurfaceScattering.hlsli
- [ ] GraphicsEngine/Raytracing/SubsurfaceScattering/SubsurfaceScatteringRT.hlsl
- [ ] GraphicsEngine/Raytracing/SubsurfaceScattering/SubsurfaceScatteringShader.cpp
- [ ] GraphicsEngine/Raytracing/SubsurfaceScattering/SubsurfaceScatteringShader.h

### GraphicsEngine/Raytracing
- [ ] GraphicsEngine/Raytracing/TopLevelAccelerationStructure.cpp
- [ ] GraphicsEngine/Raytracing/TopLevelAccelerationStructure.h

### GraphicsEngine/Raytracing/VolumetricCloudScapes
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/CloudNoiseBake.hlsli
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/CloudNoiseDetailBakeCS.hlsl
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/CloudNoiseShapeBakeCS.hlsl
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/VolumetricCloudScapes.hlsli
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/VolumetricCloudScapesRT.hlsl
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/VolumetricCloudScapesShader.cpp
- [ ] GraphicsEngine/Raytracing/VolumetricCloudScapes/VolumetricCloudScapesShader.h

### GraphicsEngine/Raytracing/VolumetricLight
- [ ] GraphicsEngine/Raytracing/VolumetricLight/VolumetricLight.hlsli
- [ ] GraphicsEngine/Raytracing/VolumetricLight/VolumetricLightScatteringRT.hlsl
- [ ] GraphicsEngine/Raytracing/VolumetricLight/VolumetricLightShader.cpp
- [ ] GraphicsEngine/Raytracing/VolumetricLight/VolumetricLightShader.h

### GraphicsEngine/Raytracing/VolumetricStar
- [ ] GraphicsEngine/Raytracing/VolumetricStar/VolumetricStar.hlsli
- [ ] GraphicsEngine/Raytracing/VolumetricStar/VolumetricStarRT.hlsl
- [ ] GraphicsEngine/Raytracing/VolumetricStar/VolumetricStarShader.cpp
- [ ] GraphicsEngine/Raytracing/VolumetricStar/VolumetricStarShader.h

### GraphicsEngine/Renderer
- [ ] GraphicsEngine/Renderer/AmbientOcclusionRenderer.cpp
- [ ] GraphicsEngine/Renderer/AmbientOcclusionRenderer.h
- [ ] GraphicsEngine/Renderer/AvatarRenderer.cpp
- [ ] GraphicsEngine/Renderer/AvatarRenderer.h
- [ ] GraphicsEngine/Renderer/BootScreenRenderer.cpp
- [ ] GraphicsEngine/Renderer/BootScreenRenderer.h
- [ ] GraphicsEngine/Renderer/ColliderRenderer.cpp
- [ ] GraphicsEngine/Renderer/ColliderRenderer.h
- [ ] GraphicsEngine/Renderer/DecalRenderer.cpp
- [ ] GraphicsEngine/Renderer/DecalRenderer.h
- [ ] GraphicsEngine/Renderer/DlssRayReconstructionRenderer.cpp
- [ ] GraphicsEngine/Renderer/DlssRayReconstructionRenderer.h
- [ ] GraphicsEngine/Renderer/EffekseerRenderer.cpp
- [ ] GraphicsEngine/Renderer/EffekseerRenderer.h
- [ ] GraphicsEngine/Renderer/FontRenderer.cpp
- [ ] GraphicsEngine/Renderer/FontRenderer.h
- [ ] GraphicsEngine/Renderer/GlobalIlluminationRenderer.cpp
- [ ] GraphicsEngine/Renderer/GlobalIlluminationRenderer.h
- [ ] GraphicsEngine/Renderer/HUDComposeRenderer.cpp
- [ ] GraphicsEngine/Renderer/HUDComposeRenderer.h
- [ ] GraphicsEngine/Renderer/MaterialRenderer.cpp
- [ ] GraphicsEngine/Renderer/MaterialRenderer.h
- [ ] GraphicsEngine/Renderer/ModelRenderer.cpp
- [ ] GraphicsEngine/Renderer/ModelRenderer.h
- [ ] GraphicsEngine/Renderer/ModelTransformRenderer.cpp
- [ ] GraphicsEngine/Renderer/ModelTransformRenderer.h
- [ ] GraphicsEngine/Renderer/MovieRenderer.cpp
- [ ] GraphicsEngine/Renderer/MovieRenderer.h
- [ ] GraphicsEngine/Renderer/OutlineRenderer.cpp
- [ ] GraphicsEngine/Renderer/OutlineRenderer.h
- [ ] GraphicsEngine/Renderer/PostProcessRenderer.cpp
- [ ] GraphicsEngine/Renderer/PostProcessRenderer.h
- [ ] GraphicsEngine/Renderer/RaytracingRenderer.cpp
- [ ] GraphicsEngine/Renderer/RaytracingRenderer.h
- [ ] GraphicsEngine/Renderer/ReflectionRenderer.cpp
- [ ] GraphicsEngine/Renderer/ReflectionRenderer.h
- [ ] GraphicsEngine/Renderer/RefractionRenderer.cpp
- [ ] GraphicsEngine/Renderer/RefractionRenderer.h
- [ ] GraphicsEngine/Renderer/Renderer.cpp
- [ ] GraphicsEngine/Renderer/Renderer.h
- [ ] GraphicsEngine/Renderer/ShadowRenderer.cpp
- [ ] GraphicsEngine/Renderer/ShadowRenderer.h
- [ ] GraphicsEngine/Renderer/ShapeRenderer.cpp
- [ ] GraphicsEngine/Renderer/ShapeRenderer.h
- [ ] GraphicsEngine/Renderer/SkeletonControllerRenderer.cpp
- [ ] GraphicsEngine/Renderer/SkeletonControllerRenderer.h
- [ ] GraphicsEngine/Renderer/SkyRenderer.cpp
- [ ] GraphicsEngine/Renderer/SkyRenderer.h
- [ ] GraphicsEngine/Renderer/SubsurfaceScatteringRenderer.cpp
- [ ] GraphicsEngine/Renderer/SubsurfaceScatteringRenderer.h
- [ ] GraphicsEngine/Renderer/TaauUpsamplingRenderer.cpp
- [ ] GraphicsEngine/Renderer/TaauUpsamplingRenderer.h
- [ ] GraphicsEngine/Renderer/TextureRenderer.cpp
- [ ] GraphicsEngine/Renderer/TextureRenderer.h
- [ ] GraphicsEngine/Renderer/TimelineRenderer.cpp
- [ ] GraphicsEngine/Renderer/TimelineRenderer.h
- [ ] GraphicsEngine/Renderer/ViewMode.h
- [ ] GraphicsEngine/Renderer/VolumetricCloudScapesRenderer.cpp
- [ ] GraphicsEngine/Renderer/VolumetricCloudScapesRenderer.h
- [ ] GraphicsEngine/Renderer/VolumetricLightRenderer.cpp
- [ ] GraphicsEngine/Renderer/VolumetricLightRenderer.h
- [ ] GraphicsEngine/Renderer/VolumetricStarRenderer.cpp
- [ ] GraphicsEngine/Renderer/VolumetricStarRenderer.h
- [ ] GraphicsEngine/Renderer/WeatherParticleRenderer.cpp
- [ ] GraphicsEngine/Renderer/WeatherParticleRenderer.h

### GraphicsEngine/ScreenSpace
- [ ] GraphicsEngine/ScreenSpace/ScreenSpaceContext.h

### GraphicsEngine/Shader
- [ ] GraphicsEngine/Shader/Constants.hlsli
- [ ] GraphicsEngine/Shader/Culling.hlsli
- [ ] GraphicsEngine/Shader/Denoiser.hlsli
- [ ] GraphicsEngine/Shader/Dispatch.hlsli
- [ ] GraphicsEngine/Shader/Material.hlsli
- [ ] GraphicsEngine/Shader/Noise.hlsli
- [ ] GraphicsEngine/Shader/Normal.hlsli
- [ ] GraphicsEngine/Shader/Precipitation.hlsli
- [ ] GraphicsEngine/Shader/Sampler.hlsli
- [ ] GraphicsEngine/Shader/Scene.hlsli
- [ ] GraphicsEngine/Shader/ShaderCache.cpp
- [ ] GraphicsEngine/Shader/ShaderCache.h
- [ ] GraphicsEngine/Shader/ShaderCompiler.cpp
- [ ] GraphicsEngine/Shader/ShaderCompiler.h
- [ ] GraphicsEngine/Shader/ShaderHotReload.cpp
- [ ] GraphicsEngine/Shader/ShaderHotReload.h
- [ ] GraphicsEngine/Shader/ShaderResources.hlsli
- [ ] GraphicsEngine/Shader/UnorderedAccesses.hlsli
- [ ] GraphicsEngine/Shader/Vertex.hlsli

### GraphicsEngine/Shape/Collider
- [ ] GraphicsEngine/Shape/Collider/ColliderLine.hlsli
- [ ] GraphicsEngine/Shape/Collider/ColliderLineMS.hlsl
- [ ] GraphicsEngine/Shape/Collider/ColliderLinePS.hlsl
- [ ] GraphicsEngine/Shape/Collider/ColliderLineShader.cpp
- [ ] GraphicsEngine/Shape/Collider/ColliderLineShader.h
- [ ] GraphicsEngine/Shape/Collider/ColliderLineVS.hlsl

### GraphicsEngine/Shape/HUD
- [ ] GraphicsEngine/Shape/HUD/FullscreenVS.hlsl
- [ ] GraphicsEngine/Shape/HUD/HUD.hlsli
- [ ] GraphicsEngine/Shape/HUD/HUDComposeMS.hlsl
- [ ] GraphicsEngine/Shape/HUD/HUDComposePS.hlsl
- [ ] GraphicsEngine/Shape/HUD/HUDComposeShader.cpp
- [ ] GraphicsEngine/Shape/HUD/HUDComposeShader.h

### GraphicsEngine/Shape/Outline
- [ ] GraphicsEngine/Shape/Outline/OutlineMS.hlsl
- [ ] GraphicsEngine/Shape/Outline/OutlinePS.hlsl
- [ ] GraphicsEngine/Shape/Outline/OutlineShader.cpp
- [ ] GraphicsEngine/Shape/Outline/OutlineShader.h

### GraphicsEngine/Shape/Screen
- [ ] GraphicsEngine/Shape/Screen/BootScreen.cpp
- [ ] GraphicsEngine/Shape/Screen/BootScreen.h
- [ ] GraphicsEngine/Shape/Screen/BootScreenPS.hlsl
- [ ] GraphicsEngine/Shape/Screen/FadeScreen.cpp
- [ ] GraphicsEngine/Shape/Screen/FadeScreen.h
- [ ] GraphicsEngine/Shape/Screen/FadeScreenPS.hlsl
- [ ] GraphicsEngine/Shape/Screen/LetterScreen.cpp
- [ ] GraphicsEngine/Shape/Screen/LetterScreen.h
- [ ] GraphicsEngine/Shape/Screen/LetterScreenPS.hlsl
- [ ] GraphicsEngine/Shape/Screen/SplashScreen.cpp
- [ ] GraphicsEngine/Shape/Screen/SplashScreen.h
- [ ] GraphicsEngine/Shape/Screen/SplashScreenPS.hlsl
- [ ] GraphicsEngine/Shape/Screen/SplashScreenVS.hlsl

### GraphicsEngine/Sky/Cubemap
- [ ] GraphicsEngine/Sky/Cubemap/EquirectCubemapCS.hlsl
- [ ] GraphicsEngine/Sky/Cubemap/ProceduralCubemapCS.hlsl

### GraphicsEngine/Sky/IBL
- [ ] GraphicsEngine/Sky/IBL/BrdfLutCS.hlsl
- [ ] GraphicsEngine/Sky/IBL/DiffuseIrradianceCS.hlsl
- [ ] GraphicsEngine/Sky/IBL/ImageBasedLighting.hlsli
- [ ] GraphicsEngine/Sky/IBL/SpecularPrefilterCS.hlsl

### GraphicsEngine/Sky
- [ ] GraphicsEngine/Sky/Sky.hlsli
- [ ] GraphicsEngine/Sky/SkyGenerate.hlsli
- [ ] GraphicsEngine/Sky/Skymap.cpp
- [ ] GraphicsEngine/Sky/Skymap.h
- [ ] GraphicsEngine/Sky/SkymapCache.cpp
- [ ] GraphicsEngine/Sky/SkymapCache.h
- [ ] GraphicsEngine/Sky/SkymapLoader.cpp
- [ ] GraphicsEngine/Sky/SkymapLoader.h
- [ ] GraphicsEngine/Sky/SkymapResource.cpp
- [ ] GraphicsEngine/Sky/SkymapResource.h

### GraphicsEngine/System
- [ ] GraphicsEngine/System/AnimationSystem.cpp
- [ ] GraphicsEngine/System/AnimationSystem.h
- [ ] GraphicsEngine/System/CameraSystem.cpp
- [ ] GraphicsEngine/System/CameraSystem.h
- [ ] GraphicsEngine/System/CelestialSystem.cpp
- [ ] GraphicsEngine/System/CelestialSystem.h
- [ ] GraphicsEngine/System/ConstraintSystem.cpp
- [ ] GraphicsEngine/System/ConstraintSystem.h
- [ ] GraphicsEngine/System/IndicesSystem.cpp
- [ ] GraphicsEngine/System/IndicesSystem.h
- [ ] GraphicsEngine/System/LightSystem.cpp
- [ ] GraphicsEngine/System/LightSystem.h
- [ ] GraphicsEngine/System/MovieSystem.cpp
- [ ] GraphicsEngine/System/MovieSystem.h
- [ ] GraphicsEngine/System/SceneSystem.cpp
- [ ] GraphicsEngine/System/SceneSystem.h
- [ ] GraphicsEngine/System/WeatherSystem.cpp
- [ ] GraphicsEngine/System/WeatherSystem.h

### GraphicsEngine/TAAU
- [ ] GraphicsEngine/TAAU/TaauResolveCS.hlsl
- [ ] GraphicsEngine/TAAU/TaauResolveShader.cpp
- [ ] GraphicsEngine/TAAU/TaauResolveShader.h

### GraphicsEngine/Texture/Billboard
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardAS.hlsl
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardMS.hlsl
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardPS.hlsl
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardSilhouetteAS.hlsl
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardSilhouetteVS.hlsl
- [ ] GraphicsEngine/Texture/Billboard/TextureBillboardVS.hlsl

### GraphicsEngine/Texture/Compression
- [ ] GraphicsEngine/Texture/Compression/BC7CompressCS.hlsl
- [ ] GraphicsEngine/Texture/Compression/BC7CompressShader.cpp
- [ ] GraphicsEngine/Texture/Compression/BC7CompressShader.h

### GraphicsEngine/Texture
- [ ] GraphicsEngine/Texture/Image.h

### GraphicsEngine/Texture/Sprite
- [ ] GraphicsEngine/Texture/Sprite/TextureSpriteAS.hlsl
- [ ] GraphicsEngine/Texture/Sprite/TextureSpriteMS.hlsl
- [ ] GraphicsEngine/Texture/Sprite/TextureSpritePS.hlsl
- [ ] GraphicsEngine/Texture/Sprite/TextureSpriteSilhouetteAS.hlsl
- [ ] GraphicsEngine/Texture/Sprite/TextureSpriteSilhouetteVS.hlsl
- [ ] GraphicsEngine/Texture/Sprite/TextureSpriteVS.hlsl

### GraphicsEngine/Texture
- [ ] GraphicsEngine/Texture/Texture.cpp
- [ ] GraphicsEngine/Texture/Texture.h
- [ ] GraphicsEngine/Texture/Texture.hlsli
- [ ] GraphicsEngine/Texture/TextureLoader.cpp
- [ ] GraphicsEngine/Texture/TextureLoader.h
- [ ] GraphicsEngine/Texture/TextureResource.cpp
- [ ] GraphicsEngine/Texture/TextureResource.h
- [ ] GraphicsEngine/Texture/TextureShader.cpp
- [ ] GraphicsEngine/Texture/TextureShader.h
- [ ] GraphicsEngine/Texture/TextureSilhouettePS.hlsl

## PhysicsEngine (46/47)

### PhysicsEngine/CharacterController
- [x] PhysicsEngine/CharacterController/CharacterController.cpp — 日英コメント付け、MoveDirection/ForwardDirection を setter/getter オーバーロード化、GroundNormal/WallNormal/CeilingNormal の Get を削除
- [x] PhysicsEngine/CharacterController/CharacterController.h — 日英コメント付け、MoveDirection/ForwardDirection を setter/getter オーバーロード化、GroundNormal/WallNormal/CeilingNormal の Get を削除

### PhysicsEngine/Collider
- [x] PhysicsEngine/Collider/BoxCollider.cpp — 日英コメント付け、宣言順に定義を整理
- [x] PhysicsEngine/Collider/BoxCollider.h — 日英コメント付け
- [x] PhysicsEngine/Collider/CapsuleCollider.cpp — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/CapsuleCollider.h — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/CircleCollider.cpp — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/CircleCollider.h — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/CylinderCollider.cpp — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/CylinderCollider.h — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/MeshCollider.cpp — 日英コメント付け、MeshCollision の完全型 include を実装側へ移動、Rigidbody のボディ生成前は Build を保留して再試行
- [x] PhysicsEngine/Collider/MeshCollider.h — 日英コメント付け、MeshCollision を前方宣言化
- [x] PhysicsEngine/Collider/RectCollider.cpp — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/RectCollider.h — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/SphereCollider.cpp — 日英コメント付け、BoxCollider.h と同じ順に整理
- [x] PhysicsEngine/Collider/SphereCollider.h — 日英コメント付け、BoxCollider.h と同じ順に整理

### PhysicsEngine/Joint
- [x] PhysicsEngine/Joint/FixedJoint.cpp — 日英コメント付け
- [x] PhysicsEngine/Joint/FixedJoint.h — 日英コメント付け
- [x] PhysicsEngine/Joint/HingeJoint.cpp — 日英コメント付け
- [x] PhysicsEngine/Joint/HingeJoint.h — 日英コメント付け
- [x] PhysicsEngine/Joint/SliderJoint.cpp — 日英コメント付け
- [x] PhysicsEngine/Joint/SliderJoint.h — 日英コメント付け
- [x] PhysicsEngine/Joint/SpringJoint.cpp — 日英コメント付け
- [x] PhysicsEngine/Joint/SpringJoint.h — 日英コメント付け

### PhysicsEngine/JoltPhysics
- [x] PhysicsEngine/JoltPhysics/JoltCharacterContactListener.cpp — 日英コメント付け、センサー判定を検索結果から直接分岐
- [x] PhysicsEngine/JoltPhysics/JoltCharacterContactListener.h — 日英コメント付け
- [x] PhysicsEngine/JoltPhysics/JoltConstraintPool.cpp — 日英コメント付け、ConstraintHandle を Handle<JPH::Constraint> の直接表記へ変更
- [x] PhysicsEngine/JoltPhysics/JoltConstraintPool.h — 日英コメント付け、ConstraintHandle の型別名を廃止
- [x] PhysicsEngine/JoltPhysics/JoltContactListener.cpp — 日英コメント付け、DispatchPendingEvents → DispatchEvent、ActiveWorld setter/getter をオーバーロード化、QueueEvent を削除して各コールバックへ展開
- [x] PhysicsEngine/JoltPhysics/JoltContactListener.h — 日英コメント付け、DispatchPendingEvents → DispatchEvent、ActiveWorld setter/getter をオーバーロード化、QueueEvent を削除
- [x] PhysicsEngine/JoltPhysics/JoltExecutorBridge.cpp — 日英コメント付け（GetMaxConcurrency は Jolt 純粋仮想関数の override のため名前を維持）
- [x] PhysicsEngine/JoltPhysics/JoltExecutorBridge.h — 日英コメント付け（GetMaxConcurrency は Jolt 純粋仮想関数の override のため名前を維持）
- [x] PhysicsEngine/JoltPhysics/JoltLayerdef.cpp — 既存コメントを維持して不足分を追加、ヘッダー内実装を移動、定義を名前空間・クラス単位に分割
- [x] PhysicsEngine/JoltPhysics/JoltLayerdef.h — 既存コメントを維持して不足分を追加、関数実装を cpp へ移動
- [x] PhysicsEngine/JoltPhysics/JoltManager.cpp — 日英コメント付け、参照Getterの Get を削除、ActiveWorld setter/getter をオーバーロード化
- [x] PhysicsEngine/JoltPhysics/JoltManager.h — 日英コメント付け、参照Getterの Get を削除、ActiveWorld setter/getter をオーバーロード化
- [x] PhysicsEngine/JoltPhysics/JoltShapePool.cpp
- [x] PhysicsEngine/JoltPhysics/JoltShapePool.h

### PhysicsEngine/Physics
- [x] PhysicsEngine/Physics/Physics.cpp — 関数本体に日英の /// コメントを追加、RefleshJoint の説明を実装（ボディの参加状態で拘束を有効/無効化）に合わせて修正、BodyShape で動的ボディの質量を保持（慣性のみ新形状から再計算）
- [x] PhysicsEngine/Physics/Physics.h — RefleshJoint・BodyShape の説明を実装に合わせて修正
- [x] PhysicsEngine/Physics/PhysicsSystem.cpp — 日英コメント付け、FindColliderShape/ToAllowedDOFs を Rigidbody::OnAwake へラムダでべた書き、ApplyActorTransform → ApplyTransform、ApplyActive を ApplyTransform の後ろへ、GatherColliderInstances を Renderer::GatherColliders へ移管、Resolve〜 を単数形に、DispatchToActor を各 Dispatch〜 へべた書き
- [x] PhysicsEngine/Physics/PhysicsSystem.h — 日英コメント付け、同上の宣言側の変更

### PhysicsEngine
- [ ] PhysicsEngine/Prelude.cpp

### PhysicsEngine/Rigidbody
- [x] PhysicsEngine/Rigidbody/Rigidbody.cpp — 日英コメント付け、GetBodyID → BodyID、pixelsPerMeter をヘッダーの static メンバーへ
- [x] PhysicsEngine/Rigidbody/Rigidbody.h — 日英コメント付け、GetBodyID → BodyID、pixelsPerMeter_ / defaultShapeRadius_ を private static に、JoltShapePool の include を Handle.h へ

### PhysicsEngine/Softbody
- [x] PhysicsEngine/Softbody/Softbody.cpp — 日英コメント付け、HasBody → BodyID、GetVertexPositions → VertexPositionList、Rigidbody と同じ順に整理
- [x] PhysicsEngine/Softbody/Softbody.h — 日英コメント付け、同上、Crister を前方宣言化（include は cpp のみ）、経緯コメントを設計の説明に書き直し

## AIEngine (0/6)

### AIEngine/CharacterAI/BehaviorTree
- [ ] AIEngine/CharacterAI/BehaviorTree/BehaviorTreeData.h
- [ ] AIEngine/CharacterAI/BehaviorTree/BehaviorTreeGraph.h
- [ ] AIEngine/CharacterAI/BehaviorTree/BehaviorTreeNode.cpp
- [ ] AIEngine/CharacterAI/BehaviorTree/BehaviorTreeNode.h

### AIEngine
- [ ] AIEngine/Prelude.cpp

### AIEngine/SpatialAI/NavigationAI
- [ ] AIEngine/SpatialAI/NavigationAI/NavMesh.h

## AudioEngine (21/22)

### AudioEngine/Audio
- [x] AudioEngine/Audio/Audio.cpp — 日英コメント付け、UpdateListener と SetAttenuationDistance の位置入れ替え、IsPlaying/IsPaused → Playing/Paused
- [x] AudioEngine/Audio/Audio.h — 日英コメント付け、UpdateListener と SetAttenuationDistance の位置入れ替え、IsPlaying/IsPaused → Playing/Paused
- [x] AudioEngine/Audio/AudioByteStream.cpp — 日英コメント付け、GetFileSize/GetReadSize → FileSize/ReadSize、IsReadComplete → Complete
- [x] AudioEngine/Audio/AudioByteStream.h — 日英コメント付け、GetFileSize/GetReadSize → FileSize/ReadSize、IsReadComplete → Complete
- [x] AudioEngine/Audio/AudioListener.cpp — 日英コメント付け
- [x] AudioEngine/Audio/AudioListener.h — 日英コメント付け
- [x] AudioEngine/Audio/AudioLoader.cpp — 日英コメント付け
- [x] AudioEngine/Audio/AudioLoader.h — 日英コメント付け
- [x] AudioEngine/Audio/AudioResource.cpp — 日英コメント付け
- [x] AudioEngine/Audio/AudioResource.h — 日英コメント付け
- [x] AudioEngine/Audio/AudioSource.cpp — 日英コメント付け
- [x] AudioEngine/Audio/AudioSource.h — 日英コメント付け
- [x] AudioEngine/Audio/AudioSystem.cpp — 日英コメント付け
- [x] AudioEngine/Audio/AudioSystem.h — 日英コメント付け
- [x] AudioEngine/Audio/MixerSystem.cpp — 日英コメント付け
- [x] AudioEngine/Audio/MixerSystem.h — 日英コメント付け
- [x] AudioEngine/Audio/Sound.cpp — 日英コメント付け
- [x] AudioEngine/Audio/Sound.h — 日英コメント付け

### AudioEngine/CRI
- [x] AudioEngine/CRI/CriAllocator.h — 日英コメント付け（関数ポインタ経由でしか呼ばれないので、.cpp へ移しても性能は変わらない）
- [x] AudioEngine/CRI/CriManager.cpp — 日英コメント付け、Get/Set を外す（音量系は setter/getter オーバーロード化）、GetCategoryNames → CategoryNameList、ResetVolumes → ResetVolume、LoadBindings/SaveBindings → Load/Save
- [x] AudioEngine/CRI/CriManager.h — 同上

### AudioEngine
- [ ] AudioEngine/Prelude.cpp

## Runtime (0/8)

### Runtime/Application
- [ ] Runtime/Application/Engine.cpp
- [ ] Runtime/Application/Engine.h
- [ ] Runtime/Application/Framework.cpp
- [ ] Runtime/Application/Framework.h
- [ ] Runtime/Application/Main.cpp
- [ ] Runtime/Application/Window.cpp
- [ ] Runtime/Application/Window.h

### Runtime
- [ ] Runtime/Prelude.cpp

## Editor (0/83)

### Editor/Editor
- [ ] Editor/Editor/Editor.cpp
- [ ] Editor/Editor/Editor.h
- [ ] Editor/Editor/EditorContext.h
- [ ] Editor/Editor/Engine.cpp
- [ ] Editor/Editor/Engine.h
- [ ] Editor/Editor/Framework.cpp
- [ ] Editor/Editor/Framework.h
- [ ] Editor/Editor/GizmoContext.h

### Editor/Editor/ImGui
- [ ] Editor/Editor/ImGui/ImGuiCommon.h
- [ ] Editor/Editor/ImGui/ImGuiRenderer.cpp
- [ ] Editor/Editor/ImGui/ImGuiRenderer.h
- [ ] Editor/Editor/ImGui/ImGuiTexture.cpp
- [ ] Editor/Editor/ImGui/ImGuiTexture.h

### Editor/Editor
- [ ] Editor/Editor/Main.cpp

### Editor/Editor/Panel
- [ ] Editor/Editor/Panel/AddComponentPanel.cpp
- [ ] Editor/Editor/Panel/AddComponentPanel.h
- [ ] Editor/Editor/Panel/AnimatorControllerPanel.cpp
- [ ] Editor/Editor/Panel/AnimatorControllerPanel.h
- [ ] Editor/Editor/Panel/AvatarPanel.cpp
- [ ] Editor/Editor/Panel/AvatarPanel.h
- [ ] Editor/Editor/Panel/BootScreenPanel.cpp
- [ ] Editor/Editor/Panel/BootScreenPanel.h
- [ ] Editor/Editor/Panel/CanvasViewPanel.cpp
- [ ] Editor/Editor/Panel/CanvasViewPanel.h
- [ ] Editor/Editor/Panel/ConfigPanel.cpp
- [ ] Editor/Editor/Panel/ConfigPanel.h
- [ ] Editor/Editor/Panel/ConsolePanel.cpp
- [ ] Editor/Editor/Panel/ConsolePanel.h
- [ ] Editor/Editor/Panel/ContentsDrawerPanel.cpp
- [ ] Editor/Editor/Panel/ContentsDrawerPanel.h
- [ ] Editor/Editor/Panel/ControlPanel.cpp
- [ ] Editor/Editor/Panel/ControlPanel.h
- [ ] Editor/Editor/Panel/DiagnosticsPanel.cpp
- [ ] Editor/Editor/Panel/DiagnosticsPanel.h
- [ ] Editor/Editor/Panel/EditorWindowPanel.cpp
- [ ] Editor/Editor/Panel/EditorWindowPanel.h
- [ ] Editor/Editor/Panel/EnvironmentMenuPanel.cpp
- [ ] Editor/Editor/Panel/EnvironmentMenuPanel.h
- [ ] Editor/Editor/Panel/GameWindowPanel.cpp
- [ ] Editor/Editor/Panel/GameWindowPanel.h
- [ ] Editor/Editor/Panel/GraphicsMenuPanel.cpp
- [ ] Editor/Editor/Panel/GraphicsMenuPanel.h
- [ ] Editor/Editor/Panel/GuizmoPanel2D.cpp
- [ ] Editor/Editor/Panel/GuizmoPanel2D.h
- [ ] Editor/Editor/Panel/GuizmoPanel3D.cpp
- [ ] Editor/Editor/Panel/GuizmoPanel3D.h
- [ ] Editor/Editor/Panel/HierarchyPanel.cpp
- [ ] Editor/Editor/Panel/HierarchyPanel.h
- [ ] Editor/Editor/Panel/InspectorPanel.cpp
- [ ] Editor/Editor/Panel/InspectorPanel.h
- [ ] Editor/Editor/Panel/LayerSettingsPanel.cpp
- [ ] Editor/Editor/Panel/LayerSettingsPanel.h
- [ ] Editor/Editor/Panel/MaterialViewerPanel.cpp
- [ ] Editor/Editor/Panel/MaterialViewerPanel.h
- [ ] Editor/Editor/Panel/MenuBarPanel.cpp
- [ ] Editor/Editor/Panel/MenuBarPanel.h
- [ ] Editor/Editor/Panel/ModelTransformPanel.cpp
- [ ] Editor/Editor/Panel/ModelTransformPanel.h
- [ ] Editor/Editor/Panel/ProfilerPanel.cpp
- [ ] Editor/Editor/Panel/ProfilerPanel.h
- [ ] Editor/Editor/Panel/RasterizationPanel.cpp
- [ ] Editor/Editor/Panel/RasterizationPanel.h
- [ ] Editor/Editor/Panel/RaytracingPanel.cpp
- [ ] Editor/Editor/Panel/RaytracingPanel.h
- [ ] Editor/Editor/Panel/ScreenSpacePanel.cpp
- [ ] Editor/Editor/Panel/ScreenSpacePanel.h
- [ ] Editor/Editor/Panel/ShortCutKeyPanel.cpp
- [ ] Editor/Editor/Panel/ShortCutKeyPanel.h
- [ ] Editor/Editor/Panel/SkeletonControllerPanel.cpp
- [ ] Editor/Editor/Panel/SkeletonControllerPanel.h
- [ ] Editor/Editor/Panel/SpecMemoPanel.cpp
- [ ] Editor/Editor/Panel/SpecMemoPanel.h
- [ ] Editor/Editor/Panel/TimelinePanel.cpp
- [ ] Editor/Editor/Panel/TimelinePanel.h
- [ ] Editor/Editor/Panel/TodoListPanel.cpp
- [ ] Editor/Editor/Panel/TodoListPanel.h
- [ ] Editor/Editor/Panel/VersionPanel.cpp
- [ ] Editor/Editor/Panel/VersionPanel.h

### Editor/Editor
- [ ] Editor/Editor/ViewportPicking.cpp
- [ ] Editor/Editor/ViewportPicking.h
- [ ] Editor/Editor/Window.cpp
- [ ] Editor/Editor/Window.h

### Editor
- [ ] Editor/Prelude.cpp

## SeedCore (0/6)

### SeedCore
- [ ] SeedCore/dllmain.cpp
- [ ] SeedCore/ScComponent.h
- [ ] SeedCore/ScInput.h
- [ ] SeedCore/ScMath.h
- [ ] SeedCore/ScPrefab.h
- [ ] SeedCore/ScScene.h

## UserProject (0/6)

### UserProject
- [ ] UserProject/dllmain.cpp

### UserProject/EntryPoint
- [ ] UserProject/EntryPoint/EntryGame.cpp
- [ ] UserProject/EntryPoint/EntryGame.h

### UserProject
- [ ] UserProject/Prelude.cpp

### UserProject/Tutorial
- [ ] UserProject/Tutorial/ScTutorial.cpp
- [ ] UserProject/Tutorial/ScTutorial.h


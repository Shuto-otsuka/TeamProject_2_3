# リファクタ・コメント付け 引き継ぎ資料

このファイルは、会話の文脈も CLAUDE.md も持たない AI が、この作業をそのまま続けられるように書いたもの。
作業を始める前に最後まで読むこと。進捗そのものは `RefactorProgress.md` にある。

---

## 1. 何をしている作業か

- SeedCore（Windows / DirectX 12 の自作 C++ ゲームエンジン）のソース全体に、日英のコメントを付けていく作業と、ユーザーが気に入らない箇所のリファクタ。
- **ユーザーが1ファイルずつ指示を出し、AI はその指示どおりに直す**。AI から改善案を並べて提案する進め方は、ユーザーに明確に断られている（→ 4章）。
- 対象は約 1,050 ファイル（`.cpp` / `.h` / `.hlsl` / `.hlsli`）。一覧とチェックは `RefactorProgress.md`。

---

## 2. プロジェクトの最低限の知識

- ビルドは Visual Studio のソリューション `Runtime/Runtime.sln` だけ（CMake 等は無い）。**AI はビルドしない**。
- モジュールと依存関係：
  ```
  FoundationEngine（ECS・リソース・入力・ジョブ・シリアライズ等の土台）
    ├── PhysicsEngine（JoltPhysics のラッパー）
    ├── GraphicsEngine（D3D12 レンダラ）
    ├── AIEngine
    └── AudioEngine（CRI ADX2 のラッパー）
  SeedCore（全モジュールを束ねるだけのプロジェクト）
  UserProject / Runtime / Editor は SeedCore に依存
  ```
- 各モジュールの `Prelude.h`（主に `FoundationEngine/Prelude.h`）がサードパーティのヘッダと共通マクロを一括で include している。外部ライブラリの新しいヘッダは .cpp ではなく Prelude に足す。
- `External/` は外部ライブラリ（Jolt / DirectXTK / DirectXTex / DLSS / ImGui / SDL3 / nlohmann json / CRI ADX2 / FreeType 等）。**触らない**。
- コード生成：`FoundationEngine` のビルド前イベントで `Tools/Python/Reflection.py` と `Payload.py` が動き、`SC_REFLECTION_FIELD()` や `SC_PAYLOAD_FIELD(type)` などの目印マクロを読んで `ReflectionRegistry.cpp` / `PayloadRegistry.cpp` の自動生成区間（`// [REFLECTION_AUTO_BEGIN]` 〜 `END` など）を書き換える。**生成区間は手で編集しない**。
- 暗号化：`.icon` / `.logo` / `.texture` / `.scg` / `.audio` / `.dx.cso` などは AES-256-CBC（鍵は `FoundationEngine/Prelude.h` の `SC_ENCRYPTION_KEY_SEED` の SHA-256）。

---

## 3. 絶対に守ること（禁止事項）

- `.vcxproj` / `.vcxproj.filters` を編集しない。新しいファイルを作ったら、登録はユーザーに頼む。
- ビルドしない（msbuild などを実行しない）。コンパイル確認はユーザーがする。エラーが出たらユーザーがログを貼ってくる。
- `git commit` しない（`git add` もしない）。確認も求めない。変更は作業ツリーに置いたままにする。
- `UserProject/Assets/**` などユーザーのコンテンツを、削除・移動・リネームしない（明示の許可があるときだけ）。
- `External/` のファイルを変えない。外部 API の名前も変えない（例：ImGui の `IsItemHovered`、Jolt の `IsAdded` / `JPH::Result::IsValid`、FBX の `FbxProperty::IsValid`、CRI の `criAtomEx_IsInitialized`）。
- 頼まれていないのにコメントを足さない（このリファクタ作業中は、ユーザーが「コメントを」と言ったファイルには付ける）。

---

## 4. 作業の進め方（ユーザーの好み）

- **ユーザーが対象ファイルと指示を出す。AI はそのとおりにだけ直す。**
  - ファイルを先読みして「気になる点」「直し方の案」を並べない。ユーザーから「いや一ファイルずつおれが指示するからそれに従って」と言われている。
  - ファイル名だけ言われたときは、中身をひと目で分かる程度に要約して（役割・公開関数の一覧程度）、指示を待つ。
- **「〜これは？」は質問であって、変更の指示ではない**。その関数が何をしていて、どこから使われているかを説明するだけにして、編集はしない。
- 指示が曖昧でも、確認の質問を連発しない。常識的に読めるならそのまま進め、解釈を一言添える。選択肢形式の質問はユーザーに閉じられることが多い。
- **1ファイル（.h と .cpp の組）が終わるたび**に `RefactorProgress.md` を更新する：
  - `- [ ]` を `- [x]` にし、行末に ` — ` で指示の要点を1行で書く。
  - モジュール見出しの `(済/全体)` を数え直す。
  - 区切りでユーザーに言われたら「チェックポイント」節に1行足す。
- ユーザーが「〜終わり」「ここは終わりで」と言ったら、そのファイルを完了として記録する。
- 途中でユーザーから割り込みの指示が来たら、それを優先する（例：「後でいいよ」と言われたら、その調査はやめる）。
- 名前を決めるときに候補を出すことはある（ユーザーが「名前気に入らない」と言ったときなど）。そのときは候補を数個並べ、ユーザーが選んだものにする。
- 一括置換など広範囲の変更は、実施前に「外部 API と同名でないか」「新しい名前が同じスコープの既存名とぶつからないか」を必ず調べる（→ 8章）。

---

## 5. コメントの書き方

### 5.1 形式

- **クラス / 構造体 / enum class / 関数の宣言**と、**その .cpp 側の定義**の両方に、`/** */` の日英ブロックを付ける：
  ```cpp
  /**
  * [EN]
  * <English description>
  *
  * ---------------------------------------------------------------------
  *
  * [JP]
  * <日本語の説明>
  */
  ```
  - .cpp 側は、ヘッダより短くまとめてよい。
- **変数（メンバ・ローカル）、enum の値、関数の中の処理**には `///` の日英ペア：
  ```cpp
  /// [EN] Untrack first so the destructor never destroys this handle a second time.
  /// [JP] 先に追跡から外し、デストラクタがこのハンドルを二重に破棄しないようにする。
  ```
- **`///` のブロックは日英あわせて 4 行まで**。長い説明を1つ書くのではなく、処理の区切りごとに短いコメントを細かく付ける。基本形は `/// [EN]` 1行 + `/// [JP]` 1行。既存の `///` も、そのファイルを触るときに 4 行以内へ直す。`/** */` のブロックには行数制限は無い。
- 細かく付ける：自明でない関数・メンバ・ローカル変数・処理の区切りには漏れなく付ける。
- **`#include` される HLSL（主に `.hlsli`）は英語のみ**（DXC が日本語で壊れるため）。include されないトップレベルの `.hlsl` は日英でよい。

### 5.2 内容

- **設計そのものを説明する。経緯は書かない**。「このバグが出たので〜」「以前は〜だった」「ツールの都合で〜」のような話は書かない。まだ有効な事実なら、技術の説明として書き直す。
- 事実だけを書く。CRI や Jolt など外部 API の挙動を書くときは、`External/` のヘッダのコメントで確認してから書く（例：`CriFsIoInterface` の枠の並び、`criAtomExPlayer_Resume` の `ALL_PLAYBACK` の意味）。
- コメントを書くときは、そのファイルの既存コメントが古くなっていないかも見直す。
- コメントを付けたら、**コメント以外のコードが変わっていないことを HEAD と突き合わせて確認する**（→ 8章）。

---

## 6. コーディング規約

- 名前空間は `namespace SeedCore { ... }`。
- 型は自前の別名を使う：`Int` / `Int8`〜`Int64`、`Uint` / `Uint8`〜`Uint64`、`Float`、`Double`、`Bool`、`Char`、`Size`、`Byte`、`String`、`DynamicArray<T>`（`std::vector` ではない）。`Byte` は `char` なので、`const uint8_t*` を取る API には `reinterpret_cast<const Uint8*>` が要る。
- 名前：クラス・関数・enum は `PascalCase`。メンバ変数は `camelCase_`（末尾アンダースコア）。ローカル変数・引数は `camelCase`。HLSL の変数は `snake_case`（構造体メンバは末尾 `_`）。定数に `kFoo` は使わない。
- **Bool を返す状態の問い合わせは `Is` を付けない**（`Playing()` / `Loaded()` / `Focused()`）。`Has〜` はそのまま。2026-09-21 にエンジン全体で改名済み。
- ホストが受け取る合図は `RequestX()` / `ConsumeX()` の組（例：`World::RequestQuit` / `ConsumeQuit`）。
- 波括弧は Allman。1行の `if` でも必ず波括弧を付ける。
- 中身の無い関数本体は `/// No Code` を書く。
- インデントはタブのみ。
- `const` の前に空白を入れない（`GetX()const`）。
- ループ変数に `i` を使わない（`index` や `actorIndex` など）。
- `auto` は、イテレータやラムダなど型が書けない所だけ。範囲 for でも型を書く。
- 初期化は `Int value = 1;`。`D3D12_RESOURCE_DESC desc{};` のような Desc 構造体だけ `{}`。
- include は常にプロジェクトからの山括弧パス（`#include <FoundationEngine/ECS/Entity.h>`）。`#pragma once`。
- 関数の引数リストは、宣言でも呼び出しでも折り返さず1行で書く。
- **ヘッダに関数本体を書かない**（1行のゲッターでも .cpp に書く）。
- 名前空間直下の関数（フリー関数）は避け、クラスの static 関数にする。シングルトンは作らない。
- 新しいメンバや関数は、ファイルの末尾に足すのではなく、関連するものの隣に置く。ヘッダと .cpp で並び順を揃える。
- 処理はヘルパー関数に切り出さず、使う場所に直接書く（ユーザーは「べた書き」を好む）。
- D3D12 の略語を識別子に使わない（`Srv` ではなく `ShaderResourceView`。`Desc` は可）。
- `std::ranges` のアルゴリズムを優先する。
- ローカル変数に `out` という名前を使わない（SAL マクロと衝突する）。
- ファイルは UTF-8、改行は CRLF。**BOM の有無は元のファイルに合わせる**（ほとんどのファイルは BOM 無し）。

---

## 7. 現在地（2026-09-22 時点）

- **AudioEngine（21/22）と PhysicsEngine（46/47）は、今ある分はひとまず終了**。残りはどちらも Prelude.cpp だけ。
- 次は Editor → FoundationEngine → GraphicsEngine の順（ファイル数の少ない順）。どのファイルから始めるかはユーザーが指定する。
- 2026-09-24：FoundationEngine の JobSystem・Log の全ファイルと Utility の大半に、Sharing（`Resource/Sharing/SharedLockTable.*`）と同じ密度でコメントを付けた（コードは変更なし）。このときユーザーから「分けずに一気に」「古いコメントもしっかり見て」と指示があった。まとめて頼まれたときは、1ファイルごとに止めずに進め、既存のコメントも実装と照らし合わせて直す。
- SeedCore / AIEngine / UserProject / Runtime は「根幹または実装が薄い」ので後回し、とユーザーが決めた。
- 2026-09-22 に決まった命名：参照を返すゲッターも含めて Get/Set を外す（対になるものは引数あり＝設定・引数なし＝取得のオーバーロード）、複数形のコレクションは 〜List、Resolve〜 は単数形、読み込み待ちを仕上げる関数は Build ＋ Pending（呼ぶのが System だけなら private ＋ friend）。

---

## 8. 確認の手順（PowerShell）

### コメント以外のコードが変わっていないかの確認

リネームがあれば HEAD 側に反映してから、`///` の行・`/** */` のブロック・空行を除いて比較する：
```powershell
$f = "AudioEngine/Audio/AudioByteStream.cpp"
$head = (git show "HEAD:$f") -join "`n"
# 例：$head = $head.Replace('IsReadComplete', 'Complete')
$strip = { param($s) (($s -split "`r?`n") | Where-Object { $_.Trim() -ne '' -and $_.Trim() -notmatch '^(///|/\*\*|\*)' } | ForEach-Object { $_.Trim() }) -join "`n" }
if ((& $strip $head) -eq (& $strip ([IO.File]::ReadAllText($f)))) { "code identical" } else { "DIFFERS" }
```

### 改行を CRLF に揃える（BOM は保持）

Write ツールで書いたファイルは LF になるので、必ず最後にこれを通す：
```powershell
$f = "AudioEngine/Audio/Audio.cpp"
$bytes = [IO.File]::ReadAllBytes($f)
$bom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
$t = [IO.File]::ReadAllText($f)
[IO.File]::WriteAllText($f, ($t -replace "`r?`n", "`r`n"), (New-Object Text.UTF8Encoding($bom)))
```
- このリポジトリは `core.autocrlf=true`。元が LF のファイルを丸ごと CRLF にすると、`git diff` の全行が変わったように見える。本当の差分は `git diff --ignore-cr-at-eol` で見る。

### 名前を一括で変えるとき

1. 自前で宣言している名前を集める（戻り値の型で絞らないこと。`CriFsIoError` を返す `IsReadComplete` を見落とした前例がある）。
2. `External/` のヘッダに同名の関数が無いかを調べる。同名があれば、呼び出し側を1件ずつ見て、自前の型への呼び出しだけを変える。
3. 新しい名前が、同じクラスや同じファイルの既存の識別子とぶつからないかを調べる。
4. 置換は単語境界（`\b`）かつ大文字小文字を区別して行う（`isPlaying` のようなローカル変数を巻き込まない）。
5. 置換後に、旧名が残っていないか検索する。

### シェーダの構文チェック（ビルドせずに）

```powershell
& "External/DXC/dxc.exe" -T ps_6_6 -E main -HV 2021 "GraphicsEngine/Shape/Screen/BootScreenPS.hlsl" -Fo "$env:TEMP\check.cso"
```
- HLSL 2021 ではベクトルの三項演算子が使えないので、`select()` を使う。

---

## 9. 持ち越し・未解決

### ユーザーの判断待ち

- **HLSL の `Is〜` 関数 8 個**を `Is` なしに揃えるか：
  - `Culling.hlsli` の `IsBackFace` / `IsLodSelected` / `IsVisibleHiZ` / `IsVisibleInFrustum` / `IsVisibleInScreen`
  - `Material.hlsli` の `IsMaterialPassthrough`
  - `Reflection.hlsli` の `IsReflectionRayOccluded`
  - `ReflectionDenoiseCS.hlsl` の `IsReflectionReprojectionValid`
  - `ShadowDenoiseCS.hlsl` の `IsReprojectionValid`
- **CLAUDE.md の命名規約**に「Bool の問い合わせは `HasX()` / `IsX()` / `ExistsX()` 形式」と残っていて、`Is` なしの方針と食い違う。直すかどうか。

### 改名し残し（ファイルを見るときに合わせて直す、とユーザーが決めた）

- `RuntimeBuilder::IsBuilding`
- `preview->IsDelivery`

どちらも、戻り値の型の条件で一括置換から漏れたもの。

### GraphicsEngine のリファクタで片付ける（2026-09-22 にユーザーが決定）

- コライダーのデバッグ描画の収集を `PhysicsSystem::GatherColliderInstances` から `Renderer::GatherColliders(World&)` へ移した。移しただけで、中身（6 形状のべた書きループ、2D の `100000.0f` オフセットと `SC_CANVAS.Height` 反転）は元のまま。
- `FoundationEngine/Interop/ColliderInstance.h`（`ColliderStructuredBuffer` / `ColliderShapeKind`）は GPU 用の型なので GraphicsEngine 側へ移す。ファイル内のコメントは「Physics が生成する」「PhysicsSystem::GatherColliderInstances 参照」のまま古くなっている。

### 気づいたが未対応

- `Audio::criManager_` はコンストラクタで受け取るだけで、`Audio` の中では使われていない。

2026-09-24 のコメント付けで見つけたバグは、同日に全て修正した（ビルド未確認）。JobSystem は Taskflow の master（IMPLICITLY_ANCHORED・AdoptedModule・NonpreemptiveRuntime がある版）を元にしているので、それと見比べて直した。

- Taskflow と比べて抜けていたものを追加
  - 例外処理。各タスクの呼び出しを try/catch で囲んで `ProcessException` へ渡す（`SC_DISABLE_EXCEPTION_HANDLING` が 0 のとき）。
  - `JobExceptionState::EXPLICITLY_ANCHORED` と `JobExplicitAnchorGuard`（JobNodeBase.h）、`JobNodeBase::RethrowException`。
  - `CorunGraph` / `JobPreemptiveRuntime::Corun` で、待つ間ノードをアンカーにし、終わったら例外を投げ直す。
  - `ProcessException` を明示アンカー優先の形にした。トポロジーは最初から EXPLICITLY_ANCHORED。
- WorkerCommon.h のマクロを `#if` で判定するように揃えた。これで既定の動作は Taskflow と同じになる（Task プール無効、NonblockingNotifier、例外処理有効）。直す前は `#ifdef` のせいでプールと AtomicNotifier が有効になっていた。
- `JobExecutor::RunUntil(JobTaskflow&&, …)` は、タスクフローを `JobTopology::ownedTaskflow_` に移して所有させる。Taskflow はここを silent_async で包んでいるが、こちらには async が無いので、トポロジーに持たせる形にした。
- `UnboundedWorkerQueue::pop` のメモリ順序を Taskflow と同じ形（relaxed store＋seq_cst fence）に戻した。
- そのほか：Strong/Weak の数え方、セマフォ獲得の条件、Subflow の処理を呼ぶ、runtime のカウント、WaitForTask の範囲、`Retain(false)`、`Join` のメッセージ、`Cancelled()`、JobTask の Acquire/Release/Each*/ToString/Work、JobVector の reverse・resize・push_back・set_size、notifier の friend 名、AtomicNotifier.h の `#pragma once`、`Composed` の `RetrieveGraph`、`JobNode` のコンストラクタ（`const S&&` → `S&&`）、.cpp に置かれていた `inline`（JobWorker・`JobPreemptiveRuntime::Worker`・`ToString`）、`Corun`/`CorunUntil`/n=0 の空の if を SC_THROW に、Bitset::resize、FlatMap の初期化子リスト、ArtMap::prefix_search の型。
- 前回挙げた「`BulkSpillRoundRobin` が first を進めていない」は誤り。`bulk_push` が `I&` で受けて進めている（Taskflow も同じ）。

### この日の作業で、まだビルド確認されていない変更（抜粋）

- `Is〜` → `Is` なしへの一括改名（117 ファイル）。
- Active の扱いの統一：非アクティブのアクターは、次のものを止める・描かない。
  - Tick 系 / Awake / Start
  - 物理ボディ（`Physics::SuspendBody` / `ResumeBody`、`PhysicsSystem::ApplyActive`）
  - ジョイント（`Physics::RefreshJoints`）
  - Spawner / Lifetime / MoveSystem / Constraint / Weather / AudioSource / コライダー表示 / 3D ビューのクリック選択
- エディタの Play / Stop で、次のものを保存・復元する（`ControlPanel`）：
  - 描画設定（raytracing / screenSpace / rasterization）
  - 音量
  - マウスキャプチャ・振動
  - タイムスケール（`GameTimer::Stop` で 1 に戻る）
- 矩形ギズモ（Rect Tool、Ctrl+T）を 2D / 3D の両方に追加。アイコンは `Editor/Icon/Viewport/Rect.icon`。
- 複数選択時にアウトラインを結合。3D ビューの Ctrl+クリックによる複数選択。
- 起動ローディング画面：
  - 設定と描画：`BootConfig`、`BootScreen`（+ `BootScreenPS.hlsl`）、`BootScreenRenderer`
  - Panel：`BootScreenPanel`（ツール → 起動ローディング画面、インスペクター乗っ取り型）
- SplashScreen はロゴ4枚だけに整理。

### 新規ファイル（vcxproj への登録状況は要確認）

- `FoundationEngine/Resource/BootConfig.h/.cpp`、`IconConfig.h/.cpp`
- `GraphicsEngine/Shape/Screen/BootScreen.h/.cpp`、`BootScreenPS.hlsl`、`LetterScreen.h/.cpp`、`LetterScreenPS.hlsl`
- `GraphicsEngine/Renderer/BootScreenRenderer.h/.cpp`
- `Editor/Editor/Panel/BootScreenPanel.h/.cpp`
- `Runtime/Application/Engine.h/.cpp`、`Window.h/.cpp`
- 上のファイルは、どれも `RefactorProgress.md` の一覧に入っている。今後この作業中に新しく作ったファイルは、一覧の該当フォルダの下に足す。

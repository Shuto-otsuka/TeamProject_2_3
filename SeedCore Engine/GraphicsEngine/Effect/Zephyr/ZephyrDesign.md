# Zephyr 設計メモ

GPU パーティクルエフェクトシステム「Zephyr」の設計メモ。

## 基本方針

- エフェクトはノードグラフで組む。
- 1エフェクトは複数のエミッターを持てる（火花・煙・閃光など）。
- **1エミッターにつきコンピュートシェーダー（CS）を1本自動生成する**（Niagara と同じ）。エミッターごとにノードグラフ・パーティクルプール・CS を持ち、グラフで使ったノードの処理だけを `SpawnMain` / `UpdateMain` に展開する。
- ノードは「固定機能ノード」（エンジン同梱）と「カスタムノード」（ユーザー作成）の2種類。どちらも同じ生成 CS に組み込まれる。
- 構成は Unreal（Niagara）に寄せつつ、必要なものに削る。

## エミッターの種類

| 種類 | 内容 | CS 生成 |
|---|---|---|
| Zephyr | ノードグラフで組む GPU パーティクル | あり |
| Effekseer | 既存の Effekseer エフェクト（`.efkefc`）を再生する | なし |
| Houdini | Houdini 由来のデータを再生する（仮。中身は未定） | 未定 |

```
爆発.effect
 ├ 火花エミッター      (Zephyr)
 ├ 煙エミッター        (Zephyr)
 └ Effekseer エミッター (.efkefc を再生)
```

### AssetType

| AssetType | 対象 | 拡張子 |
|---|---|---|
| `Effekseer` | Effekseer エフェクト（今の `Effect` を改名） | `.efkefc` / `.effekseer` |
| `Effect` | エンジン製エフェクト（Zephyr） | `.zephyr` |
| `Houdini` | Houdini 由来のデータ（仮） | 未定 |

- enum では `Effect` / `Effekseer` / `Houdini` を並べて挿入する。
- `AssetType` の数値は共有カタログ（`SharedCatalog` の `"type"`）に保存されているが、使うのは `ResourceSync::Gather`（未ダウンロードの共有アセットを一覧に出す処理）だけ。ダウンロード後はローカルの走査が拡張子から判定し直す。
- 挿入で番号がずれても困らないように、`Gather` はカタログの数値ではなく、パスの拡張子から種類を判定するように変える。拡張子の判定は `ResourceCache` の走査から切り出して共通化する。
- 変更箇所：`Asset.h`（enum）、`ResourceCache.cpp`（拡張子の分類）、`ResourceSync.cpp`（`Gather`）、`EffekseerResource.cpp`（`REGISTER_ASSET`）、`ContentsDrawerPanel.cpp`（アイコン・ペイロード名）。

### Effekseer エミッター

- パラメータ: アセット / オフセット / 回転 / スケール / 再生速度 / 開始遅延 / ループ / 色
- 読み込み（`EffekseerLoader` / `EffekseerResource`）と描画（`EffekseerRenderer`）は既存のものを使う。
- 再生（`Play` / `SetMatrix` / `StopRoot`）は今どこにも実装されていないので、`Effect` コンポーネント側に作る。
- 将来の拡張：Zephyr のパーティクルの死亡位置に Effekseer を出すなどのイベント連携（初版ではやらない）。

### Houdini エミッター（仮）

- ノードとしてだけ置いておく。中身（VAT / Point Cache など）は後で決める。

## 受け口ノード

マテリアルエディタの出力ノードと同じように、グラフには最初から次の3つが置かれていて、消せない。ほかのノードの出力をこのピンに繋いでいく。

```
[Emitter]   ← Rate, Burst, Duration, Loop, Delay, MaxParticles
[Spawn]     ← 発生形状, Lifetime, Velocity, Color, Size, Rotation, AngularVelocity
[Render]    ← パーティクル形状, Texture, Trail
```

- Spawn の Rotation と AngularVelocity は、扱いやすいようにオイラー角（度）の float3 で受け取り、内部でクォータニオンに変換する。

## 固定機能ノード

### 動きのテンプレート

| ノード | フェーズ | パラメータ（既定値） | 計算 | 状態 |
|---|---|---|---|---|
| Gravity | 力 | gravity: float3 (0, -9.8, 0) | `force += gravity` | 実装済み（旧形式） |
| Drag | 力 | drag: float (1.0) | `force -= drag * velocity` | 実装済み（旧形式） |
| Acceleration | 力 | acceleration: float3 (0, 0, 0)、space: Local / World | `force += acceleration`（Local ならエミッター行列で回す） | 未実装 |
| Vortex | 力 | center: float3、axis: float3 (0, 1, 0)、strength、pull、radius | 軸の周りの接線方向に strength、軸へ向かって pull。radius より外は減衰 | 未実装 |
| Orbit | 位置 | center: float3、axis: float3 (0, 1, 0)、speed（度/秒） | `position = center + Rotate(position - center, axis, speed * dt)` | 未実装 |
| Turbulence | 力 | strength、frequency、scroll: float3、octaves (1〜4) | `force += CurlNoise(position * frequency + time * scroll) * strength` | 未実装 |

- Vortex は力なので、粒がだんだん渦に巻き込まれる。Orbit は位置を直接回すので、最初から円軌道を描く。
- 旧形式の Gravity / Drag（`Module/*.hlsli`）は速度を直接書き換えているので、力に足し込む形に直す。

### 発生形状

どの形状も、出力は**位置**と**法線**の2つ。法線は「形状の外向きに飛ばす」初速に使う。共通パラメータとして offset（float3）と rotation（float3）を持つ。

| ノード | パラメータ |
|---|---|
| Point | なし |
| Sphere | radius、thickness（0 = 表面だけ 〜 1 = 中まで詰まる） |
| Hemisphere | radius、thickness |
| Box | size: float3、surfaceOnly: bool |
| Cone | angle（開き角）、radius（根元の半径）、length |
| Circle | radius、thickness、arc（扇の角度、360 で円全体）。Ring は Circle の thickness が小さいものとして表す |
| Line | start: float3、end: float3 |
| Torus | majorRadius、minorRadius |

### 描画

**パーティクル形状**（パーティクル自体を何として描くか）

| ノード | パラメータ |
|---|---|
| Billboard | なし（カメラに正対する板ポリ） |
| Stretched Billboard | stretch: float（速度に対する伸び率） |
| Mesh | model、material |

**Texture**（形状から独立したノード）

| ノード | パラメータ |
|---|---|
| Texture | texture、blendMode（Additive / Alpha / Multiply）、softParticle: float（0 で無効）、flipbook |

**Trail**（パーティクル形状の1つではなく、どの形状にも追加で付けられる別枠）

| ノード | パラメータ |
|---|---|
| Trail | segments（履歴の数）、width、texture（任意） |

- 例：「Mesh の弾＋その軌跡」のように組み合わせられる。
- Trail は履歴が必要なので、`ParticleSeed` ではなく別バッファで持つ。

### 数式

| ノード | パラメータ |
|---|---|
| Ease | type（Linear / Sine / Quad / Cubic / Expo / Back / Elastic / Bounce）、mode（In / Out / InOut）、from、to |
| Sin / Cos | frequency、amplitude、phase、offset |
| Noise | type（Perlin / Simplex / Value / Curl / Voronoi）、frequency、amplitude、octaves |
| Gradient | カラーキーの配列（LUT にベイク） |
| Constant Color | color |
| Curve | キーの配列（LUT にベイク） |
| Random Range | type（float / float3）、min、max |
| Random Direction | coneAngle（180 で全方向） |

四則演算のような生の演算ノードは持たない。強さやオフセットなどは各テンプレートのパラメータで扱う。

#### 入力

数式ノードはどれも入力ピンを1つ持ち、既定値は **Life**。何も繋がなければ Life で動く。入力ピンには次の Input ノードか、ほかの数式ノードの出力を繋ぐ。

| Input ノード | 意味 |
|---|---|
| Life | パーティクルの一生の進み具合（0→1） |
| Age | パーティクルの経過秒数 |
| Time | エミッターの経過秒数 |
| Cycle | エミッター1周期の進み具合（0→1、ループごとに戻る） |
| Speed | 速度の大きさ |
| Seed | パーティクルごとに固定の乱数（0〜1） |

例外:
- Random は入力を取らず、パーティクルごとのシードから値を作る。出力の範囲（min〜max）がパラメータ。
- Noise は位置か時間を入力に取る。Curl Noise は位置を入力にしてベクトルを出力する。

#### 出力の型

| ノード | 出力 |
|---|---|
| Ease / Sin / Cos / Curve | float |
| Gradient / Constant Color | float4（色） |
| Random Range | float または float3 |
| Random Direction / Curl Noise | float3 |

型が合わない接続はエディタ側で弾く。または、Ease などに出力倍率（scale / offset）を持たせて吸収する。

## 繋ぎ方のイメージ

数式ノードの出力を、テンプレート・形状・受け口のピンに繋ぐ。

```
[Life] → [EaseOutCubic] ──→ Vortex.strength      (渦がだんだん弱まる)
[Random 1〜2]  ──→ Spawn.Lifetime
[Gradient]     ──→ Spawn.Color
[Curl Noise]   ──→ Turbulence.direction
[Sphere]       ──→ Spawn.発生形状
[Billboard]    ──→ Render.パーティクル形状
```

## カスタムノード

HLSL で書く。ただし一から書かせるのではなく、フェーズごとのテンプレートを用意して、**固有の計算処理の関数の中身だけ**をユーザーに書かせる。

- エディタで「新規カスタムノード」を作ると、選んだフェーズのテンプレートから `.hlsli` が生成される。
- 入力ピンはファイル先頭の宣言から作る。入力は構造体にまとめて関数に渡す（値は定数バッファ経由）。
- パーティクルの状態は読み取り専用で渡す。書き込みは戻り値だけにして、フェーズの外の状態を壊せないようにする。
- 生成 CS には、選んだフェーズの位置にこの関数の呼び出しが展開される。

### テンプレート例（力フェーズ）

```hlsl
// @custom Spiral
// @phase Force
// @input float strength = 1.0
// @input float3 axis = (0, 1, 0)

/// ↓ ここから上はテンプレートが生成する

float3 Spiral(ParticleContext particle, SpiralInputs inputs)
{
	/// ここだけ書く
	return cross(inputs.axis_, particle.position_) * inputs.strength_;
}
```

### フェーズごとの戻り値

| フェーズ | 戻り値 | 生成 CS 側の扱い |
|---|---|---|
| 力 | `float3`（力） | `force += 戻り値` |
| 位置 | `float3`（位置） | `position = 戻り値` |
| 見た目 | 色・サイズ・回転の構造体 | 対応する属性に代入 |

## ParticleSeed

最初から 3D を前提にする。位置と回転を対にし、回転はクォータニオン、サイズは float3。

```hlsl
struct ParticleSeed
{
	float3 position_;
	float4 rotation_;

	float3 live_velocity_;
	float3 base_velocity_;

	float3 live_angular_velocity_;
	float3 base_angular_velocity_;

	float4 live_color_;
	float4 base_color_;

	float3 live_size_;
	float3 base_size_;

	float  age_;
	float  lifetime_;
	uint   seed_;
};
```

- 36 floats = 144 バイト。`RWStructuredBuffer` は 4 バイト単位で隙間なく詰めて配置される（16 バイト境界の規則があるのは定数バッファだけ）ので、パディングは要らない。
- C++ 側で写すときも、`Float3` / `Float4` は同じ順に並べるだけで一致する。
- `live_` は毎フレーム変わる今の値、`base_` は Spawn 時に決めた値。
- **color / size / angular_velocity**: 見た目フェーズで毎フレーム `live_ = base_ × カーブ` を上書きする（`live_ *= カーブ` にすると掛け算が毎フレーム重なってしまう）。
- **velocity**: 力フェーズで毎フレーム足し込まれていく値なので、上書きはしない。Velocity over Life は、位置の積分のときに倍率として掛けるだけにする。`base_velocity_` は初速の記録。
- **rotation**: Mesh はクォータニオンをそのまま使う。Billboard は常にカメラを向くので、カメラの前方向の軸まわりの成分（ロール）だけを使う。
- `force` はバッファに置かない。毎フレームゼロから足し込み直すので、CS のローカル変数で足りる。
- 旧 `emitter_id_` は削除する（1エミッターに CS 1本・プール1つなので不要）。

## 生成 CS の実行順（フェーズ固定方式）

ノードグラフには上から順という概念がないので、ノードごとに属するフェーズを決めておき、生成 CS ではフェーズ順に展開する。

```
SpawnMain
 ① 発生形状        位置と法線を決める
 ② 初速            base_velocity / live_velocity を決める
 ③ 初期値          lifetime / base_color / base_size / rotation / base_angular_velocity

UpdateMain
 ① 組み込み        age += dt, life = age / lifetime
 ② 力フェーズ      force を足し込む（順不同）
                   Gravity, Acceleration, Vortex, Turbulence, Drag(= -k * velocity)
 ③ 組み込み        live_velocity += force * dt
                   position += live_velocity * (Velocity over Life の倍率) * dt
                   rotation  = normalize(qmul(QuatFromAxisAngle(live_angular_velocity * dt), rotation))
 ④ 位置フェーズ    位置を直接いじる（Orbit）
 ⑤ 見た目フェーズ  live_color / live_size / live_angular_velocity を base × カーブで決める
 ⑥ 組み込み        寿命チェック → dead リストに返却
```

- 力はすべて足し算にするので、グラフ上のノードの並びで結果が変わらない。Drag も速度への掛け算ではなく `-k * velocity` の力として足す。
- 数式ノードは、繋がった先のノードが使う直前に評価する（依存関係の順）。
- カスタムノードは、作るときにどのフェーズ（力 / 位置 / 見た目）に入るかを選ぶ。

## CS の共有

同じ構成のエミッターどうしは CS を共有する。

- グラフの構造（ノードの種類と接続）からハッシュを作り、同じなら同じ CS を使う。
- 数値（Gravity の強さ、Random の範囲など）は定数バッファで渡すので、数値だけが違うエミッターも同じ CS で動く。
- 今の実装はハッシュを「モジュール名の並び」から作っているので、「ノードの種類と接続」から作るように変える。

## バッファ

### パーティクルプール（`ReadWriteStructuredBuffer<T>`）

パーティクル本体、dead リスト、alive リスト（read/write）、カウンターは、GPU が毎フレーム読み書きする。

- `D3D12/Buffer/StructuredBuffer.h` の `ReadWriteStructuredBuffer<T>` を使う。
- 以前は UPLOAD ヒープに UAV を付けて作っていて生成に失敗する状態だった（どこからも使われていなかった）。DEFAULT ヒープで作り、UAV と SRV をバインドレスに登録してインデックスを返す形に直した。
- コンストラクタは `(device, heap, elementCount)`。取得関数は `UnorderedAccessViewIndex()` / `ShaderResourceViewIndex()` / `ElementCount()`。
- 解放は `BindlessHeap::Release(resource, { インデックス… })` で、GPU が使い終わるまで遅らせる。
- 状態遷移のバリアはクラスでは管理しない。使う側で行う。

### 定数バッファ

| クラス | 用途 |
|---|---|
| `StaticConstantBuffer<T>` | 中身の構造体がコンパイル時に決まるもの（旧 `ConstantBuffer<T>` を改名。ファイルは `ConstantBuffer.h` のまま） |
| `DynamicConstantBuffer` | 中身が実行時に決まるもの。Zephyr のパラメータ用（`ConstantBuffer.h` / `.cpp`） |

### パラメータの定数バッファ（可変）

CS をエミッターごとに生成するので、定数バッファのレイアウトも一緒に生成する。カスタムの入力が何個あっても対応できる。

```
生成時:
  グラフ内の全ノードの入力 (固定機能 + カスタムの @input) を集める
   → HLSL のパッキング規則 (16 バイト境界) で並べる
   → HLSL 側: struct EffectParameters { ... } を生成
   → C++ 側: レイアウト表 (名前 → オフセット, 型) を作る

実行時:
  レイアウト表を見て、CPU 側のバイト列の該当オフセットに値を書く → アップロード
```

- レイアウト表は `ZephyrParameterLayout`（仮）が持つ。「名前 → オフセット・型」の管理、控えのバイト列（`DynamicArray<Byte>`）、名前での書き込みを担当する。
- バイト列を GPU に運ぶのは `DynamicConstantBuffer`。型や名前は知らず、バイト列だけを扱う。

```cpp
layout.Set("vortex_0.strength", strength);                 /// 控えのバイト列に書く
buffer.Update(device, layout.Data(), layout.Size());       /// 毎フレーム1回、丸ごと送る
dispatchBuffer.module_index_ = buffer.Index();
```
- カスタムの `@input` を足したり消したりすると構造が変わるので、ハッシュも変わり、CS とレイアウトとバッファを作り直す。値を変えるだけなら、バイト列を書き換えるだけで済む。
- ゲーム側から `Effect` コンポーネント経由で値を変える User Parameter（「爆発の大きさ」など）も、同じ仕組みに載せる。
- 定数バッファの上限は 64KB。カーブやグラデーションは LUT として別バッファに置き、ここには入れない。

### DynamicConstantBuffer の要点

- `StaticConstantBuffer` と同じフレームリング（フレームごとに別の UPLOAD リソースと CBV）。サイズだけを実行時に受け取る。
- クラス自体はテンプレートにしない（中身の構造体が実行時にしか決まらないため）。
- API: `Update(device, data, byteSize)`（と `DynamicArray<T>` を受け取るテンプレート版）、`Address()`、`Index()`。
- `Update` は渡されたバイト列を現在のスロットに丸ごとコピーする。容量（256 に切り上げた確保サイズ）を超えていたら、その場で `Release` → `Allocate` して作り直す。作り直すと `Index()` も変わるが、毎フレーム取り直す決まりなので使う側は影響を受けない。
- サイズが 0、または 65536 バイトを超えるときは確保せず、エラーログを出してインデックスを `SC_INVALID` にする。
- 控えのバイト列は持たない。**部分書き込みとフレームリング**の問題（スロットはフレームごとに別のメモリなので、変えた値だけを直接書くと書かなかったスロットに古い値が残ってちらつく）を避けるため、控えは使う側（`ZephyrParameterLayout`）が持ち、毎フレーム丸ごと `Update` に渡す。
- マップ済みのメモリは write-combined なので、そこから読み返さない。

## アセット形式（`.zephyr`）

**1エフェクト = 1つの `.zephyr` ファイル。** Animator はグラフをコンポーネントの中に持ってシーンと一緒に保存しているが、エフェクトは複数の Actor で使い回し、共有ライブラリでも配るので、独立したファイルにする。

- 構造は Animator に倣う：`SC_SERIALIZE_FIELD` 付きの構造体で、ノードの配列と接続の配列を持ち、ノードはエディタ上の位置を持つ。
- 保存は `JsonOutputArchive` / `JsonInputArchive`（`FoundationEngine/Serialization/Json/JsonArchive.h`）。差分が見えてチームで扱いやすい。
- `Effect` コンポーネントは、Animator の `animationIDs_` と同じく `SC_PAYLOAD_FIELD(Effect)` で `.zephyr` のアセット ID を持つ。

### 構造

```
EffectAsset
 ├ version_
 └ emitters_[]
     ├ name_, type_ (Zephyr / Effekseer / Houdini), enabled_
     ├ nodes_[]                     ← Zephyr のときだけ
     │   ├ id_          (Uint32、消しても変わらない固有番号)
     │   ├ type_        ("Emitter" / "Spawn" / "Render" / "Vortex" / "Ease" / "Custom" …)
     │   ├ positionX_, positionY_
     │   └ parameters_[] { name_, value_ (Float4), assetId_ }
     ├ links_[]                     ← Zephyr のときだけ
     │   └ fromNode_, fromPin_, toNode_, toPin_
     └ effekseer_                   ← Effekseer のときだけ
         └ assetId_, offset_, rotation_, scale_, speed_, delay_, loop_, color_
```

- **ノードは ID で参照する。** Animator は配列の添字で参照しているが、それだとノードを消すたびに後ろの添字がずれて接続を付け替える必要がある。
- **パラメータの値は `Float4` に統一する。** float / float3 / 色などは全部ここに入れ、enum も数値として入れる。テクスチャやモデルの参照は `assetId_` に入れる。
- **受け口ノード（Emitter / Spawn / Render）も `nodes_` に入れ、type で区別する。** 接続を一律に扱える。消せないようにするのはエディタ側で行う。

### 例

```json
{
  "version": 1,
  "emitters": [
    {
      "name": "Sparks", "type": "Zephyr", "enabled": true,
      "nodes": [
        { "id": 1, "type": "Emitter", "x": -300, "y": 0,   "parameters": [ { "name": "Rate", "value": [50, 0, 0, 0] } ] },
        { "id": 2, "type": "Spawn",   "x": 0,    "y": 0,   "parameters": [] },
        { "id": 3, "type": "Render",  "x": 300,  "y": 0,   "parameters": [] },
        { "id": 4, "type": "Sphere",  "x": -300, "y": 150, "parameters": [ { "name": "radius", "value": [1, 0, 0, 0] } ] },
        { "id": 5, "type": "Vortex",  "x": 0,    "y": 300, "parameters": [ { "name": "strength", "value": [2, 0, 0, 0] } ] },
        { "id": 6, "type": "Ease",    "x": -300, "y": 300, "parameters": [ { "name": "type", "value": [3, 0, 0, 0] } ] }
      ],
      "links": [
        { "fromNode": 4, "fromPin": "Position", "toNode": 2, "toPin": "Shape" },
        { "fromNode": 6, "fromPin": "Out",      "toNode": 5, "toPin": "strength" }
      ]
    },
    {
      "name": "Flash", "type": "Effekseer", "enabled": true,
      "effekseer": { "assetId": 1234, "speed": 1.0, "loop": false }
    }
  ]
}
```

### 配布時はバイナリにする（後で）

- 開発中は JSON、配布時はバイナリ（`BinaryOutputArchive` / `BinaryInputArchive`）にする。
- `BinaryArchive` も `JsonArchive` と同じ `Field(name, value)` の形なので、各構造体に `template<typename Archive> void Serialize(Archive&)` を1つ書いておけば、同じコードで両方に対応できる（`.material` が `BinaryArchive` で同じ形を使っている）。
- バイナリはフィールド名を持たず並び順で読むので、パッケージ時に JSON から変換して作るだけにし、直接編集はしない。

## 現在の実装

- `ZephyrModuleRegistry`: モジュール名・include パス・設定構造体名・Spawn/Update の有無を持つ表。現状は Gravity と Drag のみ。
- `ZephyrModuleAssembler`: モジュール名の配列から HLSL を生成して `Generated/Zephyr_<hash>.hlsl` に書き出し、`SpawnMain` / `UpdateMain` を ShaderHotReload でコンピュートシェーダーとして取得する。モジュール名の並びのハッシュでキャッシュしている。
- `ParticlePool.hlsli` / `Dispatch.hlsli`（`ParticleDispatchBuffer`）: パーティクル本体、dead リスト、alive リスト（read/write）、カウンター、モジュール設定、メタ情報を、ディスクリプタヒープのインデックス経由で参照する。
- `Effect`（コンポーネント）、`Zephyr`、`EffectLoader`、`EffectResource`: 中身はまだない。

### 既知の問題

- Update で位置を積分していない（`position += velocity * dt` がない）ので、パーティクルが動かない。
- Spawn 系のモジュールがないので `lifetime_` が 0 のままになり、最初の Update で即座に dead に戻る。
- `SpawnMain` に dead 数の境界チェックがない（`dead_count_ == 0` で値が回り込む）。発生させる数と `dispatch_thread_id` の比較もない。

## 未決事項

1. 実装の順番。
2. 配布時のバイナリ変換をどこで行うか（パッケージの処理に組み込む）。

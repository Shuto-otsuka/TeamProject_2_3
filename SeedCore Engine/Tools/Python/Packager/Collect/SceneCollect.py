import os
import json

LABEL = "Scene コピー"

EXTENSIONS = {".scene"}


def walk(path, context):
    """
    .scene / .prefab から参照されている Asset ID を集める。次を拾う:
      - payload_map（コンポーネント名 -> payload フィールド表示名の集合）に
        載っているフィールドの Asset ID
      - ネストされたプレハブ (nestedPrefabAssetID) と派生元プレハブ (basePrefabAssetID)
      - インスペクタで設定された String フィールドのうち、既知 Asset の
        パス/ファイル名に一致する値（スクリプトが Scene::Change / Prefab::Spawn /
        Scene::AssetID へ渡す名前をインスペクタで持つ使い方）
    Asset ID は Uint32 だが、シーン JSON の "int" は符号付きで保存されるため
    0xFFFFFFFF でマスクして戻す。
    """
    referenced_ids = set()

    def add_id(value):
        asset_id = value & 0xFFFFFFFF
        if asset_id:
            referenced_ids.add(asset_id)

    def walk_fields(fields, payload_names):
        for field in fields:
            string_value = field.get("string", "")
            if string_value and '.' in os.path.basename(string_value):
                referenced_ids.update(context.resolve(string_value))

            if field.get("name") in payload_names:
                if field.get("is_array"):
                    for child in field.get("children", []):
                        add_id(child.get("int", 0))
                else:
                    add_id(field.get("int", 0))

            walk_fields(field.get("children", []), ())

    try:
        with open(path, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception:
        print(f"シーン/プレハブの読み込みに失敗しました: {path}")
        return referenced_ids

    for node in data.get("nodes", []):
        add_id(node.get("nestedPrefabAssetID", 0))
        for comp in node.get("components", []):
            walk_fields(comp.get("fields", []), context.payload_map.get(comp.get("component"), ()))

    add_id(data.get("basePrefabAssetID", 0))
    return referenced_ids


def seed(context):
    paths = []
    scene_dir = os.path.join(context.user_project_root, "Assets", "Scene")
    if os.path.isdir(scene_dir):
        for root, _, files in os.walk(scene_dir):
            for file in files:
                paths.append(os.path.join(root, file))
    return paths


def collect(path, context):
    referenced_ids = walk(path, context) if os.path.splitext(path)[1].lower() in EXTENSIONS else set()
    return context.asset(path), referenced_ids

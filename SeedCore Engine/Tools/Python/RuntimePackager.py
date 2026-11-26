import sys
import os
import re
import struct
import shutil
import hashlib
import concurrent.futures

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
import Payload
from Packager.Collect import IconCollect
from Packager.Collect import PluginCollect
from Packager.Collect import LogoCollect
from Packager.Collect import CompilerCollect
from Packager.Collect import VCRuntimeCollect
from Packager.Collect import ShaderCollect
from Packager.Collect import ConfigCollect
from Packager.Collect import ScriptCollect
from Packager.Collect import SceneCollect
from Packager.Collect import PrefabCollect
from Packager.Collect import EffectCollect
from Packager.Collect import EffekseerCollect
from Packager.Collect import ModelCollect
from Packager.Collect import TextureCollect
from Packager.Collect import AudioCollect
from Packager.Collect import FontCollect
from Packager.Collect import MovieCollect
from Packager.Collect import SkymapCollect
from Packager.Collect import AnimationCollect
from Packager.Collect import MaterialCollect
from Packager.Collect import SkeletonCollect
from Packager.Collect import CollisionCollect
from Packager.Collect import NavmeshCollect

# .meta ファイルを持たない拡張子（ResourceCache::noMetaExtensions_ と同じ）。
NO_META_EXTENSIONS = {".h", ".cpp", ".cs", ".hlsli", ".hlsl", ".efkmodel", ".efkmat", ".efkcurve", ".scg"}

COLLECTS = [
    IconCollect,
    PluginCollect,
    LogoCollect,
    CompilerCollect,
    VCRuntimeCollect,
    ShaderCollect,
    ConfigCollect,
    ScriptCollect,
    SceneCollect,
    PrefabCollect,
    EffectCollect,
    EffekseerCollect,
    ModelCollect,
    TextureCollect,
    AudioCollect,
    FontCollect,
    MovieCollect,
    SkymapCollect,
    AnimationCollect,
    MaterialCollect,
    SkeletonCollect,
    CollisionCollect,
    NavmeshCollect,
]


def fnv1a(name):
    """
    FoundationEngine/Serialization/Binary/BinaryArchive.h の BinaryField と
    同じ FNV-1a。フィールド名からタグ付きバイナリのフィールドIDを求める。
    """
    h = 2166136261
    for byte in name.encode("utf-8"):
        h ^= byte
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def read_tagged_binary_fields(raw):
    """
    FoundationEngine/Serialization/Binary/BinaryArchive.h が書き出す、
    [fieldId:u32][size:u32][payload] の並びをスキャンし、
    {fieldId: payloadバイト列} を返す。
    """
    fields = {}
    offset = 0
    length = len(raw)
    while offset + 8 <= length:
        field_id, size = struct.unpack_from('<II', raw, offset)
        offset += 8
        if offset + size > length:
            break
        fields[field_id] = raw[offset:offset + size]
        offset += size
    return fields


GUID_FIELD_ID = fnv1a("guid")


def aes_gf_mul(a, b):
    result = 0
    while b:
        if b & 1:
            result ^= a
        a = ((a << 1) ^ (0x1B if a & 0x80 else 0)) & 0xFF
        b >>= 1
    return result


AES_SBOX = [0] * 256
_p = _q = 1
while True:
    _p = (_p ^ (_p << 1) ^ (0x1B if _p & 0x80 else 0)) & 0xFF
    _q = (_q ^ (_q << 1)) & 0xFF
    _q = (_q ^ (_q << 2)) & 0xFF
    _q = (_q ^ (_q << 4)) & 0xFF
    if _q & 0x80:
        _q ^= 0x09
    AES_SBOX[_p] = _q ^ ((_q << 1 | _q >> 7) & 0xFF) ^ ((_q << 2 | _q >> 6) & 0xFF) ^ ((_q << 3 | _q >> 5) & 0xFF) ^ ((_q << 4 | _q >> 4) & 0xFF) ^ 0x63
    if _p == 1:
        break
AES_SBOX[0] = 0x63

AES_INV_SBOX = [0] * 256
for _index, _value in enumerate(AES_SBOX):
    AES_INV_SBOX[_value] = _index

AES_MUL2 = [aes_gf_mul(_value, 2) for _value in range(256)]
AES_MUL3 = [aes_gf_mul(_value, 3) for _value in range(256)]
AES_MUL9 = [aes_gf_mul(_value, 9) for _value in range(256)]
AES_MUL11 = [aes_gf_mul(_value, 11) for _value in range(256)]
AES_MUL13 = [aes_gf_mul(_value, 13) for _value in range(256)]
AES_MUL14 = [aes_gf_mul(_value, 14) for _value in range(256)]


def read_encryption_key(project_root):
    with open(os.path.join(project_root, "FoundationEngine", "Prelude.h"), 'r', encoding='utf-8', errors='ignore') as f:
        match = re.search(r'#define\s+SC_ENCRYPTION_KEY_SEED\s+"([^"]+)"', f.read())
    if not match:
        return None
    return hashlib.sha256(match.group(1).encode("utf-8")).digest()


def aes256_expand_key(key):
    words = [list(key[4 * index:4 * index + 4]) for index in range(8)]
    rcon = 1
    for index in range(8, 60):
        temp = list(words[index - 1])
        if index % 8 == 0:
            temp = [AES_SBOX[b] for b in temp[1:] + temp[:1]]
            temp[0] ^= rcon
            rcon = aes_gf_mul(rcon, 2)
        elif index % 8 == 4:
            temp = [AES_SBOX[b] for b in temp]
        words.append([words[index - 8][j] ^ temp[j] for j in range(4)])
    return [sum(words[4 * r:4 * r + 4], []) for r in range(15)]


def aes256_cbc_encrypt(key, data):
    round_keys = aes256_expand_key(key)

    padding = 16 - len(data) % 16
    data = data + bytes([padding]) * padding

    iv = os.urandom(16)
    previous = list(iv)
    output = bytearray(iv)
    for offset in range(0, len(data), 16):
        state = [b ^ p ^ k for b, p, k in zip(data[offset:offset + 16], previous, round_keys[0])]
        for round_index in range(1, 15):
            state = [AES_SBOX[b] for b in state]
            state = [state[r + 4 * ((c + r) % 4)] for c in range(4) for r in range(4)]
            if round_index < 14:
                mixed = []
                for c in range(4):
                    a0, a1, a2, a3 = state[4 * c:4 * c + 4]
                    mixed += [
                        AES_MUL2[a0] ^ AES_MUL3[a1] ^ a2 ^ a3,
                        a0 ^ AES_MUL2[a1] ^ AES_MUL3[a2] ^ a3,
                        a0 ^ a1 ^ AES_MUL2[a2] ^ AES_MUL3[a3],
                        AES_MUL3[a0] ^ a1 ^ a2 ^ AES_MUL2[a3],
                    ]
                state = mixed
            state = [b ^ k for b, k in zip(state, round_keys[round_index])]
        output += bytes(state)
        previous = state

    return bytes(output)


def aes256_cbc_decrypt(key, data):
    if len(data) < 32 or (len(data) - 16) % 16 != 0:
        return None

    round_keys = aes256_expand_key(key)

    previous = list(data[:16])
    output = bytearray()
    for offset in range(16, len(data), 16):
        block = list(data[offset:offset + 16])
        state = [b ^ k for b, k in zip(block, round_keys[14])]
        for round_index in range(13, -1, -1):
            state = [state[r + 4 * ((c - r) % 4)] for c in range(4) for r in range(4)]
            state = [AES_INV_SBOX[b] for b in state]
            state = [b ^ k for b, k in zip(state, round_keys[round_index])]
            if round_index > 0:
                mixed = []
                for c in range(4):
                    a0, a1, a2, a3 = state[4 * c:4 * c + 4]
                    mixed += [
                        AES_MUL14[a0] ^ AES_MUL11[a1] ^ AES_MUL13[a2] ^ AES_MUL9[a3],
                        AES_MUL9[a0] ^ AES_MUL14[a1] ^ AES_MUL11[a2] ^ AES_MUL13[a3],
                        AES_MUL13[a0] ^ AES_MUL9[a1] ^ AES_MUL14[a2] ^ AES_MUL11[a3],
                        AES_MUL11[a0] ^ AES_MUL13[a1] ^ AES_MUL9[a2] ^ AES_MUL14[a3],
                    ]
                state = mixed
        output += bytes(s ^ v for s, v in zip(state, previous))
        previous = block

    padding = output[-1]
    if padding < 1 or padding > 16 or output[-padding:] != bytes([padding]) * padding:
        return None
    return bytes(output[:-padding])


def build_guid_map(key, project_root, user_project_root):
    """
    UserProject 以下を走査し、.meta を持つ全ファイルについて
    guid -> 実ファイルパス のマップと、
    プロジェクトルート相対パス（スラッシュ区切り） -> guid のマップを構築する。
    .meta は AssetMeta::Serialize がタグ付きバイナリ形式(BinaryArchive.h参照)
    で書き出し、BinaryOutputArchive::Write が [IV 16バイト][AES-256-CBC 暗号文]
    として保存している(鍵は SC_ENCRYPTION_KEY_SEED の SHA-256)。
    aes256_cbc_decrypt で復号してから、read_tagged_binary_fields で
    フィールドIDから "guid" の4バイトを引く。
    後者は ResourceCache::Find() が asset.path_（プロジェクトルート
    相対、フォワードスラッシュ）に対して完全一致で引く仕組みと同じ形。
    """
    guid_to_path = {}
    relpath_to_guid = {}

    for root, _, files in os.walk(user_project_root):
        for file in files:
            if file.endswith(".meta"):
                continue

            ext = os.path.splitext(file)[1].lower()
            if ext in NO_META_EXTENSIONS:
                continue

            full_path = os.path.join(root, file)
            meta_path = full_path + ".meta"
            if not os.path.exists(meta_path):
                continue

            try:
                with open(meta_path, 'rb') as f:
                    raw = aes256_cbc_decrypt(key, f.read())
                if raw is None:
                    print(f"警告: .meta の復号に失敗しました: {meta_path}")
                    continue
                guid_bytes = read_tagged_binary_fields(raw).get(GUID_FIELD_ID)
                if guid_bytes is None or len(guid_bytes) != 4:
                    continue
                guid = struct.unpack('<I', guid_bytes)[0]
            except Exception:
                continue

            if guid in guid_to_path:
                print(f"[ERROR] Duplicate GUID: {guid}")
                print(f"  Existing: {guid_to_path[guid]}")
                print(f"  New:      {full_path}")

            guid_to_path[guid] = full_path

            rel_path = os.path.relpath(full_path, project_root).replace('\\', '/')
            relpath_to_guid[rel_path] = guid

    return guid_to_path, relpath_to_guid


def strip_comments(content):
    """
    C++ の // 行コメント / * ブロックコメントだけを除去する
    （文字列/文字リテラルの中身はそのまま残す）。コメント中に書かれた
    Asset パス文字列が Scene::AssetID(...) 呼び出しとして誤検出される
    のを防ぐための前処理。行番号がずれないよう、コメントも改行だけは
    残して除去する。
    """
    result = []
    i = 0
    n = len(content)

    in_string = False
    in_char = False
    in_line_comment = False
    in_block_comment = False

    while i < n:
        c = content[i]
        next_c = content[i + 1] if i + 1 < n else ''

        if in_line_comment:
            if c == '\n':
                in_line_comment = False
                result.append(c)
            i += 1
            continue

        if in_block_comment:
            if c == '*' and next_c == '/':
                in_block_comment = False
                i += 2
                continue
            if c == '\n':
                result.append(c)
            i += 1
            continue

        if in_string:
            result.append(c)
            if c == '\\' and i + 1 < n:
                result.append(next_c)
                i += 2
                continue
            if c == '"':
                in_string = False
            i += 1
            continue

        if in_char:
            result.append(c)
            if c == '\\' and i + 1 < n:
                result.append(next_c)
                i += 2
                continue
            if c == "'":
                in_char = False
            i += 1
            continue

        if c == '/' and next_c == '/':
            in_line_comment = True
            i += 2
            continue

        if c == '/' and next_c == '*':
            in_block_comment = True
            i += 2
            continue

        if c == '"':
            in_string = True
            result.append(c)
            i += 1
            continue

        if c == "'":
            in_char = True
            result.append(c)
            i += 1
            continue

        result.append(c)
        i += 1

    return ''.join(result)


class PackageContext:
    def __init__(self, project_root, output_dir, key):
        self.project_root = project_root
        self.user_project_root = os.path.join(project_root, "UserProject")
        self.output_dir = output_dir
        self.staging_dir = os.path.join(output_dir, "_Staging")
        self.build_dir = os.path.join(project_root, "Runtime", "Build", "x64", "Release", "Cplusplus")
        self.key = key
        self.guid_to_path, self.relpath_to_guid = build_guid_map(key, project_root, self.user_project_root)
        self.filename_to_guids = {}
        for rel_path, guid in self.relpath_to_guid.items():
            self.filename_to_guids.setdefault(os.path.basename(rel_path), set()).add(guid)
        self.payload_map = Payload.collect_payload_field_names(project_root)
        self.store = {}

    @staticmethod
    def fnv1a(name):
        return fnv1a(name)

    @staticmethod
    def fields(raw):
        return read_tagged_binary_fields(raw)

    @staticmethod
    def record(name, payload):
        return struct.pack('<II', fnv1a(name), len(payload)) + payload

    @staticmethod
    def strip_comments(content):
        return strip_comments(content)

    def encrypt(self, data):
        return aes256_cbc_encrypt(self.key, data)

    def decrypt(self, data):
        return aes256_cbc_decrypt(self.key, data)

    def read_archive(self, path):
        if not os.path.exists(path):
            return None
        with open(path, 'rb') as f:
            body = self.decrypt(f.read())
        return read_tagged_binary_fields(body) if body is not None else None

    def resolve(self, name):
        """
        ResourceCache::Find と同じ規則で name を Asset ID の集合へ解決する:
        まずプロジェクトルート相対パスとして完全一致、外れたらファイル名だけで一致。
        同名ファイルが複数ある場合、エンジンは最初に見つかった1つを使うが、
        どれになるかは静的に決まらないため全候補を返す（パッケージには全て入れる）。
        """
        name = name.replace('\\\\', '\\').replace('\\', '/')
        exact = self.relpath_to_guid.get(name)
        if exact:
            return {exact}
        return set(self.filename_to_guids.get(os.path.basename(name), ()))

    def place(self, path):
        return os.path.relpath(path, self.project_root), path

    def asset(self, path):
        files = [self.place(path)]
        meta_path = path + ".meta"
        if os.path.exists(meta_path):
            files.append(self.place(meta_path))
        return files


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace", line_buffering=True)

    if len(sys.argv) < 3:
        print("使用法: RuntimePackager.py <projectRoot> <outputDir>")
        return 1

    project_root = os.path.abspath(sys.argv[1])
    output_dir = os.path.abspath(sys.argv[2])

    key = read_encryption_key(project_root)
    if key is None:
        print("FoundationEngine/Prelude.h から SC_ENCRYPTION_KEY_SEED を読み取れませんでした")
        return 1

    os.makedirs(output_dir, exist_ok=True)
    context = PackageContext(project_root, output_dir, key)
    shutil.rmtree(context.staging_dir, ignore_errors=True)
    os.makedirs(context.staging_dir)

    files = []
    ids = set()
    visited = set()

    def normalize(path):
        return os.path.normcase(os.path.normpath(path))

    def run(executor, jobs):
        futures = {executor.submit(module.collect, path, context): path for module, path in jobs}
        for done_count, future in enumerate(concurrent.futures.as_completed(futures)):
            result = future.result()
            print(f"@@detail {done_count + 1} {len(jobs)} {os.path.basename(futures[future])}")
            if result is None:
                return False
            files.extend(result[0])
            ids.update(result[1])
        return True

    def package():
        seeds = []
        for module in COLLECTS:
            paths = module.seed(context)
            if paths is None:
                return False
            if paths:
                seeds.append((module, paths))

        step_count = len(seeds) + 2
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as executor:
            for step_index, (module, paths) in enumerate(seeds):
                print(f"@@progress {step_index + 1} {step_count} {module.LABEL}")
                jobs = []
                for path in paths:
                    if normalize(path) in visited:
                        continue
                    visited.add(normalize(path))
                    jobs.append((module, path))
                if not run(executor, jobs):
                    return False
                print(f"{module.LABEL} 完了 ({len(jobs)} 件)")

            print(f"@@progress {step_count - 1} {step_count} 参照 Asset")
            modules_by_extension = {extension: module for module in COLLECTS for extension in module.EXTENSIONS}
            traced = set()
            asset_count = 0
            while True:
                jobs = []
                for asset_id in ids - traced:
                    traced.add(asset_id)
                    path = context.guid_to_path.get(asset_id)
                    if not path or normalize(path) in visited:
                        continue
                    visited.add(normalize(path))
                    module = modules_by_extension.get(os.path.splitext(path)[1].lower())
                    if module is None:
                        files.extend(context.asset(path))
                        asset_count += 1
                        continue
                    jobs.append((module, path))
                if not jobs:
                    break
                if not run(executor, jobs):
                    return False
                asset_count += len(jobs)
            print(f"参照 Asset 完了 ({asset_count} 件)")

        print(f"@@progress {step_count} {step_count} コピー")
        destinations = {}
        for destination, source in files:
            existing = destinations.get(normalize(destination))
            if existing and normalize(existing[1]) != normalize(source):
                print(f"同梱先が衝突しています: {destination} ← {existing[1]}, {source}")
                return False
            destinations[normalize(destination)] = (destination, source)
        for index, (destination, source) in enumerate(destinations.values()):
            target = os.path.join(output_dir, destination)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            shutil.copy2(source, target)
            print(f"@@detail {index + 1} {len(destinations)} {os.path.basename(destination)}")
        print(f"コピー完了 ({len(destinations)} 件)")
        return True

    succeeded = package()
    shutil.rmtree(context.staging_dir, ignore_errors=True)
    if not succeeded:
        print("パッケージを中断しました")
        return 1

    print(f"パッケージ完了: {output_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

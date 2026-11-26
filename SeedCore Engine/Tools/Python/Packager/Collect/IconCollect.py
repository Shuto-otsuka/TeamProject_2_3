import os
import re
import struct
import shutil
import ctypes
from ctypes import wintypes

LABEL = "アイコン埋め込み"

EXTENSIONS = set()


def read_executable_name(context):
    """
    GameConfig.scg の "executableName"（Editor の ConfigPanel「ゲーム設定」で
    入力した、プレイヤーが起動するランチャー exe の名前）を、Windows の
    ファイル名として使える形にして返す。使えない文字と制御文字を除き、
    末尾のドット/空白と、入力されていれば ".exe" を落とす。空、または
    予約デバイス名(CON, NUL, COM1 など)になった場合は "Launcher" を返す。
    """
    name = ""
    fields = context.read_archive(os.path.join(context.user_project_root, "Assets", "Config", "GameConfig.scg"))
    payload = fields.get(context.fnv1a("executableName")) if fields else None
    if payload:
        name = payload.decode("utf-8", "replace")

    name = re.sub(r'[\\/:*?"<>|\x00-\x1f]', '', name).strip()
    if name.lower().endswith(".exe"):
        name = name[:-4]
    name = name.rstrip(". ")

    reserved = {"con", "prn", "aux", "nul"} | {f"com{index}" for index in range(1, 10)} | {f"lpt{index}" for index in range(1, 10)}
    if not name or name.split(".")[0].lower() in reserved:
        return "Launcher"
    return name


def read_application_icon(context):
    """
    Editor の ConfigPanel「アイコン設定」で選ばれたアイコン（IconConfig が
    UserProject/Assets/Config/IconBindings.scg の "icon" フィールドに .ico を
    まるごと保存したもの）を返す。未指定ならエンジン既定の
    Runtime/Logo/SeedCore.ico を返す。
    """
    fields = context.read_archive(os.path.join(context.user_project_root, "Assets", "Config", "IconBindings.scg"))
    payload = fields.get(context.fnv1a("icon")) if fields else None
    if payload and len(payload) >= 4:
        count = struct.unpack_from('<I', payload, 0)[0]
        if count > 0 and len(payload) == 4 + count:
            return payload[4:], "IconBindings.scg"

    with open(os.path.join(context.project_root, "Runtime", "Logo", "SeedCore.ico"), 'rb') as f:
        return f.read(), "デフォルト (Runtime/Logo/SeedCore.ico)"


def embed_application_icon(exe_path, icon):
    """
    .ico の各画像を RT_ICON (ID 1..N) として、それらをまとめる
    RT_GROUP_ICON (ID 1) と一緒に exe へ書き込む。エクスプローラーは最小 ID の
    グループアイコンを exe のアイコンとして表示し、Runtime の Window は
    LoadImage(自身の exe, ID 1) で同じものをウィンドウ/タスクバーに使う。
    .ico 内の ICONDIRENTRY (16 バイト、末尾が画像オフセット) を、リソース用の
    GRPICONDIRENTRY (14 バイト、末尾が RT_ICON の ID) に詰め替える。
    """
    reserved, icon_type, count = struct.unpack_from('<HHH', icon, 0)
    if reserved != 0 or icon_type != 1 or count == 0 or len(icon) < 6 + count * 16:
        print(f"アイコンの形式が不正です: {exe_path}")
        return False

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
    kernel32.BeginUpdateResourceW.restype = wintypes.HANDLE
    kernel32.UpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPVOID, wintypes.WORD, wintypes.LPVOID, wintypes.DWORD]
    kernel32.UpdateResourceW.restype = wintypes.BOOL
    kernel32.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]
    kernel32.EndUpdateResourceW.restype = wintypes.BOOL

    rt_icon = 3
    rt_group_icon = 14
    language_neutral = 0

    handle = kernel32.BeginUpdateResourceW(exe_path, False)
    if not handle:
        print(f"exe のリソース更新を開始できませんでした ({ctypes.get_last_error()}): {exe_path}")
        return False

    group = struct.pack('<HHH', 0, 1, count)
    for image_index in range(count):
        width, height, color_count, entry_reserved, planes, bit_count, size, offset = struct.unpack_from('<BBBBHHII', icon, 6 + image_index * 16)
        image = ctypes.create_string_buffer(icon[offset:offset + size], size)
        if not kernel32.UpdateResourceW(handle, ctypes.c_void_p(rt_icon), ctypes.c_void_p(image_index + 1), language_neutral, image, size):
            print(f"アイコン画像を書き込めませんでした ({ctypes.get_last_error()}): {exe_path}")
            kernel32.EndUpdateResourceW(handle, True)
            return False
        group += struct.pack('<BBBBHHIH', width, height, color_count, entry_reserved, planes, bit_count, size, image_index + 1)

    group_buffer = ctypes.create_string_buffer(group, len(group))
    if not kernel32.UpdateResourceW(handle, ctypes.c_void_p(rt_group_icon), ctypes.c_void_p(1), language_neutral, group_buffer, len(group)):
        print(f"グループアイコンを書き込めませんでした ({ctypes.get_last_error()}): {exe_path}")
        kernel32.EndUpdateResourceW(handle, True)
        return False

    if not kernel32.EndUpdateResourceW(handle, False):
        print(f"exe のリソース更新を確定できませんでした ({ctypes.get_last_error()}): {exe_path}")
        return False

    return True


def seed(context):
    paths = [os.path.join(context.build_dir, "Launcher.exe"), os.path.join(context.build_dir, "Runtime.exe")]
    for path in paths:
        if not os.path.exists(path):
            print(f"{os.path.basename(path)} が見つかりません: {path}")
            return None

    icon, icon_source = read_application_icon(context)
    print(f"アイコン: {icon_source}")
    context.store["icon"] = (icon, f"{read_executable_name(context)}.exe")
    return paths


def collect(path, context):
    icon, launcher_name = context.store["icon"]
    destination = launcher_name if os.path.basename(path).lower() == "launcher.exe" else os.path.join("Plugins", "Runtime.exe")

    staged_path = os.path.join(context.staging_dir, "Icon", os.path.basename(path))
    os.makedirs(os.path.dirname(staged_path), exist_ok=True)
    shutil.copy2(path, staged_path)
    if not embed_application_icon(staged_path, icon):
        return None
    return [(destination, staged_path)], set()

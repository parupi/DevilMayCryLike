import os
import bpy # type: ignore
from ..operators.export_scene import MYADDON_OT_export_scene

# Enum items用にPythonが解放してしまわないよう参照を保持しておく(Blenderの既知の注意事項)
_ground_model_items_cache = []

def get_project_root() -> str:
    """
    ゲームプロジェクト(リポジトリ)のルートフォルダを返す。見つからなければ空文字。
    1. アドオン設定(プリファレンス)の「プロジェクトルート」
    2. アドオンがリポジトリ内(Externals/level_editor)から直接ロードされている場合の自動判別
    の順で解決する。アドオンを Blender 側(addons_core等)へコピーして使う場合は 1 が必須。
    """
    # 1. アドオン設定のプロジェクトルート
    addon_pkg = __package__.split(".")[0]
    addon = bpy.context.preferences.addons.get(addon_pkg)
    if addon and getattr(addon, "preferences", None):
        root = getattr(addon.preferences, "project_root", "")
        if root:
            root = bpy.path.abspath(root)
            if os.path.isdir(os.path.join(root, "Resource", "Models")):
                return root

    # 2. リポジトリ内から直接ロードされている場合(menus -> level_editor -> Externals -> repo root)
    addon_menus_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(addon_menus_dir)))
    if os.path.isdir(os.path.join(repo_root, "Resource", "Models")):
        return repo_root

    return ""


def get_ground_models_dir() -> str:
    """Resource/Models の絶対パスを返す。プロジェクトルートが見つからなければ空文字"""
    root = get_project_root()
    if not root:
        return ""
    return os.path.join(root, "Resource", "Models")


# ゲーム(ModelLoader)が読み込める拡張子。優先度順（C++側の判別順と合わせる）
MODEL_EXTENSIONS = (".obj", ".gltf", ".fbx")


def find_model_file(model_name: str):
    """Resource/Models/<name>/<name>.(obj|gltf|fbx) の実ファイルパスを返す。無ければ None"""
    models_dir = get_ground_models_dir()
    for ext in MODEL_EXTENSIONS:
        path = os.path.join(models_dir, model_name, model_name + ext)
        if os.path.isfile(path):
            return path
    return None


def get_ground_model_items(self, context):
    """Resource/Models 配下の <name>/<name>.(obj|gltf|fbx) を持つフォルダを列挙し、選択できるモデル一覧を返す"""
    global _ground_model_items_cache

    items = []
    models_dir = get_ground_models_dir()
    if os.path.isdir(models_dir):
        for entry in sorted(os.listdir(models_dir)):
            entry_dir = os.path.join(models_dir, entry)
            for ext in MODEL_EXTENSIONS:
                if os.path.isfile(os.path.join(entry_dir, entry + ext)):
                    items.append((entry, entry, f"{entry}{ext} を使用して生成"))
                    break

    if not items:
        items = [("Cube", "Cube", "")]

    _ground_model_items_cache = items
    return _ground_model_items_cache


def import_model_preview(context, model_name: str):
    """
    Resource/Models のモデルを実際にインポートしてプレビュー用オブジェクトとして返す。
    複数メッシュは1つに結合し、アーマチュアなどの余計なオブジェクトは削除する。
    読み込めなかった場合は None を返す（呼び出し側でキューブ等にフォールバックする）。
    """
    path = find_model_file(model_name)
    if not path:
        return None

    ext = os.path.splitext(path)[1].lower()
    try:
        bpy.ops.object.select_all(action='DESELECT')
        if ext == ".fbx":
            bpy.ops.import_scene.fbx(filepath=path)
        elif ext == ".obj":
            # Blender 4.x は wm.obj_import、3.x は import_scene.obj
            if hasattr(bpy.ops.wm, "obj_import"):
                bpy.ops.wm.obj_import(filepath=path)
            else:
                bpy.ops.import_scene.obj(filepath=path)
        elif ext == ".gltf":
            bpy.ops.import_scene.gltf(filepath=path)
        else:
            return None
    except Exception as e:
        print(f"[level_editor] モデルのインポートに失敗しました: {path} ({e})")
        return None

    imported = list(context.selected_objects)
    if not imported:
        return None

    # プレビューに不要なオブジェクト（アーマチュア・空オブジェクトなど）は削除する
    meshes = [o for o in imported if o.type == 'MESH']
    for o in imported:
        if o.type != 'MESH':
            bpy.data.objects.remove(o, do_unlink=True)

    if not meshes:
        return None

    # 複数メッシュは1オブジェクトに結合する（エクスポート時に1オブジェクトとして出すため）
    if len(meshes) > 1:
        bpy.ops.object.select_all(action='DESELECT')
        for m in meshes:
            m.select_set(True)
        context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()

    new_obj = meshes[0]
    # 親（削除済みのアーマチュア等）が残っているとトランスフォームが壊れるので外す
    new_obj.parent = None
    bpy.ops.object.select_all(action='DESELECT')
    new_obj.select_set(True)
    context.view_layer.objects.active = new_obj
    return new_obj


def ensure_game_loadable_model(context, model_name: str):
    """
    ゲーム側の assimp ビルドは FBX を読めない（OBJ/glTFのみ対応）ため、
    .fbx しか無いモデルはインポート結果を同じフォルダに .obj として書き出しておく。
    ゲームは .obj を優先して読むので、これで FBX モデルもそのまま配置できる。
    呼び出し時点で変換対象のオブジェクトが選択されている前提（import_model_preview直後）。
    """
    source_path = find_model_file(model_name)
    if not source_path or not source_path.lower().endswith(".fbx"):
        return  # .obj/.gltf が既にあるなら何もしない

    obj_path = os.path.join(os.path.dirname(source_path), model_name + ".obj")
    if os.path.isfile(obj_path):
        return  # 変換済み

    # エンジンはOBJ読み込み時にXを反転するため、そのまま書き出すと
    # メッシュがBlenderの見た目から水平180°回った向きでゲームに出る。
    # ここで180°回してから書き出して相殺し、Blenderで見えている向きのままゲームに出す。
    # （検証済み: Blender(bx,by,bz) → エンジン(bx,bz,by) となり配置座標の変換と一致する）
    import math
    from mathutils import Matrix

    target = context.active_object
    original_matrix = target.matrix_world.copy()
    target.matrix_world = Matrix.Rotation(math.pi, 4, 'Z') @ target.matrix_world

    try:
        # path_mode='COPY': テクスチャをモデルフォルダにコピーして相対パスでmtlに書く
        bpy.ops.wm.obj_export(
            filepath=obj_path,
            export_selected_objects=True,
            path_mode='COPY',
        )
        _sanitize_mtl(obj_path)
        print(f"[level_editor] ゲーム用に .obj へ変換しました: {obj_path}")
    except Exception as e:
        print(f"[level_editor] .obj 変換に失敗しました: {obj_path} ({e})")
    finally:
        # プレビュー用オブジェクトの向きを元に戻す
        target.matrix_world = original_matrix


def _sanitize_mtl(obj_path: str):
    """
    mtl から実体の無いテクスチャ参照を取り除く。
    画像ファイルを持たないマテリアルがあると Blender が「map_Kd .」のような
    無効な行を書き出し、ゲーム側のテクスチャ読み込みが失敗するため。
    """
    mtl_path = os.path.splitext(obj_path)[0] + ".mtl"
    if not os.path.isfile(mtl_path):
        return

    mtl_dir = os.path.dirname(mtl_path)
    kept_lines = []
    removed = 0

    with open(mtl_path, "r", encoding="utf-8") as f:
        for line in f:
            stripped = line.strip()
            if stripped.startswith("map_"):
                parts = stripped.split(None, 1)
                tex = parts[1].strip() if len(parts) > 1 else ""
                # 空・"." や実在しないファイルへの参照は削除
                if not tex or tex == "." or not os.path.isfile(os.path.join(mtl_dir, tex)):
                    removed += 1
                    continue
            kept_lines.append(line)

    if removed > 0:
        with open(mtl_path, "w", encoding="utf-8") as f:
            f.writelines(kept_lines)
        print(f"[level_editor] mtl から無効なテクスチャ参照を {removed} 行削除しました: {mtl_path}")


def get_unique_name(base_name: str) -> str:
    """既存のオブジェクト名と被らない名前を返す"""
    existing_names = {obj.name for obj in bpy.data.objects}
    if base_name not in existing_names:
        return base_name

    # base_name2, base_name3... と付けていく
    counter = 2
    while f"{base_name}{counter}" in existing_names:
        counter += 1
    return f"{base_name}{counter}"


class MYADDON_OT_add_object(bpy.types.Operator):
    """指定した種類のオブジェクトを追加"""
    bl_idname = "myaddon.add_object"
    bl_label = "Add Object"
    bl_description = "オブジェクトを追加します"

    object_type: bpy.props.EnumProperty(
        name="Object Type",
        items=[
            ('PLAYER', "Player", "プレイヤーを生成"),
            ('TUTORIAL_DUMMY', "TutorialDummy", "チュートリアル用の敵(練習台)を生成"),
            ('GRUNT_MELEE', "GruntMelee", "近接攻撃型の敵を生成"),
            ('BOSS_KNIGHT', "BossKnight", "ボス敵を生成"),
            ('GROUND', "Ground", "地面を生成"),
            ('PROP', "Prop", "小物(装飾オブジェクト)を生成"),
            ('POINT_LIGHT', "PointLight", "ポイントライトを生成"),
            ('EVENT_ENEMY_SPAWN', "敵出現イベント", "敵出現イベントを生成"),
            ('EVENT_FORCE_BATTLE', "強制戦闘イベント", "強制戦闘イベントを生成"),
            ('EVENT_CLEAR', "クリアイベント", "クリアイベントを生成"),
            ('EVENT_BOSS_SPAWN', "ボス出現イベント", "ボス出現イベント(カメラアップ演出)を生成"),
        ]
    ) # type: ignore

    model_name: bpy.props.StringProperty(
        name="Model Name",
        description="Groundが使用するモデル名(file_nameに設定される。Resource/Models/<名前>/<名前>.obj に対応)",
        default="Cube"
    ) # type: ignore

    def execute(self, context):
        model_base_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "models")

        new_obj = None

        if self.object_type == 'PLAYER':
            model_path = os.path.join(model_base_dir, "Player", "Player.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "Player"
            new_obj.name = get_unique_name("Player")

        elif self.object_type == 'TUTORIAL_DUMMY':
            # 配置プレビュー用にHellKainaのモデルを流用（ゲーム内モデルはC++クラス側で決まる）
            model_path = os.path.join(model_base_dir, "Enemy", "HellKaina.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "TutorialDummy"
            new_obj.name = get_unique_name("TutorialDummy")

        elif self.object_type == 'GRUNT_MELEE':
            # 配置プレビュー用にHellKainaのモデルを流用（ゲーム内モデルはC++クラス側で決まる）
            model_path = os.path.join(model_base_dir, "Enemy", "HellKaina.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "GruntMelee"
            new_obj.name = get_unique_name("GruntMelee")

        elif self.object_type == 'BOSS_KNIGHT':
            # 配置プレビュー用にHellKainaのモデルを流用（ゲーム内モデルはC++クラス側で決まる）
            model_path = os.path.join(model_base_dir, "Enemy", "HellKaina.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "BossKnight"
            new_obj.name = get_unique_name("BossKnight")

        elif self.object_type == 'GROUND':
            # 実際のモデル(.obj/.gltf/.fbx)をインポートしてプレビュー表示する。
            # 読み込めなかった場合はキューブで代用（file_nameは設定されるのでゲーム内では正しく表示される）
            new_obj = import_model_preview(context, self.model_name)
            if new_obj is None:
                bpy.ops.mesh.primitive_cube_add(size = 1)
                new_obj = context.active_object
            else:
                # .fbx しか無いモデルはゲームが読めるように .obj へ自動変換する
                ensure_game_loadable_model(context, self.model_name)
            new_obj["class_name"] = "Ground"
            new_obj["file_name"] = self.model_name
            new_obj.name = get_unique_name("Ground")

        elif self.object_type == 'PROP':
            # 実際のモデル(.obj/.gltf/.fbx)をインポートしてプレビュー表示する
            new_obj = import_model_preview(context, self.model_name)
            if new_obj is None:
                bpy.ops.mesh.primitive_cube_add(size = 1)
                new_obj = context.active_object
            else:
                # .fbx しか無いモデルはゲームが読めるように .obj へ自動変換する
                ensure_game_loadable_model(context, self.model_name)
            new_obj["class_name"] = "Prop"
            new_obj["file_name"] = self.model_name
            new_obj.name = get_unique_name(self.model_name)

        elif self.object_type == 'POINT_LIGHT':
            # 実際のBlenderライトを配置する（ビューポートで光り方をプレビューできる）
            bpy.ops.object.light_add(type='POINT')
            new_obj = context.active_object
            new_obj["class_name"] = "PointLight"
            new_obj.name = get_unique_name("PointLight")

        elif self.object_type == 'EVENT_ENEMY_SPAWN':
            bpy.ops.object.empty_add()
            new_obj = context.active_object
            new_obj["class_name"] = "Event_EnemySpawn"
            new_obj.name = get_unique_name("Event_EnemySpawn")

        elif self.object_type == 'EVENT_FORCE_BATTLE':
            bpy.ops.object.empty_add()
            new_obj = context.active_object
            new_obj["class_name"] = "Event_ForceBattle"
            new_obj.name = get_unique_name("Event_ForceBattle")

        elif self.object_type == 'EVENT_CLEAR':
            bpy.ops.object.empty_add()
            new_obj = context.active_object
            new_obj["class_name"] = "Event_Clear"
            new_obj.name = get_unique_name("Event_Clear")

        elif self.object_type == 'EVENT_BOSS_SPAWN':
            bpy.ops.object.empty_add()
            new_obj = context.active_object
            new_obj["class_name"] = "Event_BossSpawn"
            new_obj.name = get_unique_name("Event_BossSpawn")

        return {'FINISHED'}


# ==== Enemyサブメニュー ====
class TOPBAR_MT_enemy_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_enemy_menu"
    bl_label = "Enemy生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="TutorialDummy").object_type = 'TUTORIAL_DUMMY'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="GruntMelee").object_type = 'GRUNT_MELEE'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="BossKnight").object_type = 'BOSS_KNIGHT'


# ==== Eventサブメニュー ====
class TOPBAR_MT_event_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_event_menu"
    bl_label = "Event生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="敵出現").object_type = 'EVENT_ENEMY_SPAWN'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="強制戦闘").object_type = 'EVENT_FORCE_BATTLE'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="クリア").object_type = 'EVENT_CLEAR'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="ボス出現").object_type = 'EVENT_BOSS_SPAWN'


def _draw_models_dir_error(layout):
    """Resource/Models が見つからないときの案内を描画する"""
    layout.label(text="Resource/Models が見つかりません", icon='ERROR')
    layout.label(text="編集 > プリファレンス > アドオン > レベルエディタ で")
    layout.label(text="プロジェクトルート(リポジトリの場所)を設定してください")


# ==== Groundサブメニュー ====
class TOPBAR_MT_ground_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_ground_menu"
    bl_label = "Ground生成"

    def draw(self, context):
        layout = self.layout
        if not get_ground_models_dir():
            _draw_models_dir_error(layout)
            return
        for identifier, name, _description in get_ground_model_items(None, context):
            op = layout.operator(MYADDON_OT_add_object.bl_idname, text=name)
            op.object_type = 'GROUND'
            op.model_name = identifier


# ==== Propサブメニュー ====
class TOPBAR_MT_prop_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_prop_menu"
    bl_label = "Prop生成"

    def draw(self, context):
        layout = self.layout
        if not get_ground_models_dir():
            _draw_models_dir_error(layout)
            return
        # Resource/Models 配下のモデル(.obj/.gltf/.fbx)から選ぶ（Groundと同じ一覧）
        for identifier, name, _description in get_ground_model_items(None, context):
            op = layout.operator(MYADDON_OT_add_object.bl_idname, text=name)
            op.object_type = 'PROP'
            op.model_name = identifier


# ==== Object生成サブメニュー ====
class TOPBAR_MT_my_object_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_object_menu"
    bl_label = "Object生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="Player").object_type = 'PLAYER'
        layout.menu(TOPBAR_MT_enemy_menu.bl_idname, icon='ARMATURE_DATA')
        layout.menu(TOPBAR_MT_ground_menu.bl_idname, icon='MESH_CUBE')
        layout.menu(TOPBAR_MT_prop_menu.bl_idname, icon='MESH_ICOSPHERE')
        layout.operator(MYADDON_OT_add_object.bl_idname, text="PointLight", icon='LIGHT_POINT').object_type = 'POINT_LIGHT'
        layout.menu(TOPBAR_MT_event_menu.bl_idname, icon='EXPORT')


# ==== 親メニュー ====
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"

    def draw(self, context):
        layout = self.layout
        # Object生成サブメニュー
        layout.menu(TOPBAR_MT_my_object_menu.bl_idname, icon='MESH_CUBE')
        # シーン出力
        layout.operator(
            MYADDON_OT_export_scene.bl_idname,
            text="シーンをJSON出力",
            icon='EXPORT'
        )

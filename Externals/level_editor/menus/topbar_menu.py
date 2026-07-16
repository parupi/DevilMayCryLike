import os
import bpy # type: ignore
from ..operators.export_scene import MYADDON_OT_export_scene

# Enum items用にPythonが解放してしまわないよう参照を保持しておく(Blenderの既知の注意事項)
_ground_model_items_cache = []

def get_ground_models_dir() -> str:
    """Resource/Models の絶対パスを返す(アドオンからリポジトリルート相対で解決)"""
    addon_menus_dir = os.path.dirname(os.path.abspath(__file__))          # .../Externals/level_editor/menus
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(addon_menus_dir)))  # .../menus -> level_editor -> Externals -> repo root
    return os.path.join(repo_root, "Resource", "Models")


def get_ground_model_items(self, context):
    """Resource/Models 配下の <name>/<name>.obj を持つフォルダを列挙し、Groundとして選択できるモデル一覧を返す"""
    global _ground_model_items_cache

    items = []
    models_dir = get_ground_models_dir()
    if os.path.isdir(models_dir):
        for entry in sorted(os.listdir(models_dir)):
            entry_dir = os.path.join(models_dir, entry)
            if os.path.isfile(os.path.join(entry_dir, entry + ".obj")):
                items.append((entry, entry, f"{entry} を使用してGroundを生成"))

    if not items:
        items = [("Cube", "Cube", "")]

    _ground_model_items_cache = items
    return _ground_model_items_cache


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
            ('GROUND', "Ground", "地面を生成"),
            ('EVENT_ENEMY_SPAWN', "敵出現イベント", "敵出現イベントを生成"),
            ('EVENT_FORCE_BATTLE', "強制戦闘イベント", "強制戦闘イベントを生成"),
            ('EVENT_CLEAR', "クリアイベント", "クリアイベントを生成"),
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

        elif self.object_type == 'GROUND':
            bpy.ops.mesh.primitive_cube_add(size = 1)
            new_obj = context.active_object
            new_obj["class_name"] = "Ground"
            new_obj["file_name"] = self.model_name
            new_obj.name = get_unique_name("Ground")

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

        return {'FINISHED'}


# ==== Enemyサブメニュー ====
class TOPBAR_MT_enemy_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_enemy_menu"
    bl_label = "Enemy生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="TutorialDummy").object_type = 'TUTORIAL_DUMMY'


# ==== Eventサブメニュー ====
class TOPBAR_MT_event_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_event_menu"
    bl_label = "Event生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="敵出現").object_type = 'EVENT_ENEMY_SPAWN'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="強制戦闘").object_type = 'EVENT_FORCE_BATTLE'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="クリア").object_type = 'EVENT_CLEAR'


# ==== Groundサブメニュー ====
class TOPBAR_MT_ground_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_ground_menu"
    bl_label = "Ground生成"

    def draw(self, context):
        layout = self.layout
        for identifier, name, _description in get_ground_model_items(None, context):
            op = layout.operator(MYADDON_OT_add_object.bl_idname, text=name)
            op.object_type = 'GROUND'
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

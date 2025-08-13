import os
import bpy # type: ignore
import bpy.utils # type: ignore
from ..operators.export_scene import MYADDON_OT_export_scene


import os
import bpy # type: ignore

import os
import bpy # type: ignore

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
            ('HELLKAINA', "HellKaina", "ヘルカイナを生成"),
            ('GROUND', "Ground", "地面を生成"),
            ('EVENT_ENEMY_SPAWN', "敵出現イベント", "敵出現イベントを生成"),
            ('EVENT_FORCE_BATTLE', "強制戦闘イベント", "強制戦闘イベントを生成"),
            ('EVENT_CLEAR', "クリアイベント", "クリアイベントを生成"),
        ]
    ) # type: ignore

    def execute(self, context):
        model_base_dir = r"C:\Program Files\Blender Foundation\Blender 4.5\4.5\scripts\addons_core\level_editor\models"

        new_obj = None

        if self.object_type == 'PLAYER':
            model_path = os.path.join(model_base_dir, "Player", "Player.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "Player"
            new_obj.name = get_unique_name("Player")

        elif self.object_type == 'HELLKAINA':
            model_path = os.path.join(model_base_dir, "Enemy", "HellKaina.fbx")
            bpy.ops.import_scene.fbx(filepath=model_path)
            new_obj = context.selected_objects[0]
            new_obj["class_name"] = "HellKaina"
            new_obj.name = get_unique_name("HellKaina")

        elif self.object_type == 'GROUND':
            bpy.ops.mesh.primitive_cube_add(size = 1)
            new_obj = context.active_object
            new_obj["class_name"] = "Ground"
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
        layout.operator(MYADDON_OT_add_object.bl_idname, text="HellKaina").object_type = 'HELLKAINA'


# ==== Eventサブメニュー ====
class TOPBAR_MT_event_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_event_menu"
    bl_label = "Event生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="敵出現").object_type = 'EVENT_ENEMY_SPAWN'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="強制戦闘").object_type = 'EVENT_FORCE_BATTLE'
        layout.operator(MYADDON_OT_add_object.bl_idname, text="クリア").object_type = 'EVENT_CLEAR'


# ==== Object生成サブメニュー ====
class TOPBAR_MT_my_object_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_object_menu"
    bl_label = "Object生成"

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_add_object.bl_idname, text="Player").object_type = 'PLAYER'
        layout.menu(TOPBAR_MT_enemy_menu.bl_idname, icon='ARMATURE_DATA')
        layout.operator(MYADDON_OT_add_object.bl_idname, text="Ground").object_type = 'GROUND'
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

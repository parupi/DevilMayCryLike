import math
import bpy # type: ignore
import bpy_extras # type: ignore
import gpu # type: ignore
import gpu_extras.batch # type: ignore
import copy
import mathutils # type: ignore
import json

# ブレンダーに登録するアドオン情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Kawaguchi Haruki",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ",
    "warning": "",
    "support": "TESTING",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}

from .preferences import LevelEditorPreferences

from .operators.add_collider import MYADDON_OT_add_collider
from .operators.add_file_name import MYADDON_OT_add_file_name
from .operators.add_class_name import MYADDON_OT_add_class_name
from .operators.export_scene import MYADDON_OT_export_scene
#from .operators.load_model import MYADDON_OT_load_and_replace_model

from .menus.topbar_menu import (
    TOPBAR_MT_my_menu,
    TOPBAR_MT_my_object_menu,
    MYADDON_OT_add_object,
    TOPBAR_MT_enemy_menu,
    TOPBAR_MT_ground_menu,
    TOPBAR_MT_prop_menu,
    TOPBAR_MT_event_menu
)

from .operators.enemy_spawn_event import (
    EnemySpawnItem,
    EnemySpawnEventProperty,
    ENEMYSPAWN_UL_enemy_list,
    ENEMYSPAWN_OT_add_enemy,
    ENEMYSPAWN_OT_remove_enemy,
    ENEMYSPAWN_PT_panel,
)

from .operators.clear_event import(
    ClearConditionItem,
    ClearEventProperty,
    CLEAREVENT_UL_condition_list,
    CLEAREVENT_OT_add_condition,
    CLEAREVENT_OT_remove_condition,
    CLEAREVENT_PT_panel,
)

from .operators.force_battle_event import(
    ForceBattleEnemyItem,
    ForceBattleEventProperty,
    FORCEBATTLE_UL_enemy_list,
    FORCEBATTLE_OT_add_enemy,
    FORCEBATTLE_OT_remove_enemy,
    FORCEBATTLE_PT_panel,
)

from .operators.boss_spawn_event import(
    BossSpawnEventProperty,
    BOSSSPAWN_PT_panel,
)

from .operators.point_light import(
    PointLightProperty,
    POINTLIGHT_PT_panel,
)

from .operators.prop_object import(
    PropObjectProperty,
    PROP_PT_panel,
)

from .panels.level_editor_panel import OBJECT_PT_level_editor

from .draw.draw_collider import DrawCollider

from .menus.topbar_menu import TOPBAR_MT_my_menu

def draw_my_menu(self, context):
    self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

# アドオン有効化時コールバック
def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_editor_menus.append(draw_my_menu)

    bpy.types.Object.enemy_spawn_event = bpy.props.PointerProperty(type=EnemySpawnEventProperty)

    bpy.types.Object.clear_event = bpy.props.PointerProperty(type=ClearEventProperty)

    bpy.types.Object.force_battle_event = bpy.props.PointerProperty(type=ForceBattleEventProperty)

    bpy.types.Object.boss_spawn_event = bpy.props.PointerProperty(type=BossSpawnEventProperty)

    bpy.types.Object.point_light = bpy.props.PointerProperty(type=PointLightProperty)

    bpy.types.Object.prop_object = bpy.props.PointerProperty(type=PropObjectProperty)

    # 3Dビューに描画関数を追加
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(
        DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW"
    )

    # disabled をオブジェクトに追加
    bpy.types.Object.disabled = bpy.props.BoolProperty(
        name="Disabled",
        description="このオブジェクトを出力対象外にする",
        default=False
    )

    print("レベルエディタが有効化されました")
    

# アドオン無効化時コールバック
def unregister():
    bpy.types.TOPBAR_MT_editor_menus.remove(draw_my_menu)

    del bpy.types.Object.enemy_spawn_event

    del bpy.types.Object.clear_event

    del bpy.types.Object.force_battle_event

    del bpy.types.Object.boss_spawn_event

    del bpy.types.Object.point_light

    del bpy.types.Object.prop_object

    del bpy.types.Object.disabled

    bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")

    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

    print("レベルエディタが無効化されました")


# Blenderに登録するクラスリスト
classes = (
    # アドオン設定（プロジェクトルートの指定）
    LevelEditorPreferences,

    MYADDON_OT_add_file_name,
    MYADDON_OT_add_class_name,
    MYADDON_OT_add_collider,
    
    # Object生成関連
    TOPBAR_MT_enemy_menu,
    TOPBAR_MT_ground_menu,
    TOPBAR_MT_prop_menu,
    TOPBAR_MT_event_menu,

    EnemySpawnItem,
    EnemySpawnEventProperty,
    ENEMYSPAWN_UL_enemy_list,
    ENEMYSPAWN_OT_add_enemy,
    ENEMYSPAWN_OT_remove_enemy,
    ENEMYSPAWN_PT_panel,

    ClearConditionItem,
    ClearEventProperty,
    CLEAREVENT_UL_condition_list,
    CLEAREVENT_OT_add_condition,
    CLEAREVENT_OT_remove_condition,
    CLEAREVENT_PT_panel,

    ForceBattleEnemyItem,
    ForceBattleEventProperty,
    FORCEBATTLE_UL_enemy_list,
    FORCEBATTLE_OT_add_enemy,
    FORCEBATTLE_OT_remove_enemy,
    FORCEBATTLE_PT_panel,

    BossSpawnEventProperty,
    BOSSSPAWN_PT_panel,

    PointLightProperty,
    POINTLIGHT_PT_panel,

    PropObjectProperty,
    PROP_PT_panel,

    MYADDON_OT_add_object,
    TOPBAR_MT_my_object_menu,



    MYADDON_OT_export_scene,

    OBJECT_PT_level_editor,

    TOPBAR_MT_my_menu,
)

# テスト実行用コード
if __name__ == "__main__":
    register()
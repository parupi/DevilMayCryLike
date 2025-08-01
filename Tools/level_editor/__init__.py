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

from .operators.add_collider import MYADDON_OT_add_collider
from .operators.add_file_name import MYADDON_OT_add_file_name
from .operators.add_class_name import MYADDON_OT_add_class_name
from .operators.export_scene import MYADDON_OT_export_scene

from .panels.level_editor_panel import OBJECT_PT_level_editor # type: ignore

from .draw.draw_collider import DrawCollider

from .menus.topbar_menu import TOPBAR_MT_my_menu

from .spawn.spawn import MYADDON_OT_spawn_import_symbol

def draw_my_menu(self, context):
    self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

# アドオン有効化時コールバック
def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_editor_menus.append(draw_my_menu)



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

    del bpy.types.Object.disabled

    bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")

    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

    print("レベルエディタが無効化されました")


# Blenderに登録するクラスリスト
classes = (
    MYADDON_OT_add_file_name,
    MYADDON_OT_add_class_name,
    MYADDON_OT_add_collider,
    
    MYADDON_OT_export_scene,

    OBJECT_PT_level_editor,

    TOPBAR_MT_my_menu,

    MYADDON_OT_spawn_import_symbol,
)

# テスト実行用コード
if __name__ == "__main__":
    register()
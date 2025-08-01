import bpy # type: ignore
from ..operators.export_scene import MYADDON_OT_export_scene

class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"

    def draw(self, context):
        layout = self.layout
        #layout.label(text="Hello Menu")

        # シーン出力ボタン
        layout.operator(
            MYADDON_OT_export_scene.bl_idname,
            text="シーンをJSON出力",
            icon='EXPORT'
        )

        # 他のメニュー項目があるならそれも維持
        layout.operator("myaddon.spawn_import_symbol", text="シンボルを読み込んで配置")
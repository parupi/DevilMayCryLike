import bpy # type: ignore

class OBJECT_PT_myaddon_disabled_panel(bpy.types.Panel):
    bl_label = "オブジェクト無効化"
    bl_idname = "OBJECT_PT_myaddon_disabled_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "MyAddon"

    def draw(self, context):
        layout = self.layout
        obj = context.active_object

        if obj:
            layout.prop(obj, "myaddon_disabled", text="このオブジェクトを無効化", toggle=True)
            layout.operator("myaddon.toggle_object_disabled")
        else:
            layout.label(text="アクティブなオブジェクトがありません")
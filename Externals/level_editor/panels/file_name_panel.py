import bpy # type: ignore
from ..operators.add_file_name import MYADDON_OT_add_file_name

class OBJECT_PT_file_name(bpy.types.Panel):
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        obj = context.object
        layout = self.layout

        if "file_name" in obj:
            layout.prop(obj, '["file_name"]', text="FileName")
        else:
            layout.operator(MYADDON_OT_add_file_name.bl_idname)

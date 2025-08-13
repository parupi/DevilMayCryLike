import bpy # type: ignore
from ..operators.add_class_name import MYADDON_OT_add_class_name

class OBJECT_PT_class_name(bpy.types.Panel):
    bl_idname = "OBJECT_PT_class_name"
    bl_label = "ClassName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        obj = context.object
        layout = self.layout

        if "class_name" in obj:
            layout.prop(obj, '["class_name"]', text="ClassName")
        else:
            layout.operator(MYADDON_OT_add_class_name.bl_idname)

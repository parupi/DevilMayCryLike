import bpy # type: ignore
from ..operators.add_collider import MYADDON_OT_add_collider
from ..operators.add_file_name import MYADDON_OT_add_file_name
from ..operators.add_class_name import MYADDON_OT_add_class_name

class OBJECT_PT_level_editor(bpy.types.Panel):
    bl_idname = "OBJECT_PT_level_editor"
    bl_label = "Level Editor"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object

        # --- Class Name ---
        box = layout.box()
        box.label(text="Class Name")
        if "class_name" in obj:
            box.prop(obj, '["class_name"]', text="ClassName")
        else:
            box.operator(MYADDON_OT_add_class_name.bl_idname, text="Add Class Name")

        # --- Collider ---
        box = layout.box()
        box.label(text="Collider")
        if "collider" in obj:
            box.prop(obj, '["collider"]', text="Type")
            box.prop(obj, '["collider_center"]', text="Center")
            box.prop(obj, '["collider_size"]', text="Size")
        else:
            box.operator(MYADDON_OT_add_collider.bl_idname, text="Add Collider")

        # --- File Name ---
        box = layout.box()
        box.label(text="File Name")
        if "file_name" in obj:
            box.prop(obj, '["file_name"]', text="FileName")
        else:
            box.operator(MYADDON_OT_add_file_name.bl_idname, text="Add File Name")

        # --- Disabled ---
        box = layout.box()
        box.label(text="Disabled")
        box.prop(obj, "disabled", text="無効にする")
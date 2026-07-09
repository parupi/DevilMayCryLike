import bpy # type: ignore
from ..operators.add_collider import MYADDON_OT_add_collider
from ..operators.add_file_name import MYADDON_OT_add_file_name
from ..operators.add_class_name import MYADDON_OT_add_class_name
from ..operators.load_model import MYADDON_OT_load_and_replace_model

class OBJECT_PT_level_editor(bpy.types.Panel):
    bl_idname = "OBJECT_PT_level_editor"
    bl_label = "Level Editor"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object


        # --- Collider ---
        box = layout.box()
        box.label(text="Collider")
        if "collider" in obj:
            box.prop(obj, '["collider"]', text="Type")
            box.prop(obj, '["collider_center"]', text="Center")
            box.prop(obj, '["collider_size"]', text="Size")
        else:
            box.operator(MYADDON_OT_add_collider.bl_idname, text="Add Collider")


        # --- Disabled ---
        box = layout.box()
        box.label(text="Disabled")
        box.prop(obj, "disabled", text="無効にする")
        
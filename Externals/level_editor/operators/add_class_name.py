import bpy # type: ignore

class MYADDON_OT_add_class_name(bpy.types.Operator):
    bl_idname = "myaddon.add_class_name"
    bl_label = "ClassName 追加"
    bl_description = "['class_name'] のカスタムプロパティを追加します"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        obj = context.object

        if obj is None:
            self.report({'WARNING'}, "オブジェクトが選択されていません")
            return {'CANCELLED'}

        obj["class_name"] = ""
        self.report({'INFO'}, "'class_name' を追加しました")
        return {'FINISHED'}
    

    
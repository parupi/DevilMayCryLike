import bpy # type: ignore

class MYADDON_OT_add_file_name(bpy.types.Operator):
    bl_idname = "myaddon.add_file_name"
    bl_label = "FileName 追加"
    bl_description = "['file_name'] のカスタムプロパティを追加します"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        obj = context.object
        if obj is None:
            self.report({'WARNING'}, "オブジェクトが選択されていません")
            return {'CANCELLED'}

        obj["file_name"] = ""
        self.report({'INFO'}, "'file_name' を追加しました")
        return {'FINISHED'}

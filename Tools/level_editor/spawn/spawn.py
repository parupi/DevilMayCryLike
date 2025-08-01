import bpy
from bpy.types import Operator
from bpy.props import StringProperty
import os


class MYADDON_OT_spawn_import_symbol(Operator):
    bl_idname = "myaddon.spawn_import_symbol"
    bl_label = "Spawn Imported Symbol"
    bl_description = "指定されたファイルをインポートして3Dカーソル位置に配置します"
    bl_options = {'REGISTER', 'UNDO'}

    filepath: StringProperty(
        name="File Path",
        description="インポートするオブジェクトファイルのパス",
        maxlen=1024,
        subtype='FILE_PATH'
    ) # type: ignore

    def execute(self, context):
        # ファイルの拡張子を取得
        ext = os.path.splitext(self.filepath)[1].lower()

        # インポート前のオブジェクト一覧
        before = set(bpy.data.objects)

        try:
            if ext == ".obj":
                bpy.ops.import_scene.obj(filepath=self.filepath)
            elif ext == ".fbx":
                bpy.ops.import_scene.fbx(filepath=self.filepath)
            else:
                self.report({'ERROR'}, f"未対応のファイル形式です: {ext}")
                return {'CANCELLED'}
        except Exception as e:
            self.report({'ERROR'}, f"インポート失敗: {str(e)}")
            return {'CANCELLED'}

        # インポート後の新規オブジェクトを取得
        after = set(bpy.data.objects)
        new_objects = after - before

        # 配置先位置（3Dカーソル）
        cursor_loc = context.scene.cursor.location

        for obj in new_objects:
            obj.location = cursor_loc

        self.report({'INFO'}, f"{len(new_objects)} 個のオブジェクトを配置しました")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

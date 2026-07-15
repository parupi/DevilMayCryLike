import bpy
import bpy.ops
from bpy.props import StringProperty
from bpy.types import Operator, Panel


class MYADDON_OT_load_and_replace_model(Operator):
    """モデルを読み込んで現在のオブジェクトと差し替える"""
    bl_idname = "myaddon.load_and_replace_model"
    bl_label = "Load and Replace Model"
    bl_options = {'REGISTER', 'UNDO'}

    filepath: StringProperty(name="Model File", subtype='FILE_PATH')

    def execute(self, context):
        obj = context.object
        if not obj:
            self.report({'WARNING'}, "オブジェクトが選択されていません")
            return {'CANCELLED'}

        # モデル読み込み前の情報を保存
        original_name = obj.name
        original_location = obj.location.copy()
        original_rotation = obj.rotation_euler.copy()
        original_scale = obj.scale.copy()

        # モデルのインポート（ここでは .obj）
        bpy.ops.import_scene.obj(filepath=self.filepath)

        # 読み込まれたオブジェクト（複数でも対応）の取得
        imported_objs = [o for o in context.selected_objects if o.name != original_name]

        if not imported_objs:
            self.report({'ERROR'}, "モデルの読み込みに失敗しました")
            return {'CANCELLED'}

        # 元のオブジェクトを非表示にする（削除したい場合は .unlink() や .remove() も可）
        obj.hide_set(True)
        obj.hide_viewport = True

        # 最初のインポートオブジェクトをメインとして使う
        new_obj = imported_objs[0]
        new_obj.name = original_name
        new_obj.location = original_location
        new_obj.rotation_euler = original_rotation
        new_obj.scale = original_scale

        # 元のオブジェクトの代わりとして記録（必要に応じてカスタムプロパティに設定）
        new_obj["replaced"] = True

        self.report({'INFO'}, f"モデルを読み込み、{original_name} を差し替えました")
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}
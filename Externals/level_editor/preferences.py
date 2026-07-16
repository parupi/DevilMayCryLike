import os
import bpy  # type: ignore


class LevelEditorPreferences(bpy.types.AddonPreferences):
    """
    レベルエディタのアドオン設定。
    アドオンを Blender 側(addons_core など)にコピーして使う場合、
    ゲームプロジェクトの場所が分からなくなるため、ここでプロジェクトルートを指定する。
    """
    # アドオンのモジュール名（"level_editor"）と一致させる必要がある
    bl_idname = __package__

    project_root: bpy.props.StringProperty(
        name="プロジェクトルート",
        description="DevilMayCryLike リポジトリのルートフォルダ（中に Resource/Models がある場所）",
        subtype='DIR_PATH',
        default="",
    )  # type: ignore

    def draw(self, context):
        layout = self.layout
        layout.prop(self, "project_root")

        # 現在の解決結果を表示して、設定が正しいか一目で分かるようにする
        root = bpy.path.abspath(self.project_root) if self.project_root else ""
        models_dir = os.path.join(root, "Resource", "Models") if root else ""
        if models_dir and os.path.isdir(models_dir):
            layout.label(text=f"OK: {models_dir}", icon='CHECKMARK')
        else:
            layout.label(text="Resource/Models が見つかりません。リポジトリのルートフォルダを指定してください", icon='ERROR')
        layout.label(text="Ground/Prop のモデル一覧は <プロジェクトルート>/Resource/Models から読み込まれます")


classes = (
    LevelEditorPreferences,
)

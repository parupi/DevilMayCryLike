import bpy  # type: ignore


# === 小物(Prop)のプロパティ ===
class PropObjectProperty(bpy.types.PropertyGroup):
    light_enabled: bpy.props.BoolProperty(
        name="ライトを付ける",
        description="ランタンなど発光する小物用のポイントライトを付ける",
        default=False,
    )  # type: ignore
    light_color: bpy.props.FloatVectorProperty(
        name="ライトの色",
        subtype='COLOR',
        size=3,
        default=(1.0, 0.75, 0.4),  # 暖色（ランタン想定）
        min=0.0,
        max=1.0,
    )  # type: ignore
    light_offset: bpy.props.FloatVectorProperty(
        name="ライトのオフセット",
        description="オブジェクト原点からのローカルオフセット(Blender座標。Zが上)",
        subtype='TRANSLATION',
        size=3,
        default=(0.0, 0.0, 1.0),
    )  # type: ignore
    light_intensity: bpy.props.FloatProperty(
        name="明るさ",
        default=1.5,
        min=0.0,
        max=100.0,
    )  # type: ignore
    light_radius: bpy.props.FloatProperty(
        name="届く距離",
        default=8.0,
        min=0.0,
        max=1000.0,
    )  # type: ignore
    light_decay: bpy.props.FloatProperty(
        name="減衰",
        default=1.0,
        min=0.0,
        max=10.0,
    )  # type: ignore


# === パネル ===
class PROP_PT_panel(bpy.types.Panel):
    bl_label = "小物(Prop)設定"
    bl_idname = "PROP_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "Prop"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.prop_object

        layout.label(text=f"モデル: {obj.get('file_name', '(未設定)')}")
        layout.label(text="※ 当たり判定が必要ならコライダーを追加してください")

        layout.separator()
        layout.prop(props, "light_enabled")

        if props.light_enabled:
            box = layout.box()
            box.prop(props, "light_color")
            box.prop(props, "light_offset")
            box.prop(props, "light_intensity")
            box.prop(props, "light_radius")
            box.prop(props, "light_decay")


# === register ===
classes = (
    PropObjectProperty,
    PROP_PT_panel,
)

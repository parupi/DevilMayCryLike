import bpy  # type: ignore


# === ポイントライトのプロパティ ===
# 色は Blender のライトデータ(obj.data.color)を使う（ビューポートのプレビューと一致させるため）
class PointLightProperty(bpy.types.PropertyGroup):
    intensity: bpy.props.FloatProperty(
        name="Intensity",
        description="ライトの明るさ",
        default=1.5,
        min=0.0,
        max=100.0,
    )  # type: ignore
    radius: bpy.props.FloatProperty(
        name="Radius",
        description="ライトが届く距離[m]",
        default=10.0,
        min=0.0,
        max=1000.0,
    )  # type: ignore
    decay: bpy.props.FloatProperty(
        name="Decay",
        description="距離減衰の強さ",
        default=1.0,
        min=0.0,
        max=10.0,
    )  # type: ignore


# === パネル ===
class POINTLIGHT_PT_panel(bpy.types.Panel):
    bl_label = "ポイントライト設定"
    bl_idname = "POINTLIGHT_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "PointLight"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.point_light

        # 色は Blender ライトの色をそのまま使う（ビューポートのプレビューと一致する）
        if obj.type == 'LIGHT':
            layout.prop(obj.data, "color", text="色")
        layout.prop(props, "intensity", text="明るさ")
        layout.prop(props, "radius", text="届く距離")
        layout.prop(props, "decay", text="減衰")


# === register ===
classes = (
    PointLightProperty,
    POINTLIGHT_PT_panel,
)

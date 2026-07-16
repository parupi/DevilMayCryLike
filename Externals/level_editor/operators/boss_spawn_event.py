import bpy  # type: ignore


# === イベント全体のプロパティ ===
class BossSpawnEventProperty(bpy.types.PropertyGroup):
    boss: bpy.props.PointerProperty(
        name="Boss",
        type=bpy.types.Object,
        description="このイベントで出現させるボスオブジェクト"
    )  # type: ignore


# === パネル ===
class BOSSSPAWN_PT_panel(bpy.types.Panel):
    bl_label = "ボス出現イベント設定"
    bl_idname = "BOSSSPAWN_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "Event_BossSpawn"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.boss_spawn_event

        layout.label(text="出現させるボス（カメラアップ＋ゆっくりディゾルブ）")
        layout.prop(props, "boss", text="ボス")

        layout.label(text="※ トリガーに BOX コライダーを追加してください")


# === register ===
classes = (
    BossSpawnEventProperty,
    BOSSSPAWN_PT_panel,
)

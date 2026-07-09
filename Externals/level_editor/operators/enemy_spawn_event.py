import bpy # type: ignore

# === 個別の敵設定 ===
class EnemySpawnItem(bpy.types.PropertyGroup):
    enemy: bpy.props.PointerProperty(
        name="Enemy",
        type=bpy.types.Object,
        description="出現させる敵オブジェクト"
    ) # type: ignore
    delay: bpy.props.FloatProperty(
        name="Delay",
        description="この敵の出現までの遅延秒数",
        default=0.0,
        min=0.0
    ) # type: ignore

# === イベント全体のプロパティ ===
class EnemySpawnEventProperty(bpy.types.PropertyGroup):
    enemies: bpy.props.CollectionProperty(type=EnemySpawnItem) # type: ignore
    active_index: bpy.props.IntProperty(default=0) # type: ignore

    trigger_type: bpy.props.EnumProperty(
        name="Trigger",
        items=[
            ('AREA', "エリア進入", "エリアに入ったら出現"),
            ('TIME', "時間経過", "一定時間後に出現"),
            ('MANUAL', "手動", "スクリプトなどで手動出現")
        ],
        default='AREA'
    ) # type: ignore

# === UIList ===
class ENEMYSPAWN_UL_enemy_list(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        # item = EnemySpawnItem
        row = layout.row(align=True)
        row.prop(item, "enemy", text="", emboss=True)
        row.prop(item, "delay", text="Delay")

# === 操作用オペレーター ===
class ENEMYSPAWN_OT_add_enemy(bpy.types.Operator):
    bl_idname = "enemy_spawn.add_enemy"
    bl_label = "敵を追加"

    def execute(self, context):
        obj = context.object
        item = obj.enemy_spawn_event.enemies.add()
        obj.enemy_spawn_event.active_index = len(obj.enemy_spawn_event.enemies) - 1
        return {'FINISHED'}

class ENEMYSPAWN_OT_remove_enemy(bpy.types.Operator):
    bl_idname = "enemy_spawn.remove_enemy"
    bl_label = "敵を削除"

    def execute(self, context):
        obj = context.object
        idx = obj.enemy_spawn_event.active_index
        if 0 <= idx < len(obj.enemy_spawn_event.enemies):
            obj.enemy_spawn_event.enemies.remove(idx)
            obj.enemy_spawn_event.active_index = min(idx, len(obj.enemy_spawn_event.enemies) - 1)
        return {'FINISHED'}

# === パネル ===
class ENEMYSPAWN_PT_panel(bpy.types.Panel):
    bl_label = "敵出現イベント設定"
    bl_idname = "ENEMYSPAWN_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "Event_EnemySpawn"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.enemy_spawn_event

        layout.prop(props, "trigger_type")

        row = layout.row()
        row.template_list(
            "ENEMYSPAWN_UL_enemy_list", "", 
            props, "enemies", 
            props, "active_index"
        )

        col = row.column(align=True)
        col.operator("enemy_spawn.add_enemy", icon="ADD", text="")
        col.operator("enemy_spawn.remove_enemy", icon="REMOVE", text="")

# === register ===
classes = (
    EnemySpawnItem,
    EnemySpawnEventProperty,
    ENEMYSPAWN_UL_enemy_list,
    ENEMYSPAWN_OT_add_enemy,
    ENEMYSPAWN_OT_remove_enemy,
    ENEMYSPAWN_PT_panel,
)

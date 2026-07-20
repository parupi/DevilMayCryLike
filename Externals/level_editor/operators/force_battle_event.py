import bpy  # type: ignore

# === 出現させる敵1体分の設定 ===
class ForceBattleEnemyItem(bpy.types.PropertyGroup):
    enemy: bpy.props.PointerProperty(
        name="Enemy",
        type=bpy.types.Object,
        description="この強制戦闘で出現させる敵オブジェクト"
    )  # type: ignore


# === イベント全体のプロパティ ===
class ForceBattleEventProperty(bpy.types.PropertyGroup):
    enemies: bpy.props.CollectionProperty(type=ForceBattleEnemyItem)  # type: ignore
    active_index: bpy.props.IntProperty(default=0)  # type: ignore


# === UIList ===
class FORCEBATTLE_UL_enemy_list(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        layout.prop(item, "enemy", text="", emboss=True)


# === 操作用オペレーター ===
class FORCEBATTLE_OT_add_enemy(bpy.types.Operator):
    bl_idname = "force_battle.add_enemy"
    bl_label = "敵を追加"

    def execute(self, context):
        obj = context.object
        obj.force_battle_event.enemies.add()
        obj.force_battle_event.active_index = len(obj.force_battle_event.enemies) - 1
        return {'FINISHED'}


class FORCEBATTLE_OT_remove_enemy(bpy.types.Operator):
    bl_idname = "force_battle.remove_enemy"
    bl_label = "敵を削除"

    def execute(self, context):
        obj = context.object
        idx = obj.force_battle_event.active_index
        if 0 <= idx < len(obj.force_battle_event.enemies):
            obj.force_battle_event.enemies.remove(idx)
            obj.force_battle_event.active_index = min(idx, len(obj.force_battle_event.enemies) - 1)
        return {'FINISHED'}


# === パネル ===
class FORCEBATTLE_PT_panel(bpy.types.Panel):
    bl_label = "強制戦闘イベント設定"
    bl_idname = "FORCEBATTLE_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "Event_ForceBattle"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.force_battle_event

        layout.label(text="出現させる敵（全滅でロック解除）")
        row = layout.row()
        row.template_list(
            "FORCEBATTLE_UL_enemy_list", "",
            props, "enemies",
            props, "active_index"
        )
        col = row.column(align=True)
        col.operator("force_battle.add_enemy", icon="ADD", text="")
        col.operator("force_battle.remove_enemy", icon="REMOVE", text="")

        layout.label(text="※ エリア兼トリガーに BOX コライダーを追加してください")


# === register ===
classes = (
    ForceBattleEnemyItem,
    ForceBattleEventProperty,
    FORCEBATTLE_UL_enemy_list,
    FORCEBATTLE_OT_add_enemy,
    FORCEBATTLE_OT_remove_enemy,
    FORCEBATTLE_PT_panel,
)

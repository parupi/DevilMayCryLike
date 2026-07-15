import bpy # type: ignore
import json
import os

# === クリア条件の種類 ===
class ClearConditionItem(bpy.types.PropertyGroup):
    condition_type: bpy.props.EnumProperty(
        name="Condition Type",
        items=[
            ('ALL_ENEMIES_DEFEATED', "全敵撃破", "敵が全て倒されたらクリア"),
            ('DEFEAT_ENEMIES', "敵撃破", "指定した敵を倒したらクリア"), 
            ('REACH_OBJECT', "到達", "特定のオブジェクトに到達したらクリア"),
            ('TIMER', "タイマー", "制限時間を超えたらクリア"),
        ],
        default='ALL_ENEMIES_DEFEATED'
    ) # type: ignore

    target: bpy.props.PointerProperty(
        name="Target Object",
        type=bpy.types.Object,
        description="対象となるオブジェクト（到達系などで使用）"
    ) # type: ignore

    timer: bpy.props.FloatProperty(
        name="Timer",
        description="制限時間（秒）",
        default=60.0,
        min=0.0
    ) # type: ignore

    # 敵撃破用リスト
    enemy_names: bpy.props.StringProperty(
        name="敵リスト",
        description="倒す対象の敵名をカンマ区切りで入力",
        default=""
    ) # type: ignore



# === イベント全体のプロパティ ===
class ClearEventProperty(bpy.types.PropertyGroup):
    conditions: bpy.props.CollectionProperty(type=ClearConditionItem) # type: ignore
    active_index: bpy.props.IntProperty(default=0) # type: ignore
    


# === UIList ===
class CLEAREVENT_UL_condition_list(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        row = layout.row(align=True)
        row.prop(item, "condition_type", text="", emboss=True)

        if item.condition_type == 'REACH_OBJECT':
            row.prop(item, "target", text="")
        elif item.condition_type == 'TIMER':
            row.prop(item, "timer", text="Time")
        elif item.condition_type == 'DEFEAT_ENEMIES':
            row.prop(item, "enemy_names", text="Enemies") 



# === 操作用オペレーター ===
class CLEAREVENT_OT_add_condition(bpy.types.Operator):
    bl_idname = "clear_event.add_condition"
    bl_label = "条件を追加"

    def execute(self, context):
        obj = context.object
        item = obj.clear_event.conditions.add()
        obj.clear_event.active_index = len(obj.clear_event.conditions) - 1
        return {'FINISHED'}


class CLEAREVENT_OT_remove_condition(bpy.types.Operator):
    bl_idname = "clear_event.remove_condition"
    bl_label = "条件を削除"

    def execute(self, context):
        obj = context.object
        idx = obj.clear_event.active_index
        if 0 <= idx < len(obj.clear_event.conditions):
            obj.clear_event.conditions.remove(idx)
            obj.clear_event.active_index = min(idx, len(obj.clear_event.conditions) - 1)
        return {'FINISHED'}


# === パネル ===
class CLEAREVENT_PT_panel(bpy.types.Panel):
    bl_label = "クリアイベント設定"
    bl_idname = "CLEAREVENT_PT_panel"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'object'

    @classmethod
    def poll(cls, context):
        obj = context.object
        return obj is not None and obj.get("class_name") == "Event_Clear"

    def draw(self, context):
        layout = self.layout
        obj = context.object
        props = obj.clear_event

        row = layout.row()
        row.template_list(
            "CLEAREVENT_UL_condition_list", "", 
            props, "conditions", 
            props, "active_index"
        )

        col = row.column(align=True)
        col.operator("clear_event.add_condition", icon="ADD", text="")
        col.operator("clear_event.remove_condition", icon="REMOVE", text="")

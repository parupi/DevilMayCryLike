import bpy # type: ignore
import bpy_extras # type: ignore
import math
import json

# オペレータ シーン出力
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"
    filename_ext = ".json"

    def export_json(self):
        json_object_root = {
            "name": "scene",
            "objects": []
        }

        for obj in bpy.context.scene.objects:
            if obj.parent:
                continue
            self.parse_scene_recursive_json(json_object_root["objects"], obj, 0)

        with open(self.filepath, "wt", encoding="utf-8") as file:
            json.dump(json_object_root, file, ensure_ascii=False, indent=4)
            print(json.dumps(json_object_root, ensure_ascii=False, indent=4))

    def parse_scene_recursive_json(self, data_parent, obj, level):
        # 無効フラグがTrueなら出力しない
        if getattr(obj, "disabled", False):
            return

        json_object = {
            "name": obj.name,
            "class": obj.get("class_name", "Object3d"),
        }

        # 変換
        trans, rot, scale = obj.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x, rot.y, rot.z = map(math.degrees, (rot.x, rot.y, rot.z))

        json_object["transform"] = {
            "translation": (trans.x, trans.y, trans.z),
            "rotation": (rot.x, rot.y, rot.z),
            "scaling": (scale.x, scale.y, scale.z),
        }

        # ファイル情報
        if "file_name" in obj:
            json_object["file_name"] = obj["file_name"]

        # コライダー情報
        if "collider" in obj:
            json_object["collider"] = {
                "type": obj["collider"],
                "center": obj["collider_center"].to_list(),
                "size": obj["collider_size"].to_list()
            }

        # ========= イベント固有の情報 =========
        if obj.get("class_name") == "Event_EnemySpawn":
            if hasattr(obj, "enemy_spawn_event"):
                props = obj.enemy_spawn_event
                json_object["event"] = {
                    "type": "EnemySpawn",
                    "trigger": props.trigger_type,
                    "enemies": [
                        {
                            "name": e.enemy.name if e.enemy else None,
                            "delay": e.delay
                        }
                        for e in props.enemies
                    ]
                }

        elif obj.get("class_name") == "Event_ForceBattle":
            json_object["event"] = {
                "type": "ForceBattle",
                "enemies": []
            }
            if hasattr(obj, "force_battle_event"):
                for e in obj.force_battle_event.enemies:
                    json_object["event"]["enemies"].append({
                        "name": e.enemy.name if e.enemy else None
                    })

        elif obj.get("class_name") == "Event_Clear":
            if hasattr(obj, "clear_event"):
                props = obj.clear_event
                json_object["event"] = {
                    "type": "Clear",
                    "conditions": []
                }
                for cond in props.conditions:
                    cond_data = {"type": cond.condition_type}
                    if cond.condition_type == 'REACH_OBJECT' and cond.target:
                        cond_data["target"] = cond.target.name
                    elif cond.condition_type == 'TIMER':
                        cond_data["time"] = cond.timer
                    elif cond.condition_type == 'DEFEAT_ENEMIES':
                        cond_data["targets"] = [s.strip() for s in cond.enemy_names.split(",") if s.strip()]
                    json_object["event"]["conditions"].append(cond_data)
            else:
                json_object["event"] = {"type": "Clear"}

        # ====================================

        data_parent.append(json_object)

        # 子オブジェクトの処理（disabled も考慮される）
        if obj.children:
            json_object["children"] = []
            for child in obj.children:
                self.parse_scene_recursive_json(json_object["children"], child, level + 1)

    def execute(self, context):
        print("シーン情報をExportします")
        self.export_json()
        print("シーン情報をExportしました")
        self.report({'INFO'}, "シーン情報をExportしました")
        return {'FINISHED'}
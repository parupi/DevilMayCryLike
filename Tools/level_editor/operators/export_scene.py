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
            "type": obj.type,
            "name": obj.name,
            "class": obj.get("class_name", "Object3d"),
        }

        trans, rot, scale = obj.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x, rot.y, rot.z = map(math.degrees, (rot.x, rot.y, rot.z))

        json_object["transform"] = {
        "translation": (trans.x, trans.y, trans.z),
        "rotation": (rot.x, rot.y, rot.z),
        "scaling": (scale.x, scale.y, scale.z),
        }

        if "file_name" in obj:
            json_object["file_name"] = obj["file_name"]

        if "collider" in obj:
            json_object["collider"] = {
            "type": obj["collider"],
            "center": obj["collider_center"].to_list(),
            "size": obj["collider_size"].to_list()
            }

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
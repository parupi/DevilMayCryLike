import gpu # type: ignore
import bpy # type: ignore
import mathutils # type: ignore
import gpu_extras.batch # type: ignore

# 当たり判定描画用
class DrawCollider:
    handle = None

    @staticmethod
    def draw_collider():
        vertices = {"pos": []}
        indices = []

        offsets = [
            [-0.5, -0.5, -0.5], [+0.5, -0.5, -0.5],
            [-0.5, +0.5, -0.5], [+0.5, +0.5, -0.5],
            [-0.5, -0.5, +0.5], [+0.5, -0.5, +0.5],
            [-0.5, +0.5, +0.5], [+0.5, +0.5, +0.5],
        ]

        for obj in bpy.context.scene.objects:
            if "collider" not in obj:
                continue

            center = mathutils.Vector(obj["collider_center"])
            size = mathutils.Vector(obj["collider_size"])

            start = len(vertices["pos"])
            for offset in offsets:
                pos = obj.matrix_world @ (center + mathutils.Vector(offset) * size)
                vertices['pos'].append(pos)

            indices += [
                [start+0, start+1], [start+2, start+3], [start+0, start+2], [start+1, start+3],
                [start+4, start+5], [start+6, start+7], [start+4, start+6], [start+5, start+7],
                [start+0, start+4], [start+1, start+5], [start+2, start+6], [start+3, start+7]
            ]

        shader = gpu.shader.from_builtin("UNIFORM_COLOR")
        batch = gpu_extras.batch.batch_for_shader(shader, "LINES", vertices, indices=indices)
        shader.bind()
        shader.uniform_float("color", [0.5, 1.0, 1.0, 1.0])
        batch.draw(shader)

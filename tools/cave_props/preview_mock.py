# Blender: 生成モデルの確認画像
#  mode=lineup : 生成モデルを並べる
#  mode=wall   : 実際の壁タイル（下段＋上段）に張り出しを載せた、部屋の内側からの眺め
# blender -b -P preview_mock.py -- <mode> <packDir> <genDir> <out.png>
import bpy, sys, os, math
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
mode, pack, gen, out = argv[:4]

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


def imp(path, loc=(0, 0, 0), rot_z=0.0):
    """OBJ を読み込む。loc はファイル座標（Y上）で渡し、Blender 座標（Z上）へ直す"""
    before = set(bpy.data.objects)
    bpy.ops.wm.obj_import(filepath=path)
    objs = [o for o in bpy.data.objects if o not in before]
    for o in objs:
        o.rotation_euler.z += rot_z
        o.location = Vector((loc[0], -loc[2], loc[1]))
    return objs


def bounds(objs):
    mn = Vector((1e9, 1e9, 1e9)); mx = -mn
    for o in objs:
        bpy.context.view_layer.update()
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            mn = Vector(map(min, mn, w)); mx = Vector(map(max, mx, w))
    return mn, mx


def camera(loc, target, lens=35):
    cd = bpy.data.cameras.new("cam"); cd.lens = lens
    cam = bpy.data.objects.new("cam", cd); scene.collection.objects.link(cam)
    cam.location = loc
    cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam


scene.render.engine = 'BLENDER_WORKBENCH'
sh = scene.display.shading
sh.light = 'STUDIO'
sh.color_type = 'MATERIAL'
sh.show_cavity = True
sh.show_shadows = True

if mode == "lineup":
    names = ["CaveOverhang_01", "CaveOverhang_02", "CaveOverhang_03", "CaveOverhang_04", "CaveOverhang_Corner",
             "Stalactite_01", "Stalactite_02", "Stalactite_03", "StalactiteCluster_01",
             "CaveRockLarge_01", "CaveRockLarge_02", "CaveRockCluster_01"]
    x = 0.0
    rowx = [0.0, 0.0]
    for i, n in enumerate(names):
        row = 0 if i < 5 else 1
        objs = imp(os.path.join(gen, n, n + ".obj"))
        mn, mx = bounds(objs)
        z0 = 0 if row == 0 else -4.2
        # 鍾乳石は根元を上にして宙に吊る
        lift = (2.2 - mx.z) if n.startswith("Stalactite") else -mn.z
        off = Vector((rowx[row] - mn.x, -(mn.y + mx.y) / 2, z0 + lift))
        for o in objs:
            o.location += off
        rowx[row] += (mx.x - mn.x) + 0.5
    w = max(rowx)
    cd = bpy.data.cameras.new("cam"); cd.type = 'ORTHO'; cd.ortho_scale = w * 1.08
    cam = bpy.data.objects.new("cam", cd); scene.collection.objects.link(cam)
    cam.location = (w / 2 + 4, -14, 4)
    cam.rotation_euler = (Vector((w / 2, 0, -0.8)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = cam
    scene.render.resolution_x, scene.render.resolution_y = 1800, 1100
else:
    # 東の壁（回転なし）。ステージの値を 1/3 にした位置に置く（scale3 を外した単位）
    #   下段 y=-2.625/3, 上段 y=3.375/3, 床 y=-6.125/3, タイル間隔 6/3=2
    variants = ["CaveOverhang_01", "CaveOverhang_02", "CaveOverhang_03", "CaveOverhang_04"]
    for i in range(-3, 3):
        z = i * 2.0
        imp(os.path.join(pack, "ModularStoneWall.obj"), (0, -0.875, z))
        imp(os.path.join(pack, "ModularStoneWall_top.obj"), (0, 1.125, z))
        n = variants[(i + 3) % 4]
        imp(os.path.join(gen, n, n + ".obj"), (0, 1.125, z))
        imp(os.path.join(pack, "ModularFloor.obj"), (1.0, -2.042, z))
        imp(os.path.join(pack, "ModularFloor.obj"), (3.0, -2.042, z))
    # 床に岩
    imp(os.path.join(gen, "CaveRockCluster_01", "CaveRockCluster_01.obj"), (0.9, -1.68, 1.6))
    imp(os.path.join(gen, "CaveRockLarge_02", "CaveRockLarge_02.obj"), (1.1, -1.68, -3.8))
    # 部屋の内側（+X 側）から見上げる。Blender の座標なので (x, -z, y)
    camera((9.0, 3.5, -0.9), (0, -1.0, 1.4), lens=22)
    # 洞窟っぽく背景を黒くして見え方を確かめる
    scene.world = bpy.data.worlds.new("w")
    scene.world.color = (0.003, 0.004, 0.005)
    sh.background_type = 'WORLD'
    scene.render.resolution_x, scene.render.resolution_y = 1600, 900

scene.render.filepath = out
bpy.ops.render.render(write_still=True)

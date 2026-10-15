# Blender: 洞窟用の岩モデルを作って OBJ で書き出す（Resource/models/Cave/ の中身はこれで作った）
#   "C:\Program Files\Blender Foundation\Blender 4.5\blender.exe" -b -P tools/cave_props/gen_cave.py -- Resource/models/Cave
# 見た目の確認画像: preview_mock.py（lineup=並べる / wall=壁に載せる。パックの OBJ フォルダが要る）
# 形は seed 固定なので、何度実行しても同じものができる。変えたいときは末尾の models の引数をいじる。
#
# 置き方:
#   CaveOverhang_01〜04 … ModularStoneWall_top と同じ位置・回転・スケールで置けば壁の上に合う
#   CaveOverhang_Corner  … 原点が部屋の角。回転なしなら +X と -Z が部屋の内側（北東の角）
#   Stalactite*          … 原点が根元。-Y へ垂れる
#   CaveRock*            … 原点が接地面の中心
#
# 座標はパックの OBJ と同じ（Y が上、+X が部屋の内側、壁の内側の面は x≒0.06、
# 壁タイルは z が -1〜1）。Blender の座標として扱うが、書き出しは数値そのままなので
# 軸変換は入らない。
import bpy, bmesh, sys, os, math, random
from mathutils import Vector, Matrix, Euler

argv = sys.argv[sys.argv.index("--") + 1:]
OUT_ROOT = argv[0]

ROCK = "Rock"
LIGHT = "RockLight"

MTL_TEXT = """# Blender MTL File: '{name}.blend'
# Material Count: 2

newmtl Rock
Ns 96.078431
Ka 1.000000 1.000000 1.000000
Kd 0.287072 0.287072 0.287072
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.000000
d 1.000000
illum 2

newmtl RockLight
Ns 96.078431
Ka 1.000000 1.000000 1.000000
Kd 0.510617 0.510617 0.510617
Ks 0.500000 0.500000 0.500000
Ke 0.000000 0.000000 0.000000
Ni 1.000000
d 1.000000
illum 2
"""


def hull(points):
    """点群の凸包。外向きの三角形のリスト（頂点座標）を返す"""
    bm = bmesh.new()
    for p in points:
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=bm.verts[:])
    # 凸包に使われなかった内側の点を消す
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    # ほぼ同一平面の三角形をまとめて、パックのような大きな面にする
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(4), verts=bm.verts[:], edges=bm.edges[:])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    tris = [[v.co.copy() for v in f.verts] for f in bm.faces]
    bm.free()
    return tris


def rock(rng, center, radii, n=14, rot=None, floor=None, squash_top=0.0):
    """ゆがんだ楕円体の上の点の凸包＝パックの Rock1〜5 と同じ作り方の岩"""
    pts = []
    for _ in range(n):
        # 球面上の一様な点
        z = rng.uniform(-1, 1)
        t = rng.uniform(0, 2 * math.pi)
        r = math.sqrt(1 - z * z)
        d = Vector((r * math.cos(t), z, r * math.sin(t)))
        k = rng.uniform(0.78, 1.0)
        p = Vector((d.x * radii[0], d.y * radii[1], d.z * radii[2])) * k
        if squash_top and p.y > 0:
            p.y *= (1 - squash_top)
        pts.append(p)
    m = (rot or Euler((rng.uniform(-0.3, 0.3), rng.uniform(0, 6.28), rng.uniform(-0.3, 0.3)))).to_matrix()
    pts = [m @ p + Vector(center) for p in pts]
    if floor is not None:
        pts = [Vector((p.x, max(p.y, floor), p.z)) for p in pts]
    return hull(pts)


def stalactite(rng, top, length, radius, sides=6, lean=0.12):
    """上が根元・下が先の鍾乳石。輪を数段重ねて先を尖らせた凸包"""
    pts = []
    rings = 4
    lx = rng.uniform(-lean, lean) * length
    lz = rng.uniform(-lean, lean) * length
    phase = rng.uniform(0, 6.28)
    for i in range(rings):
        t = i / rings                 # 0=根元
        rr = radius * (1 - t) ** 1.25
        y = -length * t
        for s in range(sides):
            a = phase + s / sides * 2 * math.pi + rng.uniform(-0.25, 0.25)
            q = rr * rng.uniform(0.8, 1.0)
            pts.append(Vector((math.cos(a) * q + lx * t, y, math.sin(a) * q + lz * t)))
    pts.append(Vector((lx, -length, lz)))  # 先端
    pts = [p + Vector(top) for p in pts]
    return hull(pts)


class Model:
    def __init__(self, name):
        self.name = name
        self.parts = {ROCK: [], LIGHT: []}

    def add(self, mat, tris):
        self.parts[mat].extend(tris)

    def write(self, root):
        d = os.path.join(root, self.name)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, self.name + ".mtl"), "w", newline="\n") as f:
            f.write(MTL_TEXT.format(name=self.name))
        verts, vidx, norms, nidx = [], {}, [], {}
        lines_by_mat = {}
        for mat, tris in self.parts.items():
            out = []
            for tri in tris:
                n = (tri[1] - tri[0]).cross(tri[2] - tri[0])
                if n.length < 1e-9:
                    continue
                n.normalize()
                nk = (round(n.x, 4), round(n.y, 4), round(n.z, 4))
                if nk not in nidx:
                    nidx[nk] = len(norms) + 1
                    norms.append(nk)
                ids = []
                for v in tri:
                    k = (round(v.x, 6), round(v.y, 6), round(v.z, 6))
                    if k not in vidx:
                        vidx[k] = len(verts) + 1
                        verts.append(k)
                    ids.append(vidx[k])
                out.append("f " + " ".join(f"{i}//{nidx[nk]}" for i in ids))
            if out:
                lines_by_mat[mat] = out
        with open(os.path.join(d, self.name + ".obj"), "w", newline="\n") as f:
            f.write(f"# Cave props generated for GuchisEngine (pack style: Rock / RockLight, flat shaded)\n")
            f.write(f"mtllib {self.name}.mtl\n")
            f.write(f"o {self.name}\n")
            for v in verts:
                f.write("v %.6f %.6f %.6f\n" % v)
            for n in norms:
                f.write("vn %.4f %.4f %.4f\n" % n)
            for mat, out in lines_by_mat.items():
                f.write(f"usemtl {mat}\n")
                f.write("s off\n")
                f.write("\n".join(out) + "\n")
        tri_count = sum(len(v) for v in lines_by_mat.values())
        print(f"[cave] {self.name}: {len(verts)} verts / {tri_count} tris")


# ─────────────────────────────────────────────
# 壁上段の張り出し（ModularStoneWall_top と同じ位置・回転で置く）
#   壁の上端は y≒1.24。内側の面は x≒0.06。タイル幅は z -1〜1。
# ─────────────────────────────────────────────
def overhang(name, seed, reach=1.0, height=1.0, drips=3):
    rng = random.Random(seed)
    m = Model(name)
    # 1) 壁の上端の縁（y≒1.24, x≒0.07）を前から隠す岩の列。隣のタイルと重なるよう z は ±1 を少し越える
    for i, z in enumerate([-1.05, -0.52, 0.0, 0.52, 1.05]):
        z += rng.uniform(-0.1, 0.1)
        c = (rng.uniform(0.2, 0.42), rng.uniform(1.0, 1.25), z)
        m.add(LIGHT if i == 2 else ROCK,
              rock(rng, c, (rng.uniform(0.4, 0.5), rng.uniform(0.4, 0.55), rng.uniform(0.38, 0.5)), n=13))
    # 2) 上に積み重なる大きな塊（天井へ続いていくように見せる）
    for z in [-0.7, 0.0, 0.7]:
        z += rng.uniform(-0.15, 0.15)
        r = rng.uniform(0.6, 0.75)
        c = (rng.uniform(-0.2, 0.3), 1.75 + rng.uniform(0.0, 0.3) * height, z)
        m.add(ROCK, rock(rng, c, (r, r * rng.uniform(0.85, 1.1) * height, r * 0.9), n=15))
    # 3) 空の輪郭をギザギザにする、背の高い岩
    for _ in range(2):
        z = rng.uniform(-0.8, 0.8)
        r = rng.uniform(0.4, 0.52)
        c = (rng.uniform(-0.3, 0.2), 2.25 + rng.uniform(0.0, 0.35) * height, z)
        m.add(ROCK, rock(rng, c, (r * 1.1, r * rng.uniform(1.15, 1.5) * height, r * 0.95), n=14))
    # 4) 内側へ大きく張り出す平たい岩（少し下へ傾ける）と、その先の岩
    shelves = []
    zs = [-0.55, 0.55] if rng.random() < 0.5 else [-0.7, 0.05, 0.75]
    for z in zs:
        z += rng.uniform(-0.12, 0.12)
        x = rng.uniform(0.8, 1.05) * reach
        y = rng.uniform(1.5, 1.8)
        rx = rng.uniform(0.9, 1.1) * reach
        rot = Euler((rng.uniform(-0.12, 0.12), rng.uniform(-0.35, 0.35), rng.uniform(-0.3, -0.14)))
        m.add(ROCK, rock(rng, (x, y, z), (rx, rng.uniform(0.28, 0.38), rng.uniform(0.55, 0.7)), n=15, rot=rot))
        tip = (x + rx * 0.75, y - rx * 0.25, z + rng.uniform(-0.2, 0.2))
        m.add(ROCK, rock(rng, tip, (0.35, 0.3, 0.35), n=11))
        shelves.append((x, y, z, rx))
    # 5) 張り出しの裏から垂れる鍾乳石
    for i in range(drips):
        x, y, z, rx = shelves[i % len(shelves)]
        t = rng.uniform(-0.2, 0.6)
        top = (x + rx * t, y - 0.22 - rx * 0.25 * max(t, 0), z + rng.uniform(-0.3, 0.3))
        m.add(LIGHT if i % 2 == 0 else ROCK,
              stalactite(rng, top, rng.uniform(0.5, 1.1), rng.uniform(0.11, 0.17)))
    return m


def overhang_corner(name, seed):
    """部屋の角用。原点が角で、+X と -Z の向きが部屋の内側（北東の角を回転なしで置いた場合）"""
    rng = random.Random(seed)
    m = Model(name)
    # 角を埋める大きな塊
    m.add(ROCK, rock(rng, (0.25, 1.8, -0.25), (1.15, 1.0, 1.15), n=18))
    m.add(ROCK, rock(rng, (-0.1, 2.5, 0.05), (0.7, 0.75, 0.65), n=15))
    m.add(LIGHT, rock(rng, (0.9, 2.2, -0.5), (0.5, 0.45, 0.55), n=13))
    m.add(ROCK, rock(rng, (0.4, 2.15, -1.1), (0.55, 0.5, 0.5), n=13))
    # 両側の壁沿いに伸びる縁隠しの岩
    for i in range(3):
        t = (i + 1) / 3
        m.add(ROCK, rock(rng, (0.3 + 1.3 * t, 1.1 + rng.uniform(0, 0.2), -0.3), (0.5, 0.48, 0.45), n=13))
        m.add(ROCK if i else LIGHT, rock(rng, (0.3, 1.1 + rng.uniform(0, 0.2), -0.3 - 1.3 * t), (0.45, 0.48, 0.5), n=13))
    # 斜めに張り出す岩と鍾乳石
    m.add(ROCK, rock(rng, (1.1, 1.55, -1.1), (1.2, 0.35, 0.75), n=15, rot=Euler((0.18, 0.78, -0.22))))
    m.add(ROCK, rock(rng, (1.85, 1.3, -1.85), (0.4, 0.32, 0.4), n=11))
    for top, ln in [((1.2, 1.25, -0.9), 1.0), ((0.8, 1.3, -1.35), 0.6), ((1.6, 1.15, -1.5), 0.75), ((0.6, 0.85, -0.5), 0.45)]:
        m.add(LIGHT, stalactite(rng, top, ln, 0.15))
    return m


# ─────────────────────────────────────────────
# 鍾乳石（原点＝根元。-Y へ垂れる）
# ─────────────────────────────────────────────
def stalactite_single(name, seed, length, radius):
    rng = random.Random(seed)
    m = Model(name)
    # 根元に付いた小さな岩（ぶら下がり元が見えても切れ目が目立たないように）
    m.add(ROCK, rock(rng, (0, 0.05, 0), (radius * 1.6, radius * 0.7, radius * 1.6), n=10))
    m.add(LIGHT, stalactite(rng, (0, 0, 0), length, radius, sides=6))
    return m


def stalactite_cluster(name, seed):
    rng = random.Random(seed)
    m = Model(name)
    m.add(ROCK, rock(rng, (0, 0.12, 0), (0.75, 0.22, 0.6), n=14))
    for i in range(6):
        a = i / 6 * 2 * math.pi + rng.uniform(-0.3, 0.3)
        d = rng.uniform(0.1, 0.5)
        top = (math.cos(a) * d, 0.02, math.sin(a) * d * 0.8)
        ln = rng.uniform(0.3, 1.0) * (1.2 if d < 0.3 else 0.8)
        m.add(LIGHT if i % 2 else ROCK, stalactite(rng, top, ln, rng.uniform(0.08, 0.15)))
    return m


# ─────────────────────────────────────────────
# 床置き（原点＝接地面の中心。y=0 が床）
# ─────────────────────────────────────────────
def rock_large(name, seed, size):
    rng = random.Random(seed)
    m = Model(name)
    m.add(ROCK, rock(rng, (0, size * 0.55, 0), (size, size * 0.8, size * 0.85), n=18, floor=0.0))
    m.add(LIGHT, rock(rng, (size * 0.55, size * 0.25, size * 0.45), (size * 0.45, size * 0.4, size * 0.4), n=12, floor=0.0))
    return m


def rock_cluster(name, seed):
    rng = random.Random(seed)
    m = Model(name)
    spots = [(0, 0, 0.62), (0.75, 0.3, 0.42), (-0.7, 0.2, 0.48), (0.25, -0.65, 0.35), (-0.3, 0.7, 0.3), (1.1, -0.4, 0.22), (-1.05, -0.45, 0.25)]
    for i, (x, z, r) in enumerate(spots):
        m.add(LIGHT if i in (1, 4) else ROCK,
              rock(rng, (x, r * 0.5, z), (r * 1.1, r * rng.uniform(0.8, 1.3), r), n=13, floor=0.0))
    return m


models = [
    overhang("CaveOverhang_01", 11, reach=1.0, height=1.0, drips=3),
    overhang("CaveOverhang_02", 23, reach=1.2, height=0.8, drips=3),
    overhang("CaveOverhang_03", 37, reach=0.8, height=1.3, drips=2),
    overhang("CaveOverhang_04", 41, reach=1.1, height=1.1, drips=3),
    overhang_corner("CaveOverhang_Corner", 53),
    stalactite_single("Stalactite_01", 61, 0.55, 0.12),
    stalactite_single("Stalactite_02", 67, 0.95, 0.16),
    stalactite_single("Stalactite_03", 71, 1.45, 0.2),
    stalactite_cluster("StalactiteCluster_01", 79),
    rock_large("CaveRockLarge_01", 83, 0.9),
    rock_large("CaveRockLarge_02", 89, 1.3),
    rock_cluster("CaveRockCluster_01", 97),
]
for mdl in models:
    mdl.write(OUT_ROOT)

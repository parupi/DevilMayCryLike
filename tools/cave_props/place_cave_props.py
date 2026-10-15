# 洞窟の飾り（Resource/models/Cave）と松明を、トレーニングルーム型の部屋のステージへ並べる
#   python tools/cave_props/place_cave_props.py Resource/Stage/Tutorial.json Resource/Stage/Boss.json
#
# 何度実行しても同じ結果になる（名前が "Cave_" で始まるオブジェクトを消してから置き直す）。
# 位置はステージにある壁タイルから割り出すので、部屋の大きさが変わっても追従する。
import json, sys, random, math

PREFIX = "Cave_"
OVERHANGS = ["Cave/CaveOverhang_01", "Cave/CaveOverhang_02", "Cave/CaveOverhang_03", "Cave/CaveOverhang_04"]

# 床の上面（ModularFloor の上端 0.36 × scale3 を床タイルの y に足した値）
FLOOR_TOP_OFFSET = 0.36 * 3.0
# 壁タイルの原点から部屋側の壁面までの距離（ModularStoneWall の x 0.06 × scale3）
WALL_FACE_INSET = 0.18
# 松明: 床からの高さと、壁ごとに置く位置（壁に沿った座標）
TORCH_HEIGHT = 2.0
TORCH_ALONG = [-12.0, 12.0]

# 部屋の角の向き。CaveOverhang_Corner は回転なしで「+X と +Z の角（北東）」に合う。
# 他の角は、その向きの壁と同じ回転を使うと合う（南西=西の壁、北西=北の壁、南東=南の壁）
CORNER_ROT = {
    (+1, +1): [0.0, 0.0, 0.0, 1.0],
    (-1, -1): [0.0, 1.0, 0.0, 0.0],
    (-1, +1): [0.0, -0.7071068, 0.0, 0.7071068],
    (+1, -1): [0.0, 0.7071068, 0.0, 0.7071068],
}


def side_of(t):
    """壁タイルの位置から、どの壁か（法線の向き）を返す"""
    x, _, z = t
    if abs(x) >= abs(z):
        return ("x", 1 if x > 0 else -1)
    return ("z", 1 if z > 0 else -1)


def place(path):
    data = json.load(open(path, encoding="utf-8"))
    objs = [o for o in data["objects"] if not o["name"].startswith(PREFIX)]
    removed = len(data["objects"]) - len(objs)

    tops = [o for o in objs if o.get("model") == "ModularStoneWall_top"]
    walls = [o for o in objs if o.get("model") == "ModularStoneWall"]
    floors = [o for o in objs if o.get("model") == "ModularFloor"]
    if not tops or not walls or not floors:
        print(f"{path}: 壁タイルが見つからないので飛ばす")
        return

    floor_top = floors[0]["translate"][1] + FLOOR_TOP_OFFSET
    top_y = tops[0]["translate"][1]
    scale = tops[0]["scale"]
    edge = max(abs(c) for o in tops for c in (o["translate"][0], o["translate"][2]))
    rng = random.Random(20261015)
    added = []

    def add(name, model, t, r, s, collider=None, cast_shadow=True):
        o = {"name": PREFIX + name, "class": "Ground", "translate": [round(v, 4) for v in t],
             "rotate": r, "scale": s, "model": model}
        if not cast_shadow:
            # 張り出しが影を落とすと下の壁が丸ごと影に入り、CSM のカスケードの境目が輪になって見える
            o["castShadow"] = False
        if collider:
            o["collider"] = collider
        added.append(o)

    # 1) 壁上段タイルと同じ transform で張り出しを載せる。隣と同じ形が続かないようにずらす
    prev = None
    for o in sorted(tops, key=lambda o: (side_of(o["translate"]), o["translate"][0] + o["translate"][2])):
        choices = [m for m in OVERHANGS if m != prev]
        m = rng.choice(choices)
        prev = m
        add("Overhang_" + o["name"], m, o["translate"], o["rotate"], o["scale"], cast_shadow=False)

    # 2) 四隅
    for (sx, sz), rot in CORNER_ROT.items():
        add(f"OverhangCorner_{'E' if sx > 0 else 'W'}{'N' if sz > 0 else 'S'}", "Cave/CaveOverhang_Corner",
            [sx * edge, top_y, sz * edge], rot, scale, cast_shadow=False)

    # 3) 松明。壁ごとに2本、壁面に付ける。回転はその壁のタイルと同じ（穂先が部屋の内側を向く）
    rot_of_side = {}
    for o in walls:
        rot_of_side.setdefault(side_of(o["translate"]), o["rotate"])
    face = edge - WALL_FACE_INSET
    for (axis, sign), rot in sorted(rot_of_side.items()):
        for i, a in enumerate(TORCH_ALONG):
            t = [sign * face, floor_top + TORCH_HEIGHT, a] if axis == "x" else [a, floor_top + TORCH_HEIGHT, sign * face]
            added.append({
                "name": f"{PREFIX}Torch_{axis}{'+' if sign > 0 else '-'}_{i}", "class": "WallTorch",
                "translate": [round(v, 4) for v in t], "rotate": rot, "scale": [1.0, 1.0, 1.0],
                "model": "Torch_wall",
                "light": {"color": [1.0, 0.62, 0.26], "offset": [-0.89, 0.95, 0.02],
                          "intensity": 3.0, "radius": 9.0, "decay": 1.2},
            })

    # 4) 床の岩。四隅に大岩（当たり判定つき）、壁の根元に岩の塊（低いので判定なし・ほぼ壁の判定の内側）
    corner_in = edge - 3.2
    for k, (sx, sz) in enumerate([(1, 1), (-1, 1), (-1, -1), (1, -1)]):
        model = "Cave/CaveRockLarge_02" if k % 2 == 0 else "Cave/CaveRockLarge_01"
        size = 1.3 if k % 2 == 0 else 0.9
        yaw = rng.uniform(0, 6.283)
        rot = [0.0, round(math.sin(yaw / 2), 6), 0.0, round(math.cos(yaw / 2), 6)]
        s = 2.0
        add(f"RockLarge_{k}", model, [sx * corner_in, floor_top - 0.15, sz * corner_in], rot, [s, s, s],
            collider={"shape": "OBB", "offset": [0.0, size * 0.5 * s, 0.0],
                      "halfExtents": [size * 0.85, size * 0.6, size * 0.85]})
    base = edge - 1.4
    for k, (axis, sign, a) in enumerate([("x", 1, -5.0), ("x", -1, 7.0), ("z", 1, 4.0), ("z", -1, -8.0)]):
        # 岩の塊は x 方向に長いので、壁に沿わせる
        rot = [0.0, 0.7071068, 0.0, 0.7071068] if axis == "x" else [0.0, 0.0, 0.0, 1.0]
        t = [sign * base, floor_top - 0.1, a] if axis == "x" else [a, floor_top - 0.1, sign * base]
        add(f"RockCluster_{k}", "Cave/CaveRockCluster_01", t, rot, [2.0, 2.0, 2.0])

    data["objects"] = objs + added
    # SceneSaver と同じ書式（インデント2・CRLF）で書く。差分が足した分だけになる
    text = json.dumps(data, ensure_ascii=False, indent=2).replace("\n", "\r\n") + "\r\n"
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)
    print(f"{path}: 消した {removed} / 足した {len(added)}")


for p in sys.argv[1:]:
    place(p)

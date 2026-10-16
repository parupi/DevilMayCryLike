# タイトル・クリア画面の回廊ステージを作り直す
#   python tools/cave_props/build_corridor_stages.py          （両方）
#   python tools/cave_props/build_corridor_stages.py title    （Resource/Stage/Title.json だけ）
#   python tools/cave_props/build_corridor_stages.py clear    （Resource/Stage/Clear.json だけ）
#
# 奥へ長く延ばした回廊に、壁の上の張り出しの岩・左右の松明・床の岩・天井の光を置く。
# 奥は雰囲気（Environment_Title / Environment_Clear）の距離フォグで闇に沈む。
# クリアは回廊の奥に「洞窟の出口」の暖かい光を置く（出口から差し込む光の筋と、奥の壁を照らす光）。
# 何度実行しても同じ結果になる（ステージを丸ごと書き直す。乱数は種固定）。
import json, random, sys

# 回廊の寸法（元の Title.json と同じ高さ・幅）。タイル1枚は scale3 で 6m
FLOOR_Y = -1.0
WALL_Y = 1.0
WALL_TOP_Y = 7.0
WALL_X = 8.0                 # 左右の壁の位置（タイルの原点）
FLOOR_XS = [-6.0, 0.0, 6.0]  # 床は3列
TILE = 6.0
SCALE = [3.0, 3.0, 3.0]
FLOOR_TOP = FLOOR_Y + 0.36 * 3.0
WALL_FACE_INSET = 0.18
ROT_RIGHT = [0.0, 0.0, 0.0, 1.0]   # x=+8 の壁（ローカル +X が部屋の内側）
ROT_LEFT = [0.0, 1.0, 0.0, 0.0]    # x=-8 の壁
OVERHANGS = ["Cave/CaveOverhang_01", "Cave/CaveOverhang_02", "Cave/CaveOverhang_03", "Cave/CaveOverhang_04"]
TORCH_LIGHT = {"color": [1.0, 0.62, 0.26], "offset": [-0.89, 0.95, 0.02], "intensity": 3.0, "radius": 9.0, "decay": 1.2}

CONFIGS = {
    # タイトル: プレイヤーは (0,-7)。カメラは手前（z=-30〜-25）から奥を向く
    "title": {
        "out": "Resource/Stage/Title.json",
        "environment": "Title",
        "seed": 20261016,
        "name": "Title",
        "z_start": -18.0,
        "z_end": 66.0,
        "torch_zs": [-3.0, 15.0, 33.0, 51.0],
        "rocks": [("R", 3.0), ("L", 9.0), ("R", 27.0), ("L", 39.0)],
        "spot_lights": [
            ("Cave_TitleCeilingLight", [0.0, 14.0, -7.0],
             {"color": [0.78, 0.86, 1.0], "intensity": 4.0, "distance": 30.0, "decay": 0.012,
              "outerAngle": 13.0, "innerAngle": 8.0, "particleLighting": 1.0, "volumetric": 1.0}),
        ],
        "point_lights": [],
    },
    # クリア: プレイヤーは原点。カメラは斜め前の低い位置から見上げ、背景に回廊の奥（出口の光）が入る
    "clear": {
        "out": "Resource/Stage/Clear.json",
        "environment": "Clear",
        "seed": 20261017,
        "name": "Clear",
        "z_start": -18.0,
        "z_end": 66.0,
        "torch_zs": [-6.0, 12.0, 30.0, 48.0],
        "rocks": [("R", 5.0), ("L", 15.0), ("R", 33.0), ("L", 45.0)],
        "spot_lights": [
            # Main Spot Light: プレイヤーの頭上から。勝利の光なので暖かい白
            ("Cave_ClearMainLight", [0.4, 13.0, -0.4],
             {"color": [1.0, 0.92, 0.8], "intensity": 2.6, "distance": 30.0, "decay": 0.012,
              "outerAngle": 12.0, "innerAngle": 7.0, "particleLighting": 1.0, "volumetric": 1.0}),
            # 洞窟の出口から差し込む外光。回廊の奥の高い所から、奥の床へ斜めに落とす。
            # カメラは円錐の外に置く（中に入ると視線の全区間で光が散って、画面全体が白く飛ぶ）
            ("Cave_ClearExitLight", [0.0, 9.0, 72.0],
             {"color": [1.0, 0.8, 0.55], "intensity": 4.0, "distance": 40.0, "decay": 0.004,
              "outerAngle": 24.0, "innerAngle": 12.0, "particleLighting": 0.0, "volumetric": 0.35},
             # 向き: ローカル -Y を手前（-Z）へ 55° 倒す。照らす向きは (0,-0.57,-0.82)
             [0.4617486, 0.0, 0.0, 0.8870108]),
        ],
        "point_lights": [
            # Fill Light: プレイヤーの斜め前から弱く。顔と剣の形を読めるようにする
            ("Cave_ClearFillLight", [-1.6, 1.8, -2.2],
             {"color": [0.85, 0.9, 1.0], "offset": [0.0, 0.0, 0.0], "intensity": 0.9, "radius": 6.0, "decay": 1.0}),
            # 出口のあたりの壁と床を暖かく照らす（出口がそこにあると分かるように）
            ("Cave_ClearExitGlow", [0.0, 3.0, 60.0],
             {"color": [1.0, 0.78, 0.5], "offset": [0.0, 0.0, 0.0], "intensity": 5.0, "radius": 24.0, "decay": 0.02}),
        ],
    },
}


def build(cfg):
    rng = random.Random(cfg["seed"])
    name = cfg["name"]
    objects = []

    def obj(oname, cls, t, r=None, s=None, model=None, **extra):
        o = {"name": oname, "class": cls, "translate": [round(v, 4) for v in t],
             "rotate": r or [0.0, 0.0, 0.0, 1.0], "scale": s or [1.0, 1.0, 1.0]}
        if model:
            o["model"] = model
        o.update(extra)
        objects.append(o)

    zs = []
    z = cfg["z_start"]
    while z <= cfg["z_end"] + 1e-3:
        zs.append(z)
        z += TILE

    # 床
    for zi, z in enumerate(zs):
        for xi, x in enumerate(FLOOR_XS):
            obj(f"{name}Floor_{xi}_{zi}", "Ground", [x, FLOOR_Y, z], s=SCALE, model="ModularFloor")

    # 壁（下段・上段）と、上段に載せる張り出しの岩。隣と同じ形が続かないようにずらす
    for side, x, rot in (("R", WALL_X, ROT_RIGHT), ("L", -WALL_X, ROT_LEFT)):
        prev = None
        for zi, z in enumerate(zs):
            obj(f"{name}Wall{side}_{zi}", "Ground", [x, WALL_Y, z], r=rot, s=SCALE, model="ModularStoneWall")
            obj(f"{name}WallTop{side}_{zi}", "Ground", [x, WALL_TOP_Y, z], r=rot, s=SCALE, model="ModularStoneWall_top")
            m = rng.choice([o for o in OVERHANGS if o != prev])
            prev = m
            obj(f"Cave_Overhang_{name}{side}_{zi}", "Ground", [x, WALL_TOP_Y, z], r=rot, s=SCALE, model=m)

    # 左右の松明。対にして奥へ並べる（奥のものは霧に沈む）
    for i, z in enumerate(cfg["torch_zs"]):
        for side, x, rot in (("R", WALL_X - WALL_FACE_INSET, ROT_RIGHT), ("L", -(WALL_X - WALL_FACE_INSET), ROT_LEFT)):
            obj(f"Cave_{name}Torch{side}_{i}", "WallTorch", [x, FLOOR_TOP + 2.0, z], r=rot, model="Torch_wall",
                light=dict(TORCH_LIGHT))

    # 壁の根元の岩（低いので当たり判定なし）
    for i, (side, z) in enumerate(cfg["rocks"]):
        sign = 1.0 if side == "R" else -1.0
        obj(f"Cave_{name}Rock_{i}", "Ground", [sign * (WALL_X - 1.4), FLOOR_TOP - 0.1, z],
            r=[0.0, 0.7071068, 0.0, 0.7071068], s=[2.0, 2.0, 2.0], model="Cave/CaveRockCluster_01")

    # 光
    for entry in cfg["spot_lights"]:
        oname, t, params = entry[0], entry[1], entry[2]
        rot = entry[3] if len(entry) > 3 else None
        obj(oname, "SpotLight", t, r=rot, spotLight=params)
    for oname, t, params in cfg["point_lights"]:
        obj(oname, "PointLight", t, light=params)

    data = {"format": "GuchisStage", "version": 2, "environment": cfg["environment"], "objects": objects}
    text = json.dumps(data, ensure_ascii=False, indent=2).replace("\n", "\r\n") + "\r\n"
    with open(cfg["out"], "w", encoding="utf-8", newline="") as f:
        f.write(text)
    print(f"{cfg['out']}: {len(objects)} objects")


targets = sys.argv[1:] or list(CONFIGS.keys())
for key in targets:
    build(CONFIGS[key])

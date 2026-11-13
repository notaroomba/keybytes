# Builds spacer-*.kicad_pcb from hardware/keybit/keybit.kicad_pcb (read only, loaded from a copy).
# Run with the KiCad python: python make_spacer.py /path/to/copy/of/keybit.kicad_pcb
import sys, shutil, pcbnew
from pcbnew import VECTOR2I, FromMM as mm

SRC = sys.argv[1]
PITCH = 20.55                     # key pitch (19.05 + 1.5 seam)
X_W, X_FW, X_FE, X_E = 160.15, 165.15, 179.95, 184.95   # keybit W edge, fold lines, E edge
Y_N, Y_S = 70.3, 85.1             # N/S fold lines (= W/E flap top/bottom)
Y_NE, Y_SE = 65.3, 90.1           # N/S flap outer edges
KB_XC = 172.55                    # keybit centre x (N/S pads sit at +-1.19/3.57/5.95 from it)
PAD_EDGE = 0.5                    # min gap from an N/S pad edge to the flap side edge (keybit has 0.95)
VIA_DY = 1.2                      # N/S pad -> via offset toward the body (via stays >= 1 mm off the fold)
HOLE_D = 1.7
JOG = 0.35                        # inner traces step away from the peg hole

def build(u):
    d = (1 - u) * PITCH           # centre section shrinks by the pitch difference
    b = pcbnew.LoadBoard(SRC)
    kb = {f.GetReference(): f for f in b.GetFootprints()}
    w_tps = sorted([f for f in kb.values() if f.GetPosition().x < mm(X_FW) and f.GetReference().startswith("TP")], key=lambda f: f.GetPosition().y)
    e_tps = sorted([f for f in kb.values() if f.GetPosition().x > mm(X_FE) and f.GetReference().startswith("TP")], key=lambda f: f.GetPosition().y)
    n_tps = sorted([f for f in kb.values() if f.GetPosition().y < mm(Y_N) and f.GetReference().startswith("TP")], key=lambda f: f.GetPosition().x)
    s_tps = sorted([f for f in kb.values() if f.GetPosition().y > mm(Y_S) and f.GetReference().startswith("TP")], key=lambda f: f.GetPosition().x)
    stiff = [s for s in b.GetDrawings() if s.GetLayerName() == "User.Drawings" and s.GetShapeStr() == "Rect"
             and (s.GetBoundingBox().GetRight() < mm(X_FW) or s.GetBoundingBox().GetX() > mm(X_FE)
                  or s.GetBoundingBox().GetBottom() < mm(Y_N) or s.GetBoundingBox().GetY() > mm(Y_S))]
    src = b
    b = pcbnew.CreateEmptyBoard()               # fresh board, keybit stackup/rules copied over
    b.SetDesignSettings(src.GetDesignSettings())
    b.SetEnabledLayers(src.GetEnabledLayers()); b.SetCopperLayerCount(src.GetCopperLayerCount())
    b.SetLayerName(pcbnew.User_1, src.GetLayerName(pcbnew.User_1))
    keep_fp = []
    half = (X_FE - d - X_FW) / 2                # half width of the body = half width of the N/S flaps
    fits = lambda f: abs(f.GetPosition().x / 1e6 - KB_XC) + 0.5 + PAD_EDGE <= half
    ns_idx = [i for i, f in enumerate(n_tps, 1) if fits(f)]     # keybit column numbers that fit
    for f, side in [(f, "W") for f in w_tps] + [(f, "E") for f in e_tps] + \
                   [(f, "N") for f in n_tps if fits(f)] + [(f, "S") for f in s_tps if fits(f)]:
        g = pcbnew.Cast_to_FOOTPRINT(f.Duplicate(False)); g.Pads()[0].SetNetCode(0); b.Add(g); keep_fp.append((g, side))
    w_tps, e_tps, n_tps, s_tps = ([g for g, sd in keep_fp if sd == k] for k in "WENS")
    copies = []
    for s0 in stiff:
        c = pcbnew.Cast_to_PCB_SHAPE(s0.Duplicate()); b.Add(c); copies.append(c)
    stiff = copies
    # east end moves left by d
    # N/S pads stay centred on the body, N/S flap stiffeners lose d on their east side
    for f, side in keep_fp:
        if side == "E": f.Move(VECTOR2I(-mm(d), 0))
        if side in "NS": f.Move(VECTOR2I(-mm(d / 2), 0))
    for s in stiff:
        bb = s.GetBoundingBox()
        if bb.GetX() > mm(X_FE): s.Move(VECTOR2I(-mm(d), 0))
        elif bb.GetBottom() < mm(Y_N) or bb.GetY() > mm(Y_S): s.SetEnd(VECTOR2I(s.GetEnd().x - mm(d), s.GetEnd().y))
    xfe, xe = X_FE - d, X_E - d
    xc, yc = (X_W + xe) / 2, (Y_N + Y_S) / 2

    def shape(layer, kind, pts=None, w=0.05, r=None):
        s = pcbnew.PCB_SHAPE(b)
        s.SetLayer(b.GetLayerID(layer)); s.SetWidth(mm(w))
        if kind == "rect":
            s.SetShape(pcbnew.SHAPE_T_RECT)
        elif kind == "circle":
            s.SetShape(pcbnew.SHAPE_T_CIRCLE)
        else:
            s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        if kind == "circle":
            s.SetCenter(VECTOR2I(mm(pts[0]), mm(pts[1]))); s.SetEnd(VECTOR2I(mm(pts[0] + r), mm(pts[1])))
        else:
            s.SetStart(VECTOR2I(mm(pts[0]), mm(pts[1]))); s.SetEnd(VECTOR2I(mm(pts[2]), mm(pts[3])))
        b.Add(s)

    cross = [(X_W, Y_N), (X_FW, Y_N), (X_FW, Y_NE), (xfe, Y_NE), (xfe, Y_N), (xe, Y_N), (xe, Y_S),
             (xfe, Y_S), (xfe, Y_SE), (X_FW, Y_SE), (X_FW, Y_S), (X_W, Y_S)]    # keybit cross outline
    for p, q in zip(cross, cross[1:] + cross[:1]):
        shape("Edge.Cuts", "line", p + q)
    shape("Edge.Cuts", "circle", (xc, yc), r=HOLE_D / 2)
    for x in (X_FW, xfe):                      # fold lines
        shape("User.1", "line", (x, Y_N, x, Y_S))
    for y in (Y_N, Y_S):
        shape("User.1", "line", (X_FW, y, xfe, y))
    # centre stiffener, 0.7 mm inset from all four folds like the keybit body stiffener, peg cutout +0.4 mm radius
    if xfe - X_FW - 1.4 > HOLE_D + 0.8:
        shape("User.Drawings", "rect", (X_FW + 0.7, Y_N + 0.7, xfe - 0.7, Y_S - 0.7), w=0.1)
        shape("User.Drawings", "circle", (xc, yc), w=0.1, r=HOLE_D / 2 + 0.4)
    def track(pts, layer, n):
        for p, q in zip(pts, pts[1:]):
            t = pcbnew.PCB_TRACK(b)
            t.SetStart(VECTOR2I(mm(p[0]), mm(p[1]))); t.SetEnd(VECTOR2I(mm(q[0]), mm(q[1])))
            t.SetWidth(mm(0.4)); t.SetLayer(layer); t.SetNet(n); b.Add(t)
    # Two nets. Every Keybit side reads A,B,A,B,A,B clockwise and each side's A/B pair feeds its own FET bridge,
    # so neighbours don't care which polarity they get, only that their A and B never touch. Spacer pads therefore
    # alternate SPX/SPY along every face (W/E lines and N/S columns by parity); any whole-pad stagger keeps that.
    # W pad n -> E pad n at the same height on B.Cu, N pad -> S pad on F.Cu, vias where same-net lines cross,
    # plus a F.Cu spine on each W/E flap so the lines that only cross near the peg hole still join.
    spx, spy = pcbnew.NETINFO_ITEM(b, "SPX"), pcbnew.NETINFO_ITEM(b, "SPY"); b.Add(spx); b.Add(spy)
    cls = lambda i: spx if i % 2 else spy
    def via(x, y, n):
        v = pcbnew.PCB_VIA(b); v.SetPosition(VECTOR2I(mm(x), mm(y))); v.SetDrill(mm(0.3)); v.SetWidth(mm(0.55))
        v.SetNet(n); b.Add(v)
    rows, cols = [], []
    for i, (fw, fe) in enumerate(zip(w_tps, e_tps), 1):
        n = cls(i); fw.Pads()[0].SetNet(n); fe.Pads()[0].SetNet(n)
        a, z = fw.Pads()[0].GetPosition(), fe.Pads()[0].GetPosition()
        ya, yz = a.y / 1e6, z.y / 1e6
        rows.append((ya, n, fw, fe))
        pts = [(a.x / 1e6, ya)]
        off = ya - yc
        if abs(off) < 2:                       # trace next to the peg hole: jog away
            ny = ya + (JOG if off > 0 else -JOG)
            x0 = xc - HOLE_D / 2 - 0.3 - JOG
            pts += [(x0 - JOG, ya), (x0, ny), (2 * xc - x0, ny), (2 * xc - x0 + JOG, yz)]
        pts.append((z.x / 1e6, yz))
        track(pts, pcbnew.B_Cu, n)
    for i, fn, fs in zip(ns_idx, n_tps, s_tps):
        n = cls(i); fn.Pads()[0].SetNet(n); fs.Pads()[0].SetNet(n)
        a, z = fn.Pads()[0].GetPosition(), fs.Pads()[0].GetPosition()
        x, ya, yz = a.x / 1e6, a.y / 1e6, z.y / 1e6
        cols.append((x, n, i, fn, fs))
        va, vz = ya + VIA_DY, yz - VIA_DY
        track([(x, ya), (x, va)], pcbnew.B_Cu, n); track([(x, vz), (x, yz)], pcbnew.B_Cu, n)
        via(x, va, n); via(x, vz, n)
        pts = [(x, va)]
        off = x - xc
        if abs(off) < 2:                       # same peg-hole jog as E-W, turned 90 degrees
            nx = x + (JOG if off > 0 else -JOG)
            y0 = yc - HOLE_D / 2 - 0.3 - JOG
            pts += [(x, y0 - JOG), (nx, y0), (nx, 2 * yc - y0), (x, 2 * yc - y0 + JOG)]
        pts.append((x, vz))
        track(pts, pcbnew.F_Cu, n)
    # crossing vias, only where both traces are straight (jogs live within ~1.9 mm of the peg)
    for x, n, *_ in cols:
        for y, m, *_ in rows:
            if n is m and (abs(x - xc) >= 2 or abs(y - yc) >= 2.3) and (abs(y - yc) >= 2 or abs(x - xc) >= 2.3):
                via(x, y, n)
    # flap spines: SPX on the W flap, SPY on the E flap, halfway between the pad edge and the fold
    for xs, n in (((X_W + 1.4 + 0.5 + X_FW) / 2, spx), ((xe - 1.4 - 0.5 + xfe) / 2, spy)):
        ys = [y for y, m, *_ in rows if m is n]
        for y in ys: via(xs, y, n)
        track([(xs, ys[0]), (xs, ys[-1])], pcbnew.F_Cu, n)
    # label
    tx = pcbnew.PCB_TEXT(b); tx.SetText(f"spacer {u}u"); tx.SetLayer(pcbnew.Cmts_User)
    tx.SetPosition(VECTOR2I(mm(xc), mm(Y_SE + 2))); b.Add(tx)
    name = f"spacer-{u}u"
    b.Save(f"{name}.kicad_pcb")
    return name, d, xe - X_W, xfe - X_FW, [("W/E line y=%.2f" % y, n.GetNetname(), fw.GetReference(), fe.GetReference()) for y, n, fw, fe in rows] + \
        [("N/S col %d x=%.3f" % (i, x), n.GetNetname(), fn.GetReference(), fs.GetReference(), "edge gap %.3f" % (half - abs(x - xc) - 0.5)) for x, n, i, fn, fs in cols]

for u in (1, 0.75, 0.5):
    name, d, L, c, m = build(u)
    print(name, "length %.4f centre %.4f" % (L, c)); [print("  ", x) for x in m]

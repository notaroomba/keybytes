# KiKit layout plugin: test panel. Boards plus copper-free side strips make the board area exactly
# PANEL_W wide (JLC's 5 mm rails make it 79). Keyword sits in the bottom row so its USB-C faces out.
from kikit.plugin import LayoutPlugin, TabsPlugin
from kikit.units import mm
from pcbnew import VECTOR2I
from kikit.panelize import Origin
from shapely.geometry import LineString, box
from shapely.ops import unary_union

H, P, KW = "src/keybit-hall.kicad_pcb", "src/keybit/keybit.kicad_pcb", "src/keyword/keyword.kicad_pcb"
ROWS = [[P, P], [P, P], [KW, H]]
PANEL_W = 69.0 * mm
GAP = 4.514286 * mm  # strip to board gap, same as hspace
KW_PREFIX = "KW_"
KB_PREFIX = "KB_"  # plain keybit refs collide with hall refs too
TAB_W = 1 * mm
MIN_CLEAR = 0.08 * mm   # tab edge to edge-pad edge (keybit centre tab has 0.19)

class Layout(LayoutPlugin):
    def buildLayout(self, panel, inputFile, sourceArea):
        net = lambda n, o: self.netPattern.format(n=n, orig=o)
        ref = lambda n, o: self.refPattern.format(n=n, orig=o)
        kwRef = lambda n, o: KW_PREFIX + o  # keyword refs collide with hall refs; fab.py maps them back
        kbRef = lambda n, o: KB_PREFIX + o
        y, first = 0, True
        for row in ROWS:
            x, h = 0, 0
            for f in row:
                # 3 mm tolerance keeps the keyword USB-C (overhangs south edge by ~2 mm)
                bb = panel.appendBoard(f, VECTOR2I(x, y), origin=Origin.TopLeft,
                                       tolerance=3 * mm, netRenamer=net,
                                       refRenamer=kwRef if f == KW else kbRef if f == P else ref,
                                       bakeRef=f in (KW, P),  # silkscreen keeps R1, not KW_R1
                                       inheritDrc=first and f in (H, P))  # .kicad_dru files are identical
                first = first and f not in (H, P)
                x += bb.GetWidth() + self.hspace
                h = max(h, bb.GetHeight())
            y += h + self.vspace
        return panel.substrates

class Tabs(TabsPlugin):
    # One 1 mm tab per pair of neighbouring flaps, placed where it clears the edge pads of both
    # boards the most (between pads, like the keybit centre tab); ties go to the board centre.
    def buildTabs(self, panel):
        subs = list(panel.substrates)
        x0 = min(a.bounds()[0] for a in subs); x1 = max(a.bounds()[2] for a in subs)
        y0 = min(a.bounds()[1] for a in subs); y1 = max(a.bounds()[3] for a in subs)
        cx, gap = (x0 + x1) / 2, GAP
        strips = [Strip(cx - PANEL_W / 2, y0, x0 - gap, y1), Strip(x1 + gap, y0, cx + PANEL_W / 2, y1)]
        subs += strips
        pads = [p.GetPosition() for p in panel.board.GetPads()]
        tabs = []
        for a in subs:
            for b in subs:
                ax0, ay0, ax1, ay1 = a.bounds(); bx0, by0, bx1, by1 = b.bounds()
                if 0 < by0 - ay1 < 6 * mm and min(ax1, bx1) > max(ax0, bx0):    # b below a
                    t = self._place(a, b, max(ax0, bx0), min(ax1, bx1), ay1, by0, (max(ax0, bx0) + min(ax1, bx1)) / 2, pads, True)
                elif 0 < bx0 - ax1 < 6 * mm and min(ay1, by1) > max(ay0, by0):  # b right of a
                    t = self._place(a, b, max(ay0, by0), min(ay1, by1), ax1, bx0, (max(ay0, by0) + min(ay1, by1)) / 2, pads, False)
                else:
                    continue
                if t is not None:
                    tabs.append(t)
        panel.boardSubstrate.union(unary_union([s.substrates for s in strips] + tabs))
        return []

    @staticmethod
    def _place(a, b, lo, hi, e0, e1, centre, pads, vert):
        P = (lambda u, v: (u, v)) if vert else (lambda u, v: (v, u))
        eps = 0.01 * mm
        near = [(p.x if vert else p.y) for p in pads
                if abs((p.y if vert else p.x) - e0) < 2 * mm or abs((p.y if vert else p.x) - e1) < 2 * mm]
        near = [c for c in near if lo - 2 * mm < c < hi + 2 * mm]
        best = None
        for i in range(int(lo), int(hi), int(0.01 * mm)):
            # tab edge plus 0.3 mm must sit on straight flap edge of both boards
            ea = LineString([P(i - TAB_W / 2 - 0.3 * mm, e0 - eps), P(i + TAB_W / 2 + 0.3 * mm, e0 - eps)])
            eb = LineString([P(i - TAB_W / 2 - 0.3 * mm, e1 + eps), P(i + TAB_W / 2 + 0.3 * mm, e1 + eps)])
            if not (ea.within(a.substrates) and eb.within(b.substrates)):
                continue
            clr = min([abs(i - c) - 0.5 * mm - TAB_W / 2 for c in near] or [9 * mm])  # edge pads are 1 mm
            key = (round(clr / 1e4), -abs(i - centre))
            if clr >= MIN_CLEAR and (best is None or key > best[0]):
                best = (key, i)
        if best is None:
            return None
        i = best[1]
        return box(*P(i - TAB_W / 2, e0 - eps), *P(i + TAB_W / 2, e1 + eps)) if vert else \
               box(e0 - eps, i - TAB_W / 2, e1 + eps, i + TAB_W / 2)


class Strip:
    # plain flex filler bar, no copper; only there to pad the board area to PANEL_W
    def __init__(self, *b):
        self.substrates = box(*b)
    def bounds(self):
        return self.substrates.bounds

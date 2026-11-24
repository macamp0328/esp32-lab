"""p006 crystal ball, phase 2 (layout B): the printed parts as a parametric Fusion design.

Mirrors ../../cad/p006_parts.scad (same layout, clearances and defaults); keep the two in step.
Run it in Fusion: Utilities > Add-Ins > Scripts and Add-Ins > "+" > pick this folder > Run.
- No p006 design active: builds a new, unsaved design in which every measurement is a user
  parameter (Modify > Change Parameters), then exports.
- The p006 design active (it has the user parameter holder_l): exports only.
Exports go next to this folder: ../stl/p006-<part>.stl (print orientation, like the .scad's)
and ../png/<view>.png.
"""

import math
import os
import struct

import adsk.core
import adsk.fusion

try:
    OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
except NameError:  # run from a string; the caller passes out_dir
    OUT = ""

VI = adsk.core.ValueInput.createByString
P3 = adsk.core.Point3D.create
NEW = adsk.fusion.FeatureOperations.NewBodyFeatureOperation
JOIN = adsk.fusion.FeatureOperations.JoinFeatureOperation
CUT = adsk.fusion.FeatureOperations.CutFeatureOperation
H = adsk.fusion.DimensionOrientations.HorizontalDimensionOrientation
V = adsk.fusion.DimensionOrientations.VerticalDimensionOrientation

# fmt: off
# (name, expression, unit, comment); "MEASURE" = a guess until the part is in hand
PARAMS = [
    ("clr", "0.25 mm", "mm", "gap per side between a printed part and a bought one"),
    # 18650 holder
    ("holder_l", "78 mm", "mm", "MEASURE holder: outside length over the end walls"),
    ("holder_w", "21 mm", "mm", "MEASURE holder: outside width"),
    ("holder_h", "20 mm", "mm", "MEASURE holder: underside to the top of the end walls"),
    ("holder_side_h", "13 mm", "mm", "MEASURE holder: underside to the top of the long side walls"),
    ("holder_base", "1.5 mm", "mm", "MEASURE holder: underside to the cell's lowest point"),
    ("lead_gap", "3 mm", "mm", "room at each end of the holder for its leads"),
    ("plus_left", "1", "", "1: the holder's + contact is at the left end, seen from the front; 0: right"),
    # cell, cable
    ("cell_d", "18.6 mm", "mm", "MEASURE cell diameter: protected 18650s run 18.3 to 18.8"),
    ("cell_l", "70 mm", "mm", "cell length"),
    ("plug_w", "12.5 mm", "mm", "MEASURE USB-C cable: width of the plug's moulded body"),
    ("plug_h", "7 mm", "mm", "MEASURE USB-C cable: height of the plug's moulded body"),
    # s3mini (Waveshare ESP32-S3-Zero, vendor drawing)
    ("s3_w", "18 mm", "mm", "s3mini board width"),
    ("s3_l", "23.5 mm", "mm", "s3mini board length"),
    ("s3_pcb_t", "1 mm", "mm", "MEASURE s3mini board thickness"),
    ("s3_usb_over", "0.6 mm", "mm", "MEASURE how far the USB-C shell sticks out past the board edge"),
    ("s3_usb_w", "9 mm", "mm", "USB-C shell width"),
    ("s3_usb_h", "3.2 mm", "mm", "USB-C shell height"),
    ("s3_usb_d", "7.3 mm", "mm", "USB-C shell depth"),
    ("s3_row", "15.24 mm", "mm", "header rows, centre to centre"),
    ("s3_hdr", "2.5 mm", "mm", "header plastic under the board"),
    ("s3_pin", "6 mm", "mm", "header pins below the plastic"),
    ("s3_offset", "14 mm", "mm", "s3mini centre right of the holder centre, at least"),
    ("holder_plinth", "0 mm", "mm", "lifts the holder and the s3mini; 8 makes room for Dupont sockets"),
    # perfboard
    ("perf_l", "40 mm", "mm", "MEASURE perfboard length after cutting"),
    ("perf_w", "20 mm", "mm", "MEASURE perfboard width after cutting"),
    ("perf_t", "1.6 mm", "mm", "perfboard thickness"),
    ("perf_standoff", "3 mm", "mm", "room for solder joints under the perfboard"),
    # box
    ("wall", "2 mm", "mm", "box wall"),
    ("floor_t", "2 mm", "mm", "box floor"),
    ("lid_t", "2 mm", "mm", "lid plate"),
    ("head", "6 mm", "mm", "room above the holder: fingers, and the + and - marks"),
    ("r_out", "3 mm", "mm", "outside corner radius"),
    ("skirt_h", "3 mm", "mm", "the lid's lip inside the walls"),
    ("skirt_t", "1.6 mm", "mm", "lid lip thickness"),
    ("part_t", "1.6 mm", "mm", "posts, rails and brackets inside the box"),
    ("cable_d", "4.5 mm", "mm", "MEASURE the 4-wire cable bundled 2 x 2"),
    ("cable_x", "8 mm", "mm", "cable notch, from the inside of the left wall"),
    ("tab_len", "1.5 mm", "mm", "the lid's tab that closes the notch over the cable"),
    ("pir_d", "10.5 mm", "mm", "AM312 window; the feature 'PIR window' is suppressed: unsuppress it for one"),
    # pot and ring
    ("drain_d", "38.1 mm", "mm", "MEASURE drain hole (1.5 in)"),
    ("drain_clr", "0.5 mm", "mm", "concrete is rough"),
    ("pot_floor", "10 mm", "mm", "MEASURE floor thickness at the hole"),
    ("ring_z", "30 mm", "mm", "MEASURE the ring's LED face above the pot floor; the hook tip is ~38"),
    ("ring_d", "28.7 mm", "mm", "LuMini 1 in ring body"),
    ("ring_hole", "19.3 mm", "mm", "LuMini ring hole"),
    ("ring_tabs", "43.2 mm", "mm", "across the ring's two outer tabs"),
    ("ring_tab_w", "9 mm", "mm", "MEASURE width of an outer tab"),
    ("ring_t", "1 mm", "mm", "MEASURE ring board thickness"),
    ("ring_proud", "0.4 mm", "mm", "the ring's face sits this far above the seat"),
    ("post_r", "8 mm", "mm", "ring post shaft radius"),
    # feet
    ("foot_d", "16 mm", "mm", "foot diameter"),
    ("foot_h", "3 mm", "mm", "foot height"),
    ("feet_n", "4", "", "number of feet"),
    # ---- derived: box (inside coordinates: x from the left wall, y from the front wall, z from the floor)
    ("inner_x", "holder_l + 2 * lead_gap", "mm", "derived"),
    ("hx0", "lead_gap", "mm", "derived: holder's left end"),
    ("perf_x0", "0.5 mm + part_t + clr", "mm", "derived"),
    ("s3_cx", "max(inner_x / 2 + s3_offset; perf_x0 + perf_l + 2 * (clr + part_t) + 0.5 mm + s3_w / 2)", "mm", "derived: s3mini centre"),
    ("s3_y0", "0.3 mm + part_t + clr", "mm", "derived: s3mini antenna end"),
    ("s3_y1", "s3_y0 + s3_l", "mm", "derived: s3mini USB end, facing the holder"),
    ("hy0", "s3_y1 + clr + part_t + clr", "mm", "derived: holder's front face"),
    ("inner_y", "hy0 + holder_w + clr", "mm", "derived"),
    ("inner_h", "holder_plinth + holder_h + head", "mm", "derived"),
    ("box_x", "inner_x + 2 * wall", "mm", "derived: box outside"),
    ("box_y", "inner_y + 2 * wall", "mm", "derived: box outside"),
    ("box_z", "floor_t + inner_h", "mm", "derived: box height without the lid"),
    ("usb_z", "holder_plinth + holder_side_h + plug_h / 2 + 0.6 mm", "mm", "derived: port centre, the plug clears the holder's sides"),
    ("s3_z", "usb_z - s3_usb_h / 2 - s3_pcb_t", "mm", "derived: s3mini underside"),
    ("s3_top", "s3_z + s3_pcb_t + 0.8 mm", "mm", "derived: cradle post height"),
    ("s3_e", "s3_w / 2 + clr", "mm", "derived: cradle posts' inside face from the board's centre line"),
    ("cell_zc", "holder_plinth + holder_base + cell_d / 2", "mm", "derived"),
    ("cell_top", "cell_zc + cell_d / 2", "mm", "derived"),
    ("perf_y0", "(hy0 - clr - part_t - (perf_w + 2 * (clr + part_t))) / 2 + part_t + clr", "mm", "derived"),
    ("perf_h", "perf_standoff + perf_t + 1 mm", "mm", "derived: perfboard corner wall height"),
    ("lip_z", "perf_standoff + perf_t + 0.1 mm", "mm", "derived: snap lip underside"),
    ("mark_s", "min(5 mm; head - 1 mm)", "mm", "derived: + and - size"),
    ("mark_z", "floor_t + holder_plinth + holder_h + head / 2", "mm", "derived"),
    ("plus_x", "plus_left * (hx0 + 7 mm) + (1 - plus_left) * (inner_x - hx0 - 7 mm)", "mm", "derived"),
    ("minus_x", "plus_left * (inner_x - hx0 - 7 mm) + (1 - plus_left) * (hx0 + 7 mm)", "mm", "derived"),
    ("skirt_r", "max(r_out - wall - clr; 0.3 mm)", "mm", "derived"),
    ("kz", "inner_h - (usb_z + s3_usb_h / 2 + 0.3 mm)", "mm", "derived: keeper length, stops 0.3 over the USB-C shell"),
    # ---- derived: plug and post (pot coordinates: z = 0 on the pot floor, centred on the drain hole)
    ("spigot_r", "drain_d / 2 - drain_clr", "mm", "derived"),
    ("spigot_l", "min(8 mm; pot_floor - 1 mm)", "mm", "derived"),
    ("flange_r", "drain_d / 2 + 6 mm", "mm", "derived"),
    ("flange_t", "2.5 mm", "mm", "plug flange"),
    ("sock_bot", "-spigot_l + 1.5 mm", "mm", "derived: socket floor"),
    ("post_flat", "post_r - 1.5 mm", "mm", "derived: the flat keys the post's groove to the base's"),
    ("z_ru", "ring_z - ring_t", "mm", "derived: ring underside"),
    ("z_face", "ring_z - ring_proud", "mm", "derived: seat face"),
    ("relief", "3 mm", "mm", "under the ring: solder joints and wire bends"),
    ("plate_floor", "2 mm", "mm", "seat plate under the relief"),
    ("z_pb", "z_ru - relief - plate_floor", "mm", "derived: seat plate underside"),
    ("plate_r", "ring_d / 2 + clr + 1.6 mm", "mm", "derived"),
    ("arm_w", "ring_tab_w + 2 * clr + 3.2 mm", "mm", "derived"),
    ("arm_l", "ring_tabs / 2 - 1 mm", "mm", "derived: tab tips stick out 1 mm past the arms"),
    ("cone_h", "plate_r - post_flat", "mm", "derived: 45 degrees under the seat"),
    ("gus_h", "arm_l - 5 mm", "mm", "derived: 45 degrees under the arms"),
]
# fmt: on

# the .scad's asserts, on parameter values in mm. (Read the parameters' values: right after a
# change, unitsManager.evaluateExpression can still return stale values two dependencies deep.)
CHECKS = [
    (
        lambda p: p["usb_z"] - 1.3 <= p["cell_top"] - 2,
        "holder side walls too tall: a plug would pass over the cell. File a notch plug_w + 1 wide in both side walls and set holder_side_h to its depth",
    ),
    (
        lambda p: abs(p["s3_cx"] - p["inner_x"] / 2) + p["plug_w"] / 2 + 2 <= p["cell_l"] / 2 - 5,
        "the s3mini's port no longer faces the cell's body: shorten perf_l or lower s3_offset",
    ),
    (
        lambda p: p["s3_z"] - p["s3_hdr"] - p["s3_pin"] >= 0.5,
        "header pins hit the floor: raise holder_plinth",
    ),
    (
        lambda p: p["s3_cx"] + p["s3_w"] / 2 + p["clr"] + p["part_t"] <= p["inner_x"],
        "the s3mini does not fit beside the perfboard: shorten perf_l",
    ),
    (lambda p: p["box_x"] <= 175 and p["box_y"] <= 175, "box larger than the A1 mini's bed"),
    (
        lambda p: p["z_pb"] - max(p["cone_h"], p["gus_h"]) >= p["flange_t"] + 0.5,
        "ring_z too low: the post's 45-degree seat no longer fits; print it with supports",
    ),
    (lambda p: p["spigot_l"] >= 3, "pot_floor too thin for the spigot"),
]


def _p(e):
    """Parenthesise a compound expression."""
    e = str(e).strip()
    return f"({e})" if (" " in e and not (e.startswith("(") and e.endswith(")"))) else e


def add(a, b):
    if str(a).strip() in ("0", "0 mm"):
        return str(b)
    if str(b).strip() in ("0", "0 mm"):
        return str(a)
    return f"{a} + {_p(b)}"


def sub(a, b):
    return f"{a} - {_p(b)}"


def neg(e):
    e = str(e).strip()
    if e.startswith("-(") and e.endswith(")"):
        return e[2:-1]
    if e.startswith("-") and " " not in e:
        return e[1:]
    return f"-{_p(e)}"


class Part:
    """Builds the features of one component; every length is a Fusion expression in mm."""

    def __init__(self, design, comp):
        self.design = design
        self.comp = comp
        self.um = design.unitsManager
        self._reg = {}
        self.coincidences = []  # different expressions that happen to be equal: linked, check them

    def v(self, expr):  # internal units (cm)
        return self.um.evaluateExpression(str(expr), "mm")

    def body(self):
        return self.comp.bRepBodies.item(0)

    # ---- sketches
    def sketch(self, plane, name):
        pl = {
            "xy": self.comp.xYConstructionPlane,
            "xz": self.comp.xZConstructionPlane,
            "yz": self.comp.yZConstructionPlane,
        }[plane]
        sk = self.comp.sketches.add(pl)
        sk.name = name
        sk.isComputeDeferred = True
        return sk

    @staticmethod
    def _ax(sk):
        """Model axes (0 x, 1 y, 2 z) along the sketch's x and y."""
        xd, yd = sk.xDirection.asArray(), sk.yDirection.asArray()
        return max(range(3), key=lambda i: abs(xd[i])), max(range(3), key=lambda i: abs(yd[i]))

    def _pt(self, sk, p):
        return sk.modelToSketchSpace(P3(*[self.v(e) for e in p]))

    def _dim(self, sk, spt, expr, orient):
        """Pin a sketch point's model coordinate `expr` along one sketch axis, from the origin.

        Fusion refuses a second origin dimension of the same value and direction in one sketch
        (over-constrained), so a point that shares a coordinate with an earlier one is aligned
        with that point instead.
        """
        val = self.v(expr)
        g = spt.geometry
        gc = sk.geometricConstraints
        align = gc.addVerticalPoints if orient == H else gc.addHorizontalPoints
        if abs(val) < 1e-7:
            align(sk.originPoint, spt)
            return
        reg = self._reg.setdefault(id(sk), (sk, {}))[1]  # holding sk keeps its id unique
        mag = str(expr) if val > 0 else neg(expr)  # the dimension's (positive) expression
        same, mirror = (orient, round(val, 6)), (orient, round(-val, 6))
        tp = P3(g.x + 0.25, g.y + 0.25, 0)
        if same in reg:
            first_mag, first_pt = reg[same]
            if first_mag != mag:
                self.coincidences.append(f"{sk.name}: {mag} = {first_mag}")
            align(first_pt, spt)
            return
        if mirror in reg:  # the same distance on the other side: dimension the pair
            first_mag, first_pt = reg[mirror]
            if first_mag != mag:
                self.coincidences.append(f"{sk.name}: {mag} = {first_mag} (mirrored)")
            d = sk.sketchDimensions.addDistanceDimension(first_pt, spt, orient, tp)
            d.parameter.expression = f"2 * {_p(mag)}"
            reg[same] = (mag, spt)
            return
        reg[same] = (mag, spt)
        d = sk.sketchDimensions.addDistanceDimension(sk.originPoint, spt, orient, tp)
        d.parameter.expression = mag

    def _place(self, sk, spt, p):
        ix, iy = self._ax(sk)
        self._dim(sk, spt, p[ix], H)
        self._dim(sk, spt, p[iy], V)

    def _corners(self, sk, p0, size):
        ix, iy = self._ax(sk)
        a0, b0 = p0[ix], p0[iy]
        a1, b1 = add(a0, size[ix]), add(b0, size[iy])

        def pt(a, b):
            q = ["0 mm", "0 mm", "0 mm"]
            q[ix], q[iy] = a, b
            return tuple(q)

        return [pt(a0, b0), pt(a1, b0), pt(a1, b1), pt(a0, b1)]

    def poly(self, sk, pts):
        """Closed polygon through model points (x, y, z expressions); every vertex dimensioned."""
        lines = sk.sketchCurves.sketchLines
        q = [self._pt(sk, p) for p in pts]
        segs = [lines.addByTwoPoints(q[0], q[1])]
        for k in range(2, len(q)):
            segs.append(lines.addByTwoPoints(segs[-1].endSketchPoint, q[k]))
        segs.append(lines.addByTwoPoints(segs[-1].endSketchPoint, segs[0].startSketchPoint))
        verts = [segs[0].startSketchPoint] + [s.endSketchPoint for s in segs[:-1]]
        for spt, p in zip(verts, pts, strict=True):
            self._place(sk, spt, p)
        return segs

    def rect(self, sk, p0, size):
        return self.poly(sk, self._corners(sk, p0, size))

    def rrect(self, sk, p0, size, r):
        """Rectangle with all four corners rounded to r."""
        c = self._corners(sk, p0, size)
        q = [self._pt(sk, p) for p in c]
        lines = sk.sketchCurves.sketchLines
        L = [lines.addByTwoPoints(q[k], q[(k + 1) % 4]) for k in range(4)]
        gc = sk.geometricConstraints
        for k in range(4):  # join the ends, then square them up
            gc.addCoincident(L[k].endSketchPoint, L[(k + 1) % 4].startSketchPoint)
        gc.addHorizontal(L[0])
        gc.addHorizontal(L[2])
        gc.addVertical(L[1])
        gc.addVertical(L[3])
        rv = self.v(r)
        arcs = sk.sketchCurves.sketchArcs
        A = []
        for k in range(4):
            a, b = L[k], L[(k + 1) % 4]
            A.append(
                arcs.addFillet(a, a.endSketchPoint.geometry, b, b.startSketchPoint.geometry, rv)
            )
        # L0 runs along the first axis at b0, L1 along the second at a1, L2 at b1, L3 at a0
        ix, iy = self._ax(sk)
        self._dim(sk, L[3].startSketchPoint, c[0][ix], H)
        self._dim(sk, L[1].startSketchPoint, c[1][ix], H)
        self._dim(sk, L[0].startSketchPoint, c[0][iy], V)
        self._dim(sk, L[2].startSketchPoint, c[2][iy], V)
        g = A[0].centerSketchPoint.geometry
        d = sk.sketchDimensions.addRadialDimension(A[0], P3(g.x + 0.3, g.y + 0.3, 0))
        d.parameter.expression = str(r)
        for k in range(1, 4):
            gc.addEqual(A[0], A[k])

    def circle(self, sk, c, r):
        circ = sk.sketchCurves.sketchCircles.addByCenterRadius(self._pt(sk, c), self.v(r))
        ix, iy = self._ax(sk)
        cx, cy = c[ix], c[iy]
        if abs(self.v(cx)) < 1e-7 and abs(self.v(cy)) < 1e-7:
            sk.geometricConstraints.addCoincident(circ.centerSketchPoint, sk.originPoint)
        else:
            self._place(sk, circ.centerSketchPoint, c)
        g = circ.centerSketchPoint.geometry
        d = sk.sketchDimensions.addDiameterDimension(circ, P3(g.x + self.v(r), g.y, 0))
        d.parameter.expression = f"2 * {_p(r)}"
        return circ

    def stadium(self, sk, c1, c2, r):
        """Slot of radius r between centres c1 and c2 (a hull of two circles)."""
        q1, q2 = self._pt(sk, c1), self._pt(sk, c2)
        rv = self.v(r)
        L = math.hypot(q2.x - q1.x, q2.y - q1.y)
        dx, dy = (q2.x - q1.x) / L, (q2.y - q1.y) / L
        nx, ny = -dy, dx
        arcs, lines = sk.sketchCurves.sketchArcs, sk.sketchCurves.sketchLines
        a1 = arcs.addByCenterStartSweep(q1, P3(q1.x + rv * nx, q1.y + rv * ny, 0), math.pi)
        a2 = arcs.addByCenterStartSweep(q2, P3(q2.x - rv * nx, q2.y - rv * ny, 0), math.pi)
        l1 = lines.addByTwoPoints(a1.endSketchPoint, a2.startSketchPoint)
        l2 = lines.addByTwoPoints(a2.endSketchPoint, a1.startSketchPoint)
        gc = sk.geometricConstraints
        for a, b in ((a1, l1), (l1, a2), (a2, l2), (l2, a1)):
            gc.addTangent(a, b)
        gc.addEqual(a1, a2)
        self._place(sk, a1.centerSketchPoint, c1)
        self._place(sk, a2.centerSketchPoint, c2)
        d = sk.sketchDimensions.addRadialDimension(a1, P3(q1.x + rv, q1.y + rv, 0))
        d.parameter.expression = str(r)

    def d_shape(self, sk, r, flat):
        """Circle r about the z axis with a flat at x = -flat (the post's key)."""
        self.circle(sk, ("0 mm", "0 mm", "0 mm"), r)
        reach = add(r, "1 mm")
        self.poly_open(sk, [(neg(flat), neg(reach), "0 mm"), (neg(flat), reach, "0 mm")])

    def poly_open(self, sk, pts):
        lines = sk.sketchCurves.sketchLines
        q = [self._pt(sk, p) for p in pts]
        segs = [lines.addByTwoPoints(q[0], q[1])]
        for k in range(2, len(q)):
            segs.append(lines.addByTwoPoints(segs[-1].endSketchPoint, q[k]))
        verts = [segs[0].startSketchPoint] + [s.endSketchPoint for s in segs]
        for spt, p in zip(verts, pts, strict=True):
            self._place(sk, spt, p)

    # ---- profiles
    @staticmethod
    def _done(sk):
        sk.isComputeDeferred = False
        return sk

    def all_profiles(self, sk):
        self._done(sk)
        col = adsk.core.ObjectCollection.create()
        for p in sk.profiles:
            col.add(p)
        return col

    def biggest(self, sk):
        self._done(sk)
        return max(sk.profiles, key=lambda p: p.areaProperties().area)

    def ring_profile(self, sk):
        self._done(sk)
        return [p for p in sk.profiles if p.profileLoops.count == 2][0]

    # ---- features
    def extrude(self, name, sk, profs, start, dist, op, taper=None, symmetric=False):
        """Along the sketch plane's normal (+x, +y or +z), from coordinate `start` for `dist`."""
        ef = self.comp.features.extrudeFeatures
        inp = ef.createInput(profs, op)
        if symmetric:
            inp.setSymmetricExtent(VI(dist), True)
        else:
            ext = adsk.fusion.DistanceExtentDefinition.create(VI(dist))
            pos = adsk.fusion.ExtentDirections.PositiveExtentDirection
            if taper:
                inp.setOneSideExtent(ext, pos, VI(taper))
            else:
                inp.setOneSideExtent(ext, pos)
            if str(start).strip() not in ("0", "0 mm"):
                inp.startExtent = adsk.fusion.OffsetStartDefinition.create(VI(start))
        if op != NEW:
            inp.participantBodies = [self.body()]
        f = ef.add(inp)
        f.name = name
        sk.isVisible = False
        return f

    def blocks(self, name, rects, z, dz, op):
        """Axis-aligned blocks sharing one z range: rects = [((x, y), (dx, dy)), ...]."""
        sk = self.sketch("xy", name)
        for (x, y), (dx, dy) in rects:
            self.rect(sk, (x, y, "0 mm"), (dx, dy, "0 mm"))
        return self.extrude(name, sk, self.all_profiles(sk), z, dz, op)

    def polys(self, name, plane, shapes, start, dist, op):
        sk = self.sketch(plane, name)
        for pts in shapes:
            self.poly(sk, pts)
        return self.extrude(name, sk, self.all_profiles(sk), start, dist, op)

    def chamfer(self, name, edges, dist):
        cf = self.comp.features.chamferFeatures
        col = adsk.core.ObjectCollection.create()
        for e in edges:
            col.add(e)
        inp = cf.createInput2()
        inp.chamferEdgeSets.addEqualDistanceChamferEdgeSet(col, VI(dist), False)
        f = cf.add(inp)
        f.name = name
        return f


# ================================================================ box
def build_box(pt):
    W = "wall"
    # shell
    sk = pt.sketch("xy", "outside")
    pt.rrect(sk, ("0 mm", "0 mm", "0 mm"), ("box_x", "box_y", "0 mm"), "r_out")
    pt.extrude("outside", sk, pt.all_profiles(sk), "0 mm", "box_z", NEW)
    pt.body().name = "box"
    sk = pt.sketch("xy", "inside")
    pt.rrect(sk, (W, W, "0 mm"), ("inner_x", "inner_y", "0 mm"), "r_out - wall")
    pt.extrude("inside", sk, pt.all_profiles(sk), "floor_t", "inner_h + 1 mm", CUT)
    # USB-C hole in the back wall, in line with the s3mini's port across the cell's slot:
    # with the cell in, a plug stops against the cell
    sk = pt.sketch("xz", "USB-C hole")
    cx, cz = "wall + s3_cx", "floor_t + usb_z"
    pt.rrect(
        sk,
        (sub(cx, "(plug_w + 1 mm) / 2"), "0 mm", sub(cz, "(plug_h + 1 mm) / 2")),
        ("plug_w + 1 mm", "0 mm", "plug_h + 1 mm"),
        "1.5 mm",
    )
    pt.extrude("USB-C hole", sk, pt.all_profiles(sk), "box_y - wall - 1 mm", "wall + 2 mm", CUT)
    # cable notch in the front wall, round bottom; the lid's tab closes it over the cable
    sk = pt.sketch("xz", "cable notch")
    nx, nz = "wall + cable_x", "box_z - cable_d - tab_len + cable_d / 2"
    pt.stadium(
        sk, (nx, "0 mm", nz), (nx, "0 mm", "box_z + 1 mm"), "cable_d / 2"
    )  # open above the box
    pt.extrude("cable notch", sk, pt.all_profiles(sk), "-1 mm", "wall + 2 mm", CUT)
    # lid detents: a groove inside each end wall
    sk = pt.sketch("xz", "lid detent grooves")
    for x in (W, "wall + inner_x"):
        pt.circle(sk, (x, "0 mm", "box_z - skirt_h / 2"), "0.6 mm")
    pt.extrude("lid detent grooves", sk, pt.all_profiles(sk), "box_y / 2 - 10 mm", "20 mm", CUT)
    # + and - over the holder's ends, on the back wall's inside face
    sk = pt.sketch("xz", "+ and - marks")
    px, mx = "wall + plus_x", "wall + minus_x"
    for x, a, b in ((px, "mark_s", "1.2 mm"), (px, "1.2 mm", "mark_s"), (mx, "mark_s", "1.2 mm")):
        pt.rect(sk, (sub(x, f"{a} / 2"), "0 mm", sub("mark_z", f"{b} / 2")), (a, "0 mm", b))
    pt.extrude("+ and - marks", sk, pt.all_profiles(sk), "wall + inner_y - 0.01 mm", "0.8 mm", CUT)
    # AM312 window in the left end wall, off by default
    sk = pt.sketch("yz", "PIR window")
    pt.circle(sk, ("0 mm", "wall + inner_y * 0.35", "floor_t + inner_h * 0.5"), "pir_d / 2")
    f = pt.extrude("PIR window", sk, pt.all_profiles(sk), "-1 mm", "wall + 2 mm", CUT)
    f.timelineObject.isSuppressed = True

    # inside: everything rises from z = 0, through the floor
    def i(e):  # inside coordinate -> outside
        return add(W, e)

    # holder seat: back wall behind, a low rail in front, stops at the four corners
    pt.blocks(
        "holder rail",
        [((i("hx0 + 6 mm"), i("hy0 - clr - part_t")), ("holder_l - 12 mm", "part_t"))],
        "0 mm",
        "floor_t + holder_plinth + 4 mm",
        JOIN,
    )
    pt.blocks(
        "holder plinth",
        [((i("hx0"), i("hy0")), ("holder_l", "holder_w"))],
        "0 mm",
        "floor_t + holder_plinth",
        JOIN,
    )
    stops = [
        ((i(x), i(y)), ("lead_gap - clr", "4 mm"))
        for x in ("0 mm", "inner_x - (lead_gap - clr)")
        for y in ("hy0", "hy0 + holder_w - 4 mm")
    ]
    pt.blocks("holder stops", stops, "0 mm", "floor_t + holder_plinth + 5 mm", JOIN)

    # s3mini cradle: ledges under the board's short ends, between the header rows
    pt.blocks(
        "s3mini ledges",
        [((i("s3_cx - 5.5 mm"), i(y)), ("11 mm", "2.5 mm")) for y in ("s3_y0", "s3_y1 - 2.5 mm")],
        "0 mm",
        "floor_t + s3_z",
        JOIN,
    )

    def xy(x, y):
        return (i(x), i(y), "0 mm")

    ya = "s3_y0 - clr - part_t"  # antenna end, outside face of the posts
    yat = "s3_y0 + 3 mm"  # top of the antenna-end posts
    yb, ybt = "s3_y1 + clr", "s3_y1 + clr + part_t"
    xr, xri, xl, xli = (
        "s3_cx + s3_e + part_t",
        "s3_cx + s3_e",
        "s3_cx - s3_e - part_t",
        "s3_cx - s3_e",
    )
    cradle = [
        # antenna end: L posts at both corners
        [
            xy("s3_cx + 7 mm", ya),
            xy(xr, ya),
            xy(xr, yat),
            xy(xri, yat),
            xy(xri, "s3_y0 - clr"),
            xy("s3_cx + 7 mm", "s3_y0 - clr"),
        ],
        [
            xy(xl, ya),
            xy("s3_cx - 7 mm", ya),
            xy("s3_cx - 7 mm", "s3_y0 - clr"),
            xy(xli, "s3_y0 - clr"),
            xy(xli, yat),
            xy(xl, yat),
        ],
        # USB end, right: a stop beside the port, clear of the plug, and a post up the board's side
        [
            xy(xri, "s3_y1 - 3 mm"),
            xy(xr, "s3_y1 - 3 mm"),
            xy(xr, ybt),
            xy("s3_cx + 7 mm", ybt),
            xy("s3_cx + 7 mm", yb),
            xy(xri, yb),
        ],
        # USB end, left: the stop only; the 5V wire leaves the first pin on this side
        [xy(xl, yb), xy("s3_cx - 7 mm", yb), xy("s3_cx - 7 mm", ybt), xy(xl, ybt)],
    ]
    pt.polys("s3mini posts", "xy", cradle, "0 mm", "floor_t + s3_top", JOIN)

    # perfboard seat: four corners, each a ledge, an L wall and two snap lips
    corners = [(sx, sy) for sy in (1, -1) for sx in (1, -1)]

    def cxy(sx, sy):
        cx = "wall + perf_x0" if sx > 0 else "wall + perf_x0 + perf_l"
        cy = "wall + perf_y0" if sy > 0 else "wall + perf_y0 + perf_w"
        return cx, cy

    def loc(c, s, e):  # corner origin c, mirror sign s, local coordinate e
        return add(c, e) if s > 0 else sub(c, e)

    ledges, walls = [], []
    for sx, sy in corners:
        cx, cy = cxy(sx, sy)
        ledges.append(
            [
                (loc(cx, sx, a), loc(cy, sy, b), "0 mm")
                for a, b in (("0 mm", "0 mm"), ("3 mm", "0 mm"), ("3 mm", "3 mm"), ("0 mm", "3 mm"))
            ]
        )
        o, n = "-clr - part_t", "-clr"
        walls.append(
            [
                (loc(cx, sx, a), loc(cy, sy, b), "0 mm")
                for a, b in ((o, o), ("5 mm", o), ("5 mm", n), (n, n), (n, "5 mm"), (o, "5 mm"))
            ]
        )
    pt.polys("perfboard ledges", "xy", ledges, "0 mm", "floor_t + perf_standoff", JOIN)
    pt.polys("perfboard corners", "xy", walls, "0 mm", "floor_t + perf_h", JOIN)
    # snap lips: a wedge over each board edge, 3 mm long, starting 1 mm from the corner
    lip = (
        ("-clr - 0.5 mm", "lip_z"),
        ("0.35 mm", "lip_z"),
        ("-clr", "perf_h"),
        ("-clr - 0.5 mm", "perf_h"),
    )
    for sy in (1, -1):  # lips over the short edges (wedge in x, run along y)
        shapes = []
        for sx in (1, -1):
            cx, cy = cxy(sx, sy)
            shapes.append([(loc(cx, sx, a), "0 mm", add("floor_t", z)) for a, z in lip])
        cy = cxy(1, sy)[1]
        y0 = add(cy, "1 mm") if sy > 0 else sub(cy, "4 mm")
        pt.polys(
            f"perfboard lips x {'front' if sy > 0 else 'back'}", "xz", shapes, y0, "3 mm", JOIN
        )
    for sx in (1, -1):  # lips over the long edges (wedge in y, run along x)
        shapes = []
        for sy in (1, -1):
            cx, cy = cxy(sx, sy)
            shapes.append([("0 mm", loc(cy, sy, a), add("floor_t", z)) for a, z in lip])
        cx = cxy(sx, 1)[0]
        x0 = add(cx, "1 mm") if sx > 0 else sub(cx, "4 mm")
        pt.polys(
            f"perfboard lips y {'left' if sx > 0 else 'right'}", "yz", shapes, x0, "3 mm", JOIN
        )


# ======================================================= lid (in place: underside at z = 0)
def build_lid(pt):
    sk = pt.sketch("xy", "plate")
    pt.rrect(sk, ("0 mm", "0 mm", "0 mm"), ("box_x", "box_y", "0 mm"), "r_out")
    pt.extrude("plate", sk, pt.all_profiles(sk), "0 mm", "lid_t", NEW)
    pt.body().name = "lid"
    # pull tab at the right end
    sk = pt.sketch("xy", "pull tab")
    pt.stadium(
        sk, ("box_x", "box_y / 2 - 5 mm", "0 mm"), ("box_x", "box_y / 2 + 5 mm", "0 mm"), "3 mm"
    )
    pt.extrude("pull tab", sk, pt.all_profiles(sk), "0 mm", "lid_t", JOIN)
    # lip inside the walls
    sk = pt.sketch("xy", "skirt")
    o = "wall + clr"
    pt.rrect(sk, (o, o, "0 mm"), ("inner_x - 2 * clr", "inner_y - 2 * clr", "0 mm"), "skirt_r")
    pt.rect(
        sk,
        ("wall + clr + skirt_t", "wall + clr + skirt_t", "0 mm"),
        ("inner_x - 2 * clr - 2 * skirt_t", "inner_y - 2 * clr - 2 * skirt_t", "0 mm"),
    )
    pt.extrude("skirt", sk, pt.ring_profile(sk), "-skirt_h", "skirt_h", JOIN)
    # detent bumps that click into the end walls' grooves
    sk = pt.sketch("xz", "detent bumps")
    for x in ("wall + clr", "wall + inner_x - clr"):
        pt.circle(sk, (x, "0 mm", "-skirt_h / 2"), "0.5 mm")
    pt.extrude("detent bumps", sk, pt.all_profiles(sk), "box_y / 2 - 9 mm", "18 mm", JOIN)
    # tab closing the cable notch above the cable; it also keys the lid
    pt.blocks(
        "cable tab",
        [(("wall + cable_x - cable_d / 2 + clr", "0 mm"), ("cable_d - 2 * clr", "wall"))],
        "-tab_len",
        "tab_len",
        JOIN,
    )
    # keeper: stops 0.3 mm over the s3mini's USB-C shell so the board cannot lift
    pt.blocks(
        "s3mini keeper",
        [(("wall + s3_cx - 3 mm", "wall + s3_y1 + s3_usb_over - 5.5 mm"), ("6 mm", "4 mm"))],
        "-kz",
        "kz",
        JOIN,
    )
    # gap in the lip where the cable passes
    pt.blocks(
        "cable gap",
        [
            (
                ("wall + cable_x - cable_d / 2 - 2 mm", "wall"),
                ("cable_d + 4 mm", "clr + skirt_t + 1 mm"),
            )
        ],
        "-skirt_h - 1 mm",
        "skirt_h + 0.99 mm",
        CUT,
    )


# ================================================================ drain-hole plug and ring post
def build_plug_base(pt):
    sk = pt.sketch("xy", "spigot")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "spigot_r")
    pt.extrude("spigot", sk, pt.all_profiles(sk), "-spigot_l", "spigot_l", NEW)
    pt.body().name = "plug_base"
    sk = pt.sketch("xy", "flange")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "flange_r")
    pt.extrude("flange", sk, pt.all_profiles(sk), "0 mm", "flange_t", JOIN)
    # socket for the post; closed underneath, so no light reaches the shelf
    sk = pt.sketch("xy", "socket")
    pt.d_shape(sk, "post_r + 0.2 mm", "post_flat + 0.2 mm")
    pt.extrude("socket", sk, pt.biggest(sk), "sock_bot", "flange_t - sock_bot + 1 mm", CUT)
    sk = pt.sketch("xy", "socket lead-in")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "post_r + 0.2 mm")
    pt.extrude(
        "socket lead-in", sk, pt.all_profiles(sk), "flange_t - 0.6 mm", "1 mm", CUT, taper="45 deg"
    )
    # cable groove up the spigot's side (the hole's wall closes it), then through the flange
    pt.blocks(
        "cable groove",
        [(("spigot_r - cable_d", "-cable_d / 2"), ("cable_d + 2 mm", "cable_d"))],
        "-spigot_l - 1 mm",
        "spigot_l + 1 mm",
        CUT,
    )
    pt.blocks(
        "cable hole",
        [(("spigot_r - cable_d", "-cable_d / 2"), ("cable_d", "cable_d"))],
        "-1 mm",
        "flange_t + 2 mm",
        CUT,
    )


def build_plug_post(pt):
    sk = pt.sketch("xy", "shaft")
    pt.d_shape(sk, "post_r", "post_flat")
    pt.extrude("shaft", sk, pt.biggest(sk), "sock_bot", "z_pb - sock_bot", NEW)
    pt.body().name = "plug_post"
    zmin = min(f.pointOnFace.z for f in pt.body().faces)
    foot = [
        f
        for f in pt.body().faces
        if isinstance(f.geometry, adsk.core.Plane) and abs(f.pointOnFace.z - zmin) < 1e-6
    ][0]
    pt.chamfer("foot chamfer", list(foot.edges), "0.5 mm")
    # seat plate and its two arms
    sk = pt.sketch("xy", "seat")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "plate_r")
    pt.rect(sk, ("-arm_w / 2", "-arm_l", "0 mm"), ("arm_w", "2 * arm_l", "0 mm"))
    pt.extrude("seat", sk, pt.all_profiles(sk), "z_pb", "z_face - z_pb", JOIN)
    # 45 degrees under the seat and its arms: prints upright without supports
    sk = pt.sketch("xy", "cone")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "post_flat")
    pt.extrude("cone", sk, pt.all_profiles(sk), "z_pb - cone_h", "cone_h", JOIN, taper="45 deg")
    sk = pt.sketch("yz", "gussets")
    pt.poly(
        sk,
        [
            ("0 mm", "-arm_l", "z_pb"),
            ("0 mm", "arm_l", "z_pb"),
            ("0 mm", "5 mm", "z_pb - gus_h"),
            ("0 mm", "-5 mm", "z_pb - gus_h"),
        ],
    )
    pt.extrude("gussets", sk, pt.all_profiles(sk), "0 mm", "arm_w", JOIN, symmetric=True)
    # ring pocket and the two outer-tab slots (the tabs stay on and rest in the slots)
    sk = pt.sketch("xy", "ring pocket")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "ring_d / 2 + clr")
    pt.extrude("ring pocket", sk, pt.all_profiles(sk), "z_ru", "10 mm", CUT)
    pt.blocks(
        "tab slots",
        [
            (
                ("-(ring_tab_w / 2 + clr)", "-arm_l - 1 mm"),
                ("ring_tab_w + 2 * clr", "2 * arm_l + 2 mm"),
            )
        ],
        "z_ru",
        "10 mm",
        CUT,
    )
    # relief under the ring; the inner tabs rest on the core
    sk = pt.sketch("xy", "relief")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "ring_d / 2 + clr")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "post_r")
    pt.extrude("relief", sk, pt.ring_profile(sk), "z_ru - relief", "relief + 0.01 mm", CUT)
    # wire channel from the relief down to the shaft's groove, open at the side
    pt.blocks(
        "wire channel",
        [(("post_r - 0.5 mm", "-cable_d / 2"), ("plate_r + 2 mm - post_r", "cable_d"))],
        "z_pb - cone_h - 1 mm",
        "z_ru - relief - (z_pb - cone_h - 1 mm) + 0.01 mm",
        CUT,
    )
    # groove down the shaft, in line with the base's groove
    pt.blocks(
        "shaft groove",
        [(("post_r - cable_d", "-cable_d / 2"), ("cable_d + 1 mm", "cable_d"))],
        "flange_t - 1 mm",
        "z_pb - flange_t + 1 mm",
        CUT,
    )


def build_feet(pt):
    sk = pt.sketch("xy", "foot")
    pt.circle(sk, ("0 mm", "0 mm", "0 mm"), "foot_d / 2")
    pt.extrude("foot", sk, pt.all_profiles(sk), "0 mm", "foot_h", NEW)
    pt.body().name = "foot"
    edges = [e for e in pt.body().edges if isinstance(e.geometry, adsk.core.Circle3D)]
    pt.chamfer("foot edges", edges, "0.5 mm")
    rp = pt.comp.features.rectangularPatternFeatures
    col = adsk.core.ObjectCollection.create()
    col.add(pt.body())
    inp = rp.createInput(
        col,
        pt.comp.xConstructionAxis,
        VI("feet_n"),
        VI("foot_d + 4 mm"),
        adsk.fusion.PatternDistanceType.SpacingPatternDistanceType,
    )
    inp.setDirectionTwo(
        pt.comp.yConstructionAxis, VI("1"), VI("0 mm")
    )  # a row; the default makes 3
    rp.add(inp).name = "feet"


# (component, builder, assembly offset in mm, print pose)
PARTS = [
    ("box", build_box, (0, 0, 0), None),
    ("lid", build_lid, (0, 0, "box_z + 30 mm"), "flip_y"),
    ("plug_base", build_plug_base, (-70, 30, 0), "flip_flange"),
    ("plug_post", build_plug_post, (-130, 30, 0), "lift"),
    ("feet", build_feet, (0, -30, 0), None),
]


COINCIDENCES = []


def build(app):
    doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    design.fusionUnitsManager.distanceDisplayUnits = (
        adsk.fusion.DistanceUnits.MillimeterDistanceUnits
    )
    for name, expr, unit, comment in PARAMS:
        if design.userParameters.add(name, VI(expr), unit, comment) is None:
            raise RuntimeError(f"user parameter {name} = {expr} was refused")
    root = design.rootComponent
    root.isOriginFolderLightBulbOn = False
    tl = design.timeline
    for name, fn, offset, _ in PARTS:
        m = adsk.core.Matrix3D.create()
        m.translation = adsk.core.Vector3D.create(
            *[
                design.unitsManager.evaluateExpression(str(o), "mm")
                if isinstance(o, str)
                else o / 10
                for o in offset
            ]
        )
        start = tl.count
        occ = root.occurrences.addNewComponent(m)
        occ.component.name = f"p006-{name}"
        occ.component.isOriginFolderLightBulbOn = False
        part = Part(design, occ.component)
        fn(part)
        COINCIDENCES.extend(part.coincidences)
        if tl.count - start > 1:
            tl.timelineGroups.add(start, tl.count - 1).name = name
    return doc, design


# ================================================================ export
def _occ(design, name):
    for occ in design.rootComponent.occurrences:
        if occ.component.name == f"p006-{name}":
            return occ
    raise RuntimeError(f"component p006-{name} not found")


def _print_pose(design, pose):
    def v(name):  # cm
        return design.userParameters.itemByName(name).value

    m = adsk.core.Matrix3D.create()
    if pose == "flip_y":  # top face on the bed: (x, box_y - y, lid_t - z)
        m.setCell(1, 1, -1)
        m.setCell(2, 2, -1)
        m.translation = adsk.core.Vector3D.create(0, v("box_y"), v("lid_t"))
    elif pose == "flip_flange":  # flange's top face on the bed: (x, -y, flange_t - z)
        m.setCell(1, 1, -1)
        m.setCell(2, 2, -1)
        m.translation = adsk.core.Vector3D.create(0, 0, v("flange_t"))
    elif pose == "lift":  # upright, foot on the bed
        m.translation = adsk.core.Vector3D.create(0, 0, -v("sock_bot"))
    return m


def _set_transform(occ, m):
    if hasattr(occ, "transform2"):
        occ.transform2 = m
    else:
        occ.transform = m


def _get_transform(occ):
    return occ.transform2 if hasattr(occ, "transform2") else occ.transform


def stl_bounds(path):
    with open(path, "rb") as f:
        data = f.read()
    n = struct.unpack_from("<I", data, 80)[0]
    lo, hi = [1e9] * 3, [-1e9] * 3
    for k in range(n):
        vals = struct.unpack_from("<12f", data, 84 + 50 * k)
        for j in range(3):
            for c in range(3):
                x = vals[3 + 3 * j + c]
                lo[c], hi[c] = min(lo[c], x), max(hi[c], x)
    return n, lo, hi


def params_mm(design):
    up = design.userParameters
    return {
        up.item(k).name: up.item(k).value * (10 if up.item(k).unit == "mm" else 1)
        for k in range(up.count)
    }


def check(design):
    p = params_mm(design)
    return [msg for ok, msg in CHECKS if not ok(p)]


def timeline_items(design):
    tl = design.timeline
    for k in range(tl.count):
        it = tl.item(k)
        if it.isGroup:
            g = adsk.fusion.TimelineGroup.cast(it)
            for j in range(g.count):
                yield g.item(j)
        else:
            yield it


def health(design):
    """Timeline objects in error or warning (suppressed ones aside), and how many were checked."""
    bad, n = [], 0
    for it in timeline_items(design):
        n += 1
        if it.isSuppressed:
            continue
        if it.healthState != adsk.fusion.FeatureHealthStates.HealthyFeatureHealthState:
            bad.append(f"{it.name}: {it.errorOrWarningMessage}")
    if n < 10:
        bad.append(f"only {n} timeline objects found")
    return bad


def finish_stl(path, name, m=None):
    """Move a binary STL (mm) by a rigid Matrix3D (Fusion cm), in place, and drop the colour
    Fusion writes into the header and each triangle, so the bytes depend on the geometry only."""
    with open(path, "rb") as f:
        data = bytearray(f.read())
    data[:80] = f"p006-{name} from p006_fusion.py".encode().ljust(80)
    n = struct.unpack_from("<I", data, 80)[0]
    if m is not None:
        r = [[m.getCell(i, j) for j in range(3)] for i in range(3)]
        t = [m.getCell(i, 3) * 10 for i in range(3)]
    for k in range(n):
        off = 84 + 50 * k
        if m is not None:
            vals = struct.unpack_from("<12f", data, off)
            out = []
            for j in range(4):
                p = vals[3 * j : 3 * j + 3]
                out += [
                    sum(r[i][c] * p[c] for c in range(3)) + (t[i] if j else 0) for i in range(3)
                ]
            struct.pack_into("<12f", data, off, *out)
        struct.pack_into("<H", data, off + 48, 0)
    with open(path, "wb") as f:
        f.write(data)


def export_stl(design, occ, path, pose=None):
    """Fusion writes the component's own coordinates (it ignores the occurrence's transform),
    so the mesh is moved to its print pose afterwards."""
    em = design.exportManager
    opt = em.createSTLExportOptions(occ, path)
    opt.isBinaryFormat = True
    opt.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
    opt.sendToPrintUtility = False
    em.execute(opt)
    finish_stl(path, occ.component.name[5:], _print_pose(design, pose) if pose else None)


def export_all(app, design, out_dir):
    stl_dir, png_dir = os.path.join(out_dir, "stl"), os.path.join(out_dir, "png")
    os.makedirs(stl_dir, exist_ok=True)
    os.makedirs(png_dir, exist_ok=True)
    report = []
    for name, _, _, pose in PARTS:
        path = os.path.join(stl_dir, f"p006-{name}.stl")
        export_stl(design, _occ(design, name), path, pose)
        n, lo, hi = stl_bounds(path)
        size = " x ".join(f"{hi[i] - lo[i]:.2f}" for i in range(3))
        report.append(f"{name}: {n} triangles, {size} mm, z {lo[2]:.2f}..{hi[2]:.2f}")
    # posts for trying ring heights, like the .scad's export.sh
    rz = design.userParameters.itemByName("ring_z")
    keep = rz.expression
    for z in (26, 34):
        rz.expression = f"{z} mm"
        adsk.doEvents()
        export_stl(
            design,
            _occ(design, "plug_post"),
            os.path.join(stl_dir, f"p006-plug_post-ring{z}.stl"),
            "lift",
        )
    rz.expression = keep
    adsk.doEvents()
    report += previews(app, design, png_dir)
    return report


VIEWS = [  # (file, parts shown, pose, eye direction, ortho)
    ("box-assembly", ["box", "lid"], "home", (1.0, -1.6, 1.3), False),
    ("box-inside", ["box"], "home", (0.0, 0.0, 1.0), True),
    ("box-print", ["box"], "home", (0.9, -1.3, 1.4), False),
    ("lid-print", ["lid"], "print", (0.9, -1.3, 1.4), False),
    ("pot-parts", ["plug_base", "plug_post"], "pot", (1.0, -1.6, 0.7), False),
    ("plug-base", ["plug_base"], "print", (0.9, -1.3, 1.1), False),
    ("plug-post", ["plug_post"], "print", (0.9, -1.3, 0.9), False),
    ("feet", ["feet"], "home", (0.6, -1.3, 1.2), False),
]


COLOURS = {
    "box": (79, 134, 184),
    "lid": (143, 187, 224),
    "plug_base": (232, 226, 208),
    "plug_post": (242, 237, 223),
    "feet": (90, 90, 90),
}  # as the .scad's previews


def colour(app, design):
    """Give each part a flat plastic colour (cosmetic; skipped if the library item is missing)."""
    lib = app.materialLibraries.itemByName("Fusion Appearance Library")
    base = None
    if lib:
        for k in range(lib.appearances.count):
            a = lib.appearances.item(k)
            if a.name.startswith("Plastic - Matte"):
                base = a
                break
    if base is None:
        return
    for name, rgb in COLOURS.items():
        ap = design.appearances.itemByName(f"p006 {name}")
        if ap is None:
            ap = design.appearances.addByCopy(base, f"p006 {name}")
            for k in range(ap.appearanceProperties.count):
                prop = ap.appearanceProperties.item(k)
                if isinstance(prop, adsk.core.ColorProperty) and prop.value is not None:
                    prop.value = adsk.core.Color.create(*rgb, 255)
        _occ(design, name).appearance = ap


def previews(app, design, png_dir):
    colour(app, design)
    vp = app.activeViewport
    vp.visualStyle = adsk.core.VisualStyles.ShadedWithVisibleEdgesOnlyVisualStyle
    occs = {name: _occ(design, name) for name, *_ in PARTS}
    homes = {n: _get_transform(o) for n, o in occs.items()}
    poses = {name: pose for name, _, _, pose in PARTS}
    out = []
    for fname, shown, pose, eye, ortho in VIEWS:
        for n, o in occs.items():
            o.isLightBulbOn = n in shown
            if pose == "print" and poses[n]:
                _set_transform(o, _print_pose(design, poses[n]))
            elif pose == "pot" and n in ("plug_base", "plug_post"):
                _set_transform(o, adsk.core.Matrix3D.create())  # in place on the pot floor
            else:
                _set_transform(o, homes[n])
        cam = vp.camera
        cam.cameraType = (
            adsk.core.CameraTypes.OrthographicCameraType
            if ortho
            else adsk.core.CameraTypes.PerspectiveCameraType
        )
        cam.target = P3(0, 0, 0)
        cam.eye = P3(*eye)
        cam.upVector = (
            adsk.core.Vector3D.create(0, 1, 0)
            if eye[:2] == (0.0, 0.0)
            else adsk.core.Vector3D.create(0, 0, 1)
        )
        cam.isSmoothTransition = False
        vp.camera = cam
        vp.fit()
        adsk.doEvents()
        path = os.path.join(png_dir, f"{fname}.png")
        vp.saveAsImageFile(path, 1200, 900)
        out.append(path)
    for n, o in occs.items():
        o.isLightBulbOn = True
        _set_transform(o, homes[n])
    return [f"{len(out)} previews in {png_dir}"]


def main(out_dir=None, show=False):
    app = adsk.core.Application.get()
    out_dir = out_dir or OUT
    design = adsk.fusion.Design.cast(app.activeProduct)
    built = False
    if design is None or design.userParameters.itemByName("holder_l") is None:
        _, design = build(app)
        built = True
    lines = ["built a new design" if built else "exporting the active p006 design"]
    bad = health(design)
    failed = check(design)
    if bad:
        lines += ["timeline errors:"] + bad
    if failed:
        lines += ["checks failed, nothing exported:"] + failed
    else:
        lines += export_all(app, design, out_dir)
    msg = "\n".join(lines)
    if show:
        app.userInterface.messageBox(msg, "p006 parts")
    return msg


def run(_context):
    main(show=True)

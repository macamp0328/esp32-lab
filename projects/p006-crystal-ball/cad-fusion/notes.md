# p006 parts in Fusion: notes

This is the phase 2 (layout B) printed parts rebuilt in Autodesk Fusion. It is a trial next to
[../cad/](../cad/), the OpenSCAD version, which stays the source for the build list. See
[COMPARISON.md](COMPARISON.md) for which works better.

## What is here
| Path | What |
| --- | --- |
| [p006_fusion/p006_fusion.py](p006_fusion/p006_fusion.py) | Fusion Python script. It builds the parametric design, then exports. It mirrors `p006_parts.scad` feature by feature. |
| `p006_fusion/p006_fusion.manifest` | Lets Fusion list the folder as a script |
| `stl/` | Same file names and print orientation as `../cad/stl/` (lid and plug base flipped, post standing on its foot) |
| `png/` | Previews, rendered by Fusion |

## Running it
1. In Fusion, open Utilities > Add-Ins > Scripts and Add-Ins, click **+**, and pick the
   `p006_fusion` folder. Fusion remembers it.
2. Run it with **no p006 design open**. It builds a new, unsaved design in 3–20 s (slower when
   another design is open):
   - five components (box, lid, plug_base, plug_post, feet), one timeline group each;
   - 97 user parameters, each with a comment: 54 inputs (the 18 guesses are marked MEASURE)
     and 43 derived from them;
   - it then exports `stl/` and `png/` (about 10 s) and shows a summary.
3. Change a measurement in Modify > Change Parameters (the *fx* button). For example, set
   `holder_l` to 80 mm. The model recomputes in under a second.
4. Run the script again with the p006 design active. It **exports only**: it checks the
   timeline for errors, runs the `.scad`'s asserts as checks, then rewrites `stl/` and `png/`.
   If a check fails it exports nothing and says why.

The design is **not saved**. Saving puts it in Autodesk's cloud and uses one of the Personal
plan's 10 editable-document slots; it was 9 of 10 used on 2026-11-04. Rerunning the script
rebuilds it from nothing, so the `.py` is the source and the Fusion design can be thrown away.
To keep edits made in the Fusion UI (a fillet, say), File > Save it; those edits then live only
in the cloud.

## How it maps to the .scad
- **Parameters:**
  - Every top-level `.scad` parameter is a user parameter, with the same name and default.
  - The derived values (`inner_x`, `usb_z`, `z_pb`, …) are user parameters with expressions,
    so they follow any change.
  - `ox/oy/oz` are renamed `box_x/box_y/box_z`, because Fusion reads `oz` as ounces.
  - `hz` is spelt out as `holder_plinth`.
  - `plus_end` became `plus_left`, a number: 1 means left, 0 means right.
- **Geometry:**
  - Each `cube`/`linear_extrude` is a sketch on a base plane plus an extrude, with an offset
    start.
  - Every sketch vertex is dimensioned from the origin by an expression, so sketches are fully
    constrained and follow the parameters.
  - Each `hull()` was redrawn as its exact outline:
    - the cable notch and pull tab are slots;
    - the snap lips are wedges;
    - the gussets are a trapezoid;
    - the cone under the seat is a 45° tapered extrude;
    - the foot of the post is a 0.5 mm chamfer.
  - `offset(delta = 0.2)` of the D socket is the D drawn 0.2 mm bigger.
- **Conditional parts:**
  - `pir` is a suppressed feature, *PIR window*: right-click it in the timeline and pick
    Unsuppress. No parameter switches it.
  - `if (hz > 0)` became a plinth extruded up from the floor's underside, so at height 0 it
    changes nothing.
- **Asserts:** the script's `CHECKS` run at export. Fusion does not warn while you edit
  parameters.

## Checked
Every result below came out of the comparison scripts in this session's scratchpad. The
commands are in COMPARISON.md.
- **Volume match:** every part has the same volume as the OpenSCAD STL to within 0.01 cm³:
  | Part | Fusion (cm³) | OpenSCAD (cm³) |
  | --- | --- | --- |
  | Box | 25.675 | 25.678 |
  | Lid | 10.789 | 10.789 |
  | Plug base | 11.529 | 11.521 |
  | Post | 12.347 | 12.349 |
  | Feet | 2.362 | 2.361 |
- **Geometric diff:** OpenSCAD `difference()` both ways, split into connected pieces. No piece
  is over 1.5 mm³:
  - the box's four outside corners differ by 0.75 mm³ each, from curve faceting;
  - under each post arm there is 1.4 mm³ extra in the `.scad`, because its `hull()` slabs are
    0.01 mm thick.
- **Parameter sweep:** five sets of changes, applied in Fusion and as `-D` in OpenSCAD. After
  each, no timeline errors, and every part matched to within 10 mm³:
  - holder size;
  - + on the right;
  - plinth 8;
  - ring 36, drain 40, pot floor 12;
  - clr 0.3, cable 5, perfboard 36.
- **Repeatability:** re-exporting the open design gives byte-identical STLs. A fresh rebuild
  gives byte-identical STLs for 6 of 7. The box differs only in two triangle normals, by
  1.7e-15, because that design had been through a parameter change and back.

## Fusion API gotchas met here (2706.1.17)
Each one cost a failed run.
- In one sketch, a second origin dimension with the same value and direction is refused as
  "over constrained", even on a free point. Fix: align the second point to the first with
  `addVerticalPoints`/`addHorizontalPoints`. For a mirrored point, dimension the pair at
  twice the distance.
- A slot made of two arcs and two tangent lines refuses a dimension on its second centre's
  shared coordinate. Same fix: align the centres.
- Python can reuse the `id()` of a dropped sketch wrapper; keep a reference while using one.
- STL export of an occurrence writes the *component's* coordinates and ignores the occurrence
  transform. The script moves the mesh to its print pose itself.
- STL export writes the body's colour into the header and into every triangle's attribute
  bytes. So changing an appearance changes the file; the script zeroes them.
- `unitsManager.evaluateExpression` can return a stale value right after a parameter change.
  `cell_top` read 20.1 while the parameter said 28.1. Read `userParameters.itemByName(n).value`
  instead.
- Expression functions take `;` between arguments: `max(a; b)`. A user parameter named `c` is
  refused.
- `rectangularPatternFeatures` made 12 copies when asked for 4: the second direction defaults
  to 3. Set it to 1 with `setDirectionTwo`.
- On a Z-up design, sketches on XZ have sketch y = −Z, and sketches on YZ have sketch x = −Z.
  All three base planes' normals point along +X, +Y or +Z, and an offset start is measured
  along that normal. A positive taper angle widens the extrude.

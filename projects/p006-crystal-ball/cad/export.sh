#!/usr/bin/env bash
# Rebuild every STL and preview PNG in this folder from p006_parts.scad.
# Needs OpenSCAD 2025 or later (Homebrew: openscad@snapshot); OPENSCAD=/path/to/openscad overrides.
set -euo pipefail
cd "$(dirname "$0")"
scad=p006_parts.scad
oscad=${OPENSCAD:-$(command -v openscad || echo /Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD)}
mkdir -p stl png

stl() {  # stl <file> <part> [-D name=value ...]
  local name=$1 part=$2
  shift 2
  "$oscad" -q --backend=manifold --export-format=binstl -D "part=\"$part\"" "$@" \
    -o "stl/p006-$name.stl" "$scad"
}
for p in box lid plug_base plug_post feet fit_box fit_plug; do stl "$p" "$p"; done
for z in 26 34; do stl "plug_post-ring$z" plug_post -D "ring_z=$z"; done  # posts for trying heights

png() {  # png <file> <part> <rot_x,rot_y,rot_z> [openscad args ...]
  local name=$1 part=$2 rot=$3
  shift 3
  "$oscad" -q --backend=manifold --render -D "part=\"$part\"" --camera="0,0,0,$rot,0" \
    --autocenter --viewall --imgsize=1200,900 --colorscheme=Tomorrow "$@" -o "png/$name.png" "$scad"
}
png box-assembly assembly 55,0,25
png box-inside inside 0,0,0 --projection=o
png box-section section 90,0,90 --projection=o
png pot-section pot 85,0,0 --projection=o
png box-print box 40,0,20
png lid-print lid 40,0,20
png plug-post plug_post 60,0,30
png plug-base plug_base 50,0,30
png fit-box fit_box 45,0,30
png fit-plug fit_plug 50,0,20
echo "export: $(ls stl | wc -l | tr -d ' ') STL, $(ls png | wc -l | tr -d ' ') PNG"

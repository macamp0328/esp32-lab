// p006 crystal ball, phase 2 (layout B): every printed part, for the Bambu A1 mini (180 mm cube).
// Units: mm. One part per run:  openscad -D 'part="box"' -o box.stl p006_parts.scad
// cad/export.sh writes every STL and preview. A default marked MEASURE is a guess until the part
// is in hand; cad/README.md lists them, with print settings and the fit test.

part = "assembly";  // [assembly, inside, section, pot, box, lid, plug_base, plug_post, feet, fit_box, fit_plug]

/* [Fit] */
clr = 0.25;  // gap per side between a printed part and a bought one

/* [18650 holder: MEASURE when it arrives] */
holder_l = 78;       // MEASURE: outside length over the end walls
holder_w = 21;       // MEASURE: outside width
holder_h = 20;       // MEASURE: underside to the top of the end walls
holder_side_h = 13;  // MEASURE: underside to the top of the long side walls
holder_base = 1.5;   // MEASURE: underside to the cell's lowest point
lead_gap = 3;        // room at each end of the holder for its leads
plus_end = "left";   // [left, right] the holder's + contact, seen from the front

/* [Cell] */
cell_d = 18.6;  // MEASURE: protected 18650s run 18.3 to 18.8
cell_l = 70;

/* [USB-C cable used with the cell out] */
plug_w = 12.5;  // MEASURE: width of the plug's moulded body
plug_h = 7;     // MEASURE: height of the plug's moulded body

/* [s3mini: Waveshare ESP32-S3-Zero, vendor drawing] */
s3_w = 18;
s3_l = 23.5;
s3_pcb_t = 1.0;     // MEASURE
s3_usb_over = 0.6;  // MEASURE: how far the USB-C shell sticks out past the board edge
s3_usb_w = 9;
s3_usb_h = 3.2;
s3_usb_d = 7.3;
s3_row = 15.24;    // header rows, centre to centre
s3_hdr = 2.5;      // header plastic under the board
s3_pin = 6;        // header pins below the plastic
s3_offset = 14;    // s3mini centre right of the holder centre, at least (its port must face the cell's body)
holder_plinth = 0; // lifts the holder and the s3mini; 8 makes room for Dupont sockets on the pins

/* [Perfboard: MEASURE after cutting] */
perf_l = 40;
perf_w = 20;
perf_t = 1.6;
perf_standoff = 3;  // room for solder joints under the board

/* [Box] */
wall = 2;
floor_t = 2;
lid_t = 2;
head = 6;      // room above the holder: fingers, and the + and - marks
r_out = 3;
skirt_h = 3;   // the lid's lip inside the walls
skirt_t = 1.6;
part_t = 1.6;  // posts, rails and brackets inside the box
cable_d = 4.5; // MEASURE: the 4-wire cable bundled 2 x 2
cable_x = 8;   // cable notch, from the inside of the left wall (over the perfboard's plug)
tab_len = 1.5; // the lid's tab that closes the notch over the cable
pir = false;   // AM312 window in the left end wall (presence undecided)
pir_d = 10.5;

/* [Pot and ring] */
drain_d = 38.1;   // MEASURE: 1.5 in drain hole
drain_clr = 0.5;  // concrete is rough
pot_floor = 10;   // MEASURE: floor thickness at the hole
ring_z = 30;      // MEASURE: the ring's LED face above the pot floor; the hook tip is ~38
ring_d = 28.7;
ring_hole = 19.3;
ring_tabs = 43.2;  // across the two outer tabs
ring_tab_w = 9;    // MEASURE: width of an outer tab
ring_t = 1.0;      // MEASURE: ring board thickness
ring_proud = 0.4;  // the ring's face sits this far above the seat
post_r = 8;

/* [Feet] */
foot_d = 16;
foot_h = 3;
feet_n = 4;

$fn = 72;

// ---- derived: box (inside coordinates: x from the left wall, y from the front wall, z from the floor)
hz = holder_plinth;
inner_x = holder_l + 2 * lead_gap;
hx0 = lead_gap;
perf_x0 = 0.5 + part_t + clr;
// right of the holder's centre by s3_offset, and never closer to the perfboard than 0.5 mm
s3_cx = max(inner_x / 2 + s3_offset, perf_x0 + perf_l + 2 * (clr + part_t) + 0.5 + s3_w / 2);
s3_y0 = 0.3 + part_t + clr;           // board's antenna end
s3_y1 = s3_y0 + s3_l;                 // board's USB end, facing the holder
hy0 = s3_y1 + clr + part_t + clr;     // holder's front face
inner_y = hy0 + holder_w + clr;
inner_h = hz + holder_h + head;
ox = inner_x + 2 * wall;
oy = inner_y + 2 * wall;
oz = floor_t + inner_h;               // box height without the lid
usb_z = hz + holder_side_h + plug_h / 2 + 0.6;  // port centre: the plug clears the holder's sides
s3_z = usb_z - s3_usb_h / 2 - s3_pcb_t;         // board underside
cell_zc = hz + holder_base + cell_d / 2;
cell_top = cell_zc + cell_d / 2;
cell_y = hy0 + holder_w / 2;
perf_y0 = (hy0 - clr - part_t - (perf_w + 2 * (clr + part_t))) / 2 + part_t + clr;

assert(usb_z - 1.3 <= cell_top - 2,
       "holder side walls too tall: a plug would pass over the cell. File a notch plug_w + 1 wide in both side walls and set holder_side_h to its depth");
assert(abs(s3_cx - inner_x / 2) + plug_w / 2 + 2 <= cell_l / 2 - 5,
       "the s3mini's port no longer faces the cell's body: shorten perf_l or lower s3_offset");
assert(s3_z - s3_hdr - s3_pin >= 0.5, "header pins hit the floor: raise holder_plinth");
assert(s3_cx + s3_w / 2 + clr + part_t <= inner_x, "the s3mini does not fit beside the perfboard: shorten perf_l");
assert(ox <= 175 && oy <= 175, "box larger than the A1 mini's bed");

// ---- derived: plug and post (pot coordinates: z = 0 on the pot floor, centred on the drain hole)
spigot_r = drain_d / 2 - drain_clr;
spigot_l = min(8, pot_floor - 1);
flange_r = drain_d / 2 + 6;
flange_t = 2.5;
sock_bot = -spigot_l + 1.5;
post_flat = post_r - 1.5;          // a flat on the shaft keys its groove to the base's
z_ru = ring_z - ring_t;            // ring underside
z_face = ring_z - ring_proud;      // seat face
relief = 3;                        // under the ring: solder joints and wire bends
plate_floor = 2;
z_pb = z_ru - relief - plate_floor;
plate_r = ring_d / 2 + clr + 1.6;
arm_w = ring_tab_w + 2 * clr + 3.2;
arm_l = ring_tabs / 2 - 1;         // tab tips stick out 1 mm past the arms
cone_h = plate_r - post_flat;      // 45 degrees under the seat
gus_h = arm_l - 5;                 // 45 degrees under the arms

assert(z_pb - max(cone_h, gus_h) >= flange_t + 0.5,
       str("ring_z below ", flange_t + 0.5 + max(cone_h, gus_h) + plate_floor + relief + ring_t,
           ": the post's 45-degree seat no longer fits; print it with supports"));
assert(spigot_l >= 3, "pot_floor too thin for the spigot");

// ---- helpers
module rrect(x, y, r) {
  if (r > 0.01) offset(r) offset(-r) square([x, y]);
  else square([x, y]);
}
module rrect_c(x, y, r) { translate([-x / 2, -y / 2]) rrect(x, y, r); }
module in_box() { translate([wall, wall, floor_t]) children(); }
module d2(r, flat) {
  intersection() {
    circle(r);
    translate([-flat, -r - 1]) square([2 * r + 2, 2 * r + 2]);
  }
}
module plus_sign(s, t) {
  square([s, t], center = true);
  square([t, s], center = true);
}
module minus_sign(s, t) { square([s, t], center = true); }

// ================================================================ box
module box() {
  ms = min(5, head - 1);
  mark_z = floor_t + hz + holder_h + head / 2;
  plus_x = plus_end == "left" ? hx0 + 7 : inner_x - hx0 - 7;
  minus_x = plus_end == "left" ? inner_x - hx0 - 7 : hx0 + 7;
  difference() {
    linear_extrude(oz) rrect(ox, oy, r_out);
    in_box() linear_extrude(inner_h + 1) rrect(inner_x, inner_y, r_out - wall);
    // USB-C hole in the back wall, in line with the s3mini's port across the cell's slot:
    // with the cell in, a plug stops against the cell
    translate([wall + s3_cx, oy - wall - 1, floor_t + usb_z]) rotate([-90, 0, 0])
      linear_extrude(wall + 2) rrect_c(plug_w + 1, plug_h + 1, 1.5);
    // cable notch in the front wall; the lid's tab closes it over the cable
    translate([wall + cable_x, -1, oz - cable_d - tab_len]) cable_slot(wall + 2);
    // lid detents: a groove inside each end wall
    for (x = [wall, wall + inner_x])
      translate([x, oy / 2 - 10, oz - skirt_h / 2]) rotate([-90, 0, 0]) cylinder(r = 0.6, h = 20, $fn = 24);
    // + and - over the holder's ends, on the back wall's inside face
    translate([wall + plus_x, wall + inner_y - 0.01, mark_z]) rotate([-90, 0, 0])
      linear_extrude(0.8) plus_sign(ms, 1.2);
    translate([wall + minus_x, wall + inner_y - 0.01, mark_z]) rotate([-90, 0, 0])
      linear_extrude(0.8) minus_sign(ms, 1.2);
    if (pir)
      translate([-1, wall + inner_y * 0.35, floor_t + inner_h * 0.5]) rotate([0, 90, 0])
        cylinder(d = pir_d, h = wall + 2);
  }
  in_box() {
    holder_seat();
    s3_cradle();
    perf_seat();
  }
}

module cable_slot(len) {  // along +y, round bottom at z = 0, open at the top
  hull() {
    translate([0, 0, cable_d / 2]) rotate([-90, 0, 0]) cylinder(d = cable_d, h = len, $fn = 32);
    translate([-cable_d / 2, 0, cable_d / 2]) cube([cable_d, len, cable_d / 2 + tab_len + 1]);
  }
}

module holder_seat() {
  // back wall behind, a low rail in front, stops at the four corners; the leads leave between them
  translate([hx0 + 6, hy0 - clr - part_t, 0]) cube([holder_l - 12, part_t, hz + 4]);
  if (hz > 0) translate([hx0, hy0, 0]) cube([holder_l, holder_w, hz]);
  for (x = [0, inner_x - (lead_gap - clr)], y = [hy0, hy0 + holder_w - 4])
    translate([x, y, 0]) cube([lead_gap - clr, 4, hz + 5]);
}

module s3_cradle() {
  top = s3_z + s3_pcb_t + 0.8;
  e = s3_w / 2 + clr;  // inside face of the corner posts, from the board's centre line
  // ledges under the board's two short ends, between the header rows
  for (y = [s3_y0, s3_y1 - 2.5]) translate([s3_cx - 5.5, y, 0]) cube([11, 2.5, s3_z]);
  for (sx = [-1, 1]) {
    // antenna end: L posts at both corners
    translate([s3_cx + sx * (e + part_t / 2), s3_y0 - clr - part_t, 0])
      translate([-part_t / 2, 0, 0]) cube([part_t, part_t + clr + 3, top]);
    translate([s3_cx + (sx > 0 ? 7 : -e - part_t), s3_y0 - clr - part_t, 0])
      cube([e + part_t - 7, part_t, top]);
    // USB end: a stop beside the port on each side, clear of the plug
    translate([s3_cx + (sx > 0 ? 7 : -e - part_t), s3_y1 + clr, 0]) cube([e + part_t - 7, part_t, top]);
  }
  // USB end, right side only: the left side is where the 5V wire leaves the first pin
  translate([s3_cx + e, s3_y1 - 3, 0]) cube([part_t, 3 + clr + part_t, top]);
}

module perf_corner() {  // board corner at the origin, board in +x +y
  h = perf_standoff + perf_t + 1;
  lip_z = perf_standoff + perf_t + 0.1;
  cube([3, 3, perf_standoff]);
  translate([-clr - part_t, -clr - part_t, 0]) {
    cube([part_t, 5 + clr + part_t, h]);
    cube([5 + clr + part_t, part_t, h]);
  }
  // small snap lips over the board's edges
  hull() {
    translate([-clr, 1, lip_z]) cube([clr + 0.35, 3, 0.01]);
    translate([-clr, 1, h - 0.01]) cube([0.01, 3, 0.01]);
  }
  hull() {
    translate([1, -clr, lip_z]) cube([3, clr + 0.35, 0.01]);
    translate([1, -clr, h - 0.01]) cube([3, 0.01, 0.01]);
  }
}

module perf_seat() {
  translate([perf_x0, perf_y0, 0]) {
    perf_corner();
    translate([perf_l, 0, 0]) mirror([1, 0, 0]) perf_corner();
    translate([0, perf_w, 0]) mirror([0, 1, 0]) perf_corner();
    translate([perf_l, perf_w, 0]) mirror([1, 0, 0]) mirror([0, 1, 0]) perf_corner();
  }
}

// ================================================================ lid (in place: underside at z = 0)
module lid() {
  kz = inner_h - (usb_z + s3_usb_h / 2 + 0.3);
  difference() {
    union() {
      linear_extrude(lid_t) rrect(ox, oy, r_out);
      // pull tab at the right end
      hull() for (y = [oy / 2 - 5, oy / 2 + 5]) translate([ox, y, 0]) cylinder(r = 3, h = lid_t);
      // lip inside the walls
      translate([wall + clr, wall + clr, -skirt_h]) linear_extrude(skirt_h + 0.01) difference() {
        rrect(inner_x - 2 * clr, inner_y - 2 * clr, max(r_out - wall - clr, 0.3));
        offset(-skirt_t) rrect(inner_x - 2 * clr, inner_y - 2 * clr, max(r_out - wall - clr, 0.3));
      }
      // detent bumps that click into the end walls' grooves
      for (x = [wall + clr, wall + inner_x - clr])
        translate([x, oy / 2 - 9, -skirt_h / 2]) rotate([-90, 0, 0]) cylinder(r = 0.5, h = 18, $fn = 24);
      // tab closing the cable notch above the cable; it also keys the lid
      translate([wall + cable_x - cable_d / 2 + clr, 0, -tab_len]) cube([cable_d - 2 * clr, wall, tab_len + 0.01]);
      // keeper: stops 0.3 mm over the s3mini's USB-C shell so the board cannot lift
      translate([wall + s3_cx - 3, wall + s3_y1 + s3_usb_over - 5.5, -kz]) cube([6, 4, kz + 0.01]);
    }
    // gap in the lip where the cable passes
    translate([wall + cable_x - cable_d / 2 - 2, wall, -skirt_h - 1]) cube([cable_d + 4, clr + skirt_t + 1, skirt_h + 0.99]);
  }
}

// ================================================================ drain-hole plug and ring post
module plug_base() {
  difference() {
    union() {
      translate([0, 0, -spigot_l]) cylinder(r = spigot_r, h = spigot_l + 0.01);
      cylinder(r = flange_r, h = flange_t);
    }
    // socket for the post; closed underneath, so no light reaches the shelf
    translate([0, 0, sock_bot]) linear_extrude(flange_t - sock_bot + 1) offset(delta = 0.2) d2(post_r, post_flat);
    translate([0, 0, flange_t - 0.6]) cylinder(r1 = post_r + 0.2, r2 = post_r + 0.8, h = 0.61);
    // cable groove up the spigot's side (the hole's wall closes it), then through the flange
    translate([spigot_r - cable_d, -cable_d / 2, -spigot_l - 1]) cube([cable_d + 2, cable_d, spigot_l + 1]);
    translate([spigot_r - cable_d, -cable_d / 2, -1]) cube([cable_d, cable_d, flange_t + 2]);
  }
}

module seat_2d() {
  circle(plate_r);
  translate([-arm_w / 2, -arm_l]) square([arm_w, 2 * arm_l]);
}

module plug_post() {
  difference() {
    union() {
      // shaft, chamfered at the foot; it stands on the socket's floor
      hull() {
        translate([0, 0, sock_bot]) linear_extrude(0.01) offset(-0.5) d2(post_r, post_flat);
        translate([0, 0, sock_bot + 0.5]) linear_extrude(0.01) d2(post_r, post_flat);
      }
      translate([0, 0, sock_bot + 0.5]) linear_extrude(z_pb - sock_bot - 0.49) d2(post_r, post_flat);
      translate([0, 0, z_pb]) linear_extrude(z_face - z_pb) seat_2d();
      // 45 degrees under the seat and its arms: prints upright without supports
      hull() {
        translate([0, 0, z_pb - cone_h]) cylinder(r = post_flat, h = 0.01);
        translate([0, 0, z_pb - 0.01]) cylinder(r = plate_r, h = 0.01);
      }
      for (s = [-1, 1]) hull() {
        translate([-arm_w / 2, s > 0 ? 0 : -arm_l, z_pb - 0.01]) cube([arm_w, arm_l, 0.01]);
        translate([-arm_w / 2, s > 0 ? 0 : -5, z_pb - gus_h]) cube([arm_w, 5, 0.01]);
      }
    }
    // ring pocket and the two outer-tab slots (the tabs stay on and rest in the slots)
    translate([0, 0, z_ru]) cylinder(r = ring_d / 2 + clr, h = 10);
    translate([-(ring_tab_w / 2 + clr), -arm_l - 1, z_ru]) cube([ring_tab_w + 2 * clr, 2 * arm_l + 2, 10]);
    // relief under the ring; the inner tabs rest on the core
    translate([0, 0, z_ru - relief]) linear_extrude(relief + 0.01) difference() {
      circle(r = ring_d / 2 + clr);
      circle(r = post_r);
    }
    // wire channel from the relief down to the shaft's groove, open at the side to lay the wires in
    translate([post_r - 0.5, -cable_d / 2, z_pb - cone_h - 1])
      cube([plate_r + 2 - post_r, cable_d, z_ru - relief - (z_pb - cone_h - 1) + 0.01]);
    // groove down the shaft, in line with the base's groove
    translate([post_r - cable_d, -cable_d / 2, flange_t - 1]) cube([cable_d + 1, cable_d, z_pb - flange_t + 1]);
  }
}

// ================================================================ feet
module foot() {
  hull() {
    cylinder(d = foot_d - 1, h = 0.01);
    translate([0, 0, 0.5]) cylinder(d = foot_d, h = foot_h - 1);
    translate([0, 0, foot_h - 0.01]) cylinder(d = foot_d - 1, h = 0.01);
  }
}
module feet() { for (i = [0:feet_n - 1]) translate([i * (foot_d + 4), 0, 0]) foot(); }

// ================================================================ fit tests
module fit_box() {  // a 34 mm slice of the box through the s3mini, the cell slot and the USB hole
  intersection() {
    box();
    translate([wall + s3_cx - 17, -1, -1]) cube([34, oy + 2, floor_t + usb_z + plug_h / 2 + 4]);
  }
}

module fit_plug() {
  // 1. drain-hole ring with the post's socket: turn it over and drop it in the hole
  difference() {
    union() {
      cylinder(r = flange_r, h = 1.2);
      difference() {
        cylinder(r = spigot_r, h = 6.2);
        translate([0, 0, -1]) cylinder(r = spigot_r - 2, h = 10);
      }
      linear_extrude(7.2) offset(delta = 2.2) d2(post_r, post_flat);
    }
    translate([0, 0, -1]) linear_extrude(20) offset(delta = 0.2) d2(post_r, post_flat);
  }
  // 2. a stub of the shaft, to try in the socket
  translate([flange_r + post_r + 6, 0, 0]) difference() {
    linear_extrude(10) d2(post_r, post_flat);
    translate([post_r - cable_d, -cable_d / 2, -1]) cube([cable_d + 1, cable_d, 12]);
  }
  // 3. the ring seat, face up: does the ring drop in with its tabs on
  translate([0, flange_r + arm_l + 4, -z_pb]) intersection() {
    plug_post();
    translate([0, 0, z_pb]) cylinder(r = 40, h = 20);
  }
}

// ================================================================ stand-ins for previews
module holder_ghost() {
  translate([hx0, hy0, hz]) difference() {
    union() {
      cube([holder_l, holder_w, holder_side_h]);
      cube([4, holder_w, holder_h]);
      translate([holder_l - 4, 0, 0]) cube([4, holder_w, holder_h]);
    }
    translate([4, 1.2, holder_base]) cube([holder_l - 8, holder_w - 2.4, holder_h]);
  }
}
module cell_ghost() {
  translate([inner_x / 2 - cell_l / 2, cell_y, cell_zc]) rotate([0, 90, 0]) cylinder(d = cell_d, h = cell_l);
}
module s3_ghost() {
  translate([s3_cx, 0, 0]) {
    color("#1d4f91") translate([-s3_w / 2, s3_y0, s3_z]) linear_extrude(s3_pcb_t) rrect(s3_w, s3_l, 1);
    color("#c0c0c0") translate([-s3_usb_w / 2, s3_y1 + s3_usb_over - s3_usb_d, s3_z + s3_pcb_t])
      cube([s3_usb_w, s3_usb_d, s3_usb_h]);
    color("#222") for (sx = [-1, 1])
      translate([sx * s3_row / 2 - 1.27, s3_y1 - 1.59 - 8 * 2.54 - 1.27, s3_z - s3_hdr]) cube([2.54, 9 * 2.54, s3_hdr]);
    color("#d4af37") for (sx = [-1, 1], k = [0:8])
      translate([sx * s3_row / 2 - 0.32, s3_y1 - 1.59 - k * 2.54 - 0.32, s3_z - s3_hdr - s3_pin]) cube([0.64, 0.64, s3_pin]);
  }
}
module perf_ghost() {
  translate([perf_x0, perf_y0, perf_standoff]) {
    color("#c8a165") cube([perf_l, perf_w, perf_t]);
    color("#222") translate([4, 2, perf_t]) cube([10.2, 2.54, 2.5 + 14]);  // 4-pin header and its housing
    color("#333") translate([18, 8, perf_t]) cube([4.5, 3.5, 8]);         // 2N7000
    color("#2a6") translate([26, 6, perf_t]) cube([7, 8, 6]);             // SOT-23 adapter
    color("#b85") for (i = [0:3]) translate([6 + i * 7, 14, perf_t]) cube([6, 2.2, 2.2]);
  }
}
module plug_ghost() {  // a USB-C cable pushed in from the back, stopped by the cell
  dz = usb_z - cell_zc;
  tip = cell_y + sqrt(max(cell_d * cell_d / 4 - dz * dz, 0));
  translate([wall + s3_cx, wall + tip, floor_t + usb_z]) {
    translate([-4.2, 0, -1.3]) cube([8.4, 6.6, 2.6]);
    translate([-plug_w / 2, 6.6, -plug_h / 2]) cube([plug_w, 22, plug_h]);
    translate([0, 28, 0]) rotate([-90, 0, 0]) cylinder(d = 4, h = 8);
  }
}
module ring_ghost() {
  translate([0, 0, z_ru]) {
    color("#151515") linear_extrude(ring_t) difference() {
      union() {
        circle(d = ring_d);
        for (s = [-1, 1]) hull() {
          translate([0, s * 10]) circle(d = ring_tab_w);
          translate([0, s * (ring_tabs / 2 - ring_tab_w / 2)]) circle(d = ring_tab_w);
        }
        for (s = [-1, 1]) translate([s * 6.5, 0]) circle(d = 8);
      }
      circle(d = ring_hole - 0.01);
      for (s = [-1, 1]) translate([s * 6.5, 0]) circle(d = 3);
      for (s = [-1, 1]) translate([0, s * (ring_tabs / 2 - ring_tab_w / 2)]) circle(d = 3);
    }
    color("#ffcf6e") for (i = [0:19]) rotate(i * 18) translate([12, 0, ring_t]) cube([2, 2, 0.9], center = true);
  }
}
module pot_ghost() {  // half the pot, the orb's hook above the ring
  color("#9a9a96") difference() {
    union() {
      translate([0, 0, -pot_floor]) cylinder(r = 51, h = pot_floor);
      translate([0, 0, -pot_floor]) difference() {
        cylinder(r = 51, h = 95);
        translate([0, 0, -1]) cylinder(r = 41, h = 97);
      }
    }
    translate([0, 0, -pot_floor - 1]) cylinder(d = drain_d, h = pot_floor + 2);
    translate([-60, -120, -20]) cube([120, 120, 140]);
  }
  color("#bfe3ee") {  // the glass hook: a loop ~30 mm wide, its tip ~38 mm above the floor
    translate([0, 0, 38 + 26]) cylinder(d = 6, h = 12);
    translate([0, 0, 38 + 14.5]) rotate([90, 0, 0]) rotate_extrude($fn = 48) translate([12, 0]) circle(d = 5);
  }
  color("#d7eef5") intersection() {  // the bottom of the orb, sunk ~10 mm into the mouth
    translate([0, 0, 83 + 85 - 10]) sphere(d = 170, $fn = 96);
    translate([0, 0, 70]) cylinder(r = 60, h = 25);
  }
}
module pot_cable() {
  color("#c33") {
    translate([spigot_r - cable_d / 2, 0, -pot_floor - 3]) cylinder(d = 3.4, h = pot_floor + flange_t + 3.5);
    translate([spigot_r - cable_d / 2, 0, flange_t + 1.5]) rotate([0, -90, 0]) cylinder(d = 3.4, h = spigot_r - post_r - 1);
    translate([post_r - cable_d / 2, 0, flange_t]) cylinder(d = 3.4, h = z_ru - flange_t - 1);
  }
}

module half_y() { intersection() { children(); translate([-200, 0, -200]) cube([400, 400, 400]); } }
module cut_x() { intersection() { children(); translate([-50, -50, -50]) cube([wall + s3_cx + 50, 400, 400]); } }

// ================================================================ dispatch
if (part == "assembly") {
  color("#4f86b8") box();
  color("#8fbbe0") translate([0, 0, oz + 30]) lid();
  in_box() {
    color("#2b2b2b") holder_ghost();
    color("#3a7d44") cell_ghost();
    s3_ghost();
    perf_ghost();
  }
} else if (part == "inside") {
  color("#4f86b8") box();
  in_box() {
    color("#2b2b2b") holder_ghost();
    color("#3a7d44") cell_ghost();
    s3_ghost();
    perf_ghost();
  }
} else if (part == "section") {
  color("#4f86b8") cut_x() box();
  color("#8fbbe0") cut_x() translate([0, 0, oz]) lid();
  color("#2b2b2b") cut_x() in_box() holder_ghost();
  color("#3a7d44") cut_x() in_box() cell_ghost();
  cut_x() in_box() s3_ghost();
  color("#d9534f") plug_ghost();
} else if (part == "pot") {
  pot_ghost();
  color("#e8e2d0") half_y() plug_base();
  color("#f2eddf") half_y() plug_post();
  ring_ghost();
  pot_cable();
} else if (part == "box") {
  box();
} else if (part == "lid") {
  translate([0, oy, lid_t]) rotate([180, 0, 0]) lid();  // top face on the bed
} else if (part == "plug_base") {
  translate([0, 0, flange_t]) rotate([180, 0, 0]) plug_base();  // flange's top face on the bed
} else if (part == "plug_post") {
  translate([0, 0, -sock_bot]) plug_post();  // upright, foot on the bed
} else if (part == "feet") {
  feet();
} else if (part == "fit_box") {
  translate([-(wall + s3_cx - 17), 0, 0]) fit_box();
} else if (part == "fit_plug") {
  fit_plug();
}

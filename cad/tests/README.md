# Joint tests

Test cases for different ways of joining modules. Resin print, pair them up and see which one holds best.

## Final choice

- **Keybit:** J7, 8x 4mm x 3mm magnets sticking out 0.75mm, glued with the inner jig
- **Keyword:** J7.1b, 2x 4mm x 1mm magnets on collars per side, sticking out 0.75mm
- The press fit coupon (v1) was loose in every pocket, so the magnets are glued
- The final cases are in `production/CAD` (`Keybit_Case_Big.step` and `Keyword_Case_Big_Inner.step`)

| File | Board | Joint | Magnets | Print |
| --- | --- | --- | --- | --- |
| `J1_magnets_only.step` | keybit | magnets only (baseline) | 4mm x 1mm, 8 | 2 |
| `J2_bowtie_case.step` | keybit | dovetail sockets, joined with a drop in key from the top | 4mm x 1mm, 8 | 2 |
| `J2_bowtie_key.step` | - | bow tie key for J2 | - | 4 |
| `J3_snap_hooks.step` | keybit | snap hook + window on each side | 4mm x 1mm, 8 | 2 |
| `J4_sliding_dovetail.step` | keybit | built in dovetail, slide one module down onto the other | 4mm x 1mm, 8 | 2 |
| `J5_slip_pegs.step` | keybit | loose alignment pegs, magnets hold | 4mm x 1mm, 8 | 2 |
| `J6_crush_rib_pegs.step` | keybit | press fit pegs with crush ribs (will probably crack in clear resin) | 4mm x 1mm, 8 | 2 |
| `J7_keybit_protruding.step` | keybit | magnets stick out 0.75mm, 1.5mm gap between cases (same as the main keybit case now) | 4mm x 3mm, 8 | 2 |
| `J7.1_keybit_protruding.step` | keybit | J7 + 0.2mm floor behind each magnet so it stops at 0.75mm out, 0.3mm pocket clearance | 4mm x 3mm, 8 | 2 |
| `J7.1_keyword_protruding.step` | keyword (inner) | 2 magnets per side on collars (4x2 at the front corner spot, 4x1 at the other), same as the main keyword case now | 4mm x 2mm, 3 + 4mm x 1mm, 3 | 1 |
| `J7.1b_keyword_2x4x1.step` | keyword (inner) | same as J7.1 but 4x1 on collars at both spots, so both sides pull the same | 4mm x 1mm, 6 | 1 |
| `J7.2_keybit_pressfit.step` | keybit | J7.1 with 3 crush ribs in each pocket (0.05mm interference) + 0.2mm lead in, no glue | 4mm x 3mm, 8 | 2 |
| `MAGNET_pressfit_coupon.step` | test | 7 numbered 4x3 pockets to find the right press fit, see below | 4mm x 3mm, 7 | 1 |
| `JIG_keybit_J7_inner.step` | tool | drops inside a J7 keybit case (no switch), 8 pads stop each magnet at 0.75mm out | - | 1 |
| `JIG_keybit_J7_outer_gauge.step` | tool | bar pressed on the outside of one side, pads set 2 magnets at 0.75mm out | - | 1 |

## Pairing

- J1 to J6: pair two of the same keybit case
- J7: pair two `J7_keybit_protruding`
- J7.1: pair two `J7.1_keybit_protruding`, then one with `J7.1_keyword_protruding` and one with `J7.1b_keyword_2x4x1`
- J7.2: pair two `J7.2_keybit_pressfit`, or one with a J7.1 keybit
- Older files (one magnet per side keyword trays, the standoff pad versions, the first jig) are in `superseded/`
- Keyword files are only `Keyword_Inner`, the front and corner parts are the same as before

## Magnets

Magnet nearest the clockwise corner of each side (from above, keycap up) goes N out, the other one S out. Keyword J7.1 and J7.1b have two per side and use the same rule as keybit.

The Form 4 prints small holes tight, a 0.15mm clearance pocket can be too small after curing. Open it up with a 4.1 to 4.2mm drill bit by hand if a magnet will not go in. J7.1 uses 0.3mm on the plain keybit pockets.

On J7 (not J7.1) the keybit pockets go through the wall, so the magnet height has to be set with the jig (or two cases stuck together flat on a table).

## Press fit coupon

Pockets 1 to 5 are 4.3mm with 3 crush ribs, interference 0.02, 0.04, 0.06, 0.08 and 0.10mm. Pocket 6 is a plain 4.05mm hole, pocket 7 a plain 4.10mm hole. All are 2.25mm deep to a 0.2mm floor (same as J7.1) with a 0.2mm lead in.

1. Wash and fully cure the coupon like a real case
2. Press a 4x3 magnet into each pocket with something flat (a vise or a flat bar), not a point
3. Pick the tightest one that holds the magnet without cracking or going white around the hole
4. Tell me the number, J7.2 is one value in Onshape (rib interference) so it is a quick re export

## Gluing (J7 keybit)

1. Put packing tape over the jig pads (or the gauge pads) so glue does not stick to it
2. Drop the inner jig into the case from the top, no switch in, and hold it by the tab
3. Push the magnets in dry from the outside until they hit the jig pads, check polarity on each
4. Wick a little thin CA in from the inside behind each magnet (the jig face is recessed 0.5mm around each pad so the glue can get to the magnet rim)
5. Never glue on the outside rim, it stops the cases sitting flat
6. Let it cure, then pull the jig out by the tab
7. Any CA on a magnet face: scrape it off with a razor blade held flat, no acetone near the resin

The jig fits both J7 keybit cases (main and test): 0.25mm per side to the switch guide blocks, about 1mm to everything else, the pads only touch the magnets. It has a 0.3mm lead in chamfer on the bottom.

The outer gauge does the same thing from the outside if you do not want to use the jig: feet sit on the case face, pads push the magnets in to 0.75mm.

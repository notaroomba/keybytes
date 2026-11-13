FeatureScript 3095;
import(path : "onshape/std/common.fs", version : "3095.0");

// Keyword V2 case. Same frame and outside as the Keybit V2 case so the modules mate:
// origin = middle of the flex (KiCad 172.55, 77.7), Z = 0 is the top of the 0.2mm middle stiffener, flaps fold UP.
// The Keyword has three flaps (W, N, E), USB-C on the south edge, no switch. The battery lies ON TOP of the board in the lid,
// north of the USB-C, so the bottom can be as shallow as the hall modules. The base is only a cover for the back side parts.

const ZB = -0.304;      // flex bottom (B.Cu)
const ZL = -0.454;      // ledge under the flex
// case bottom is the feature parameter "Depth below board top", shared with the Keybit cases so the keyboard sits flat
const ZTOP = 6.0;       // case top
const PADZ = 3.539;     // contact pad centres on the folded flaps
const PADT = [-5.95, -3.57, -1.19, 1.19, 3.57, 5.95];
const HOLDT = [-4.35, 4.35];
const HOLDERS = false;   // 4x3 holder magnets are off: they stop keys being staggered
// USB-C (HC-TYPE-C-16P-01A) folded-model box: x +-4.47, y -9.02..-1.37, z -1.21..3.06
const USBX = 4.47;
const USBZ0 = -0.2;
const USBZ1 = 3.06;
// B side parts of hardware/keyword (user routed board), [x0,y0,z0,x1,y1,z1]
const BCOMP = [[-2.81, -0.84, -1.07, 2.81, 4.79, -0.3], [-2.35, -5.25, -0.8, -1.35, -4.75, -0.3], [0.4, -3.7, -0.8, 1.4, -3.2, -0.3], [-0.42, -6.9, -0.65, 0.57, -6.4, -0.3], [-5.97, 5.62, -0.8, -4.97, 6.12, -0.3], [2.65, 3.8, -0.8, 3.15, 4.8, -0.3], [-4.85, -0.95, -0.8, -3.85, -0.45, -0.3], [-2.35, -4.28, -0.65, -1.35, -3.77, -0.3], [-3.12, -1.7, -0.8, -2.62, -0.7, -0.3], [-1.5, -3.27, -0.8, -0.5, -2.77, -0.3], [3.4, 1.05, -0.8, 4.4, 1.55, -0.3], [-0.58, -6.01, -1.1, 1.64, -4.0, -0.3], [-2.35, -6.22, -0.65, -1.35, -5.72, -0.3], [-4.92, 1.6, -0.8, -3.92, 2.1, -0.3], [2.12, -5.6, -0.8, 2.62, -4.6, -0.3], [-4.38, 0.05, -0.8, -3.38, 0.55, -0.3], [-6.7, 2.4, -1.4, -3.55, 5.4, -0.3], [0.3, 4.4, -0.95, 2.55, 7.0, -0.3], [5.5, 4.8, -0.9, 6.75, 6.95, -0.3]];
// F side parts, the USB-C is handled on its own
const FCOMP = [[-5.03, 1.0, -0.2, -4.03, 1.5, 0.3], [-0.7, 0.33, -0.2, 0.9, 1.93, 0.35], [-5.24, 2.15, -0.2, -3.35, 3.9, 0.6], [3.1, -0.07, -0.2, 4.7, 0.72, 0.6], [1.52, 0.05, -0.2, 2.02, 1.05, 0.15], [-1.82, 0.05, -0.2, -1.32, 1.05, 0.15]];
const BAT = [-6.6, -0.6, 0.7, 6.6, 6.9, 5.2];   // battery cavity in the lid: 13.2 x 7.5, from 0.7 above the board top to a 0.8 roof
const LED = [-4.79, 3.03];                               // D2
const TPS = [[-6.05, 5.5], [-4.0, 5.5], [-1.95, 5.5], [0.1, 5.5]];   // TP4 GND, TP3 TCK, TP2 TIO, TP1 3V3 (top side)
const BATPADS = [[6.15, -6.15], [-6.15, -6.15]];         // TP11 BAT (south-east), TP12 GND (south-west), top side

function v3(p is array) returns Vector { return vector(p[0], p[1], p[2]) * millimeter; }

// rotate [x,y,z] by k * 90 deg about Z
function rz(p is array, k is number) returns array
{
    var q = p;
    for (var i = 0; i < k; i += 1)
        q = [-q[1], q[0], q[2]];
    return q;
}

function pBox(a is array, b is array) returns map { return { "t" : "box", "a" : a, "b" : b }; }
function pCyl(p0 is array, p1 is array, r is number) returns map { return { "t" : "cyl", "a" : p0, "b" : p1, "r" : r }; }
function pSph(c is array, r is number) returns map { return { "t" : "sph", "a" : c, "r" : r }; }
function pCone(p0 is array, p1 is array, r0 is number, r1 is number) returns map { return { "t" : "cone", "a" : p0, "b" : p1, "r" : r0, "r1" : r1 }; }

// box rotated about Z by k*90
function boxr(a is array, b is array, k is number) returns map { return pBox(rz(a, k), rz(b, k)); }
function cylr(p0 is array, p1 is array, r is number, k is number) returns map { return pCyl(rz(p0, k), rz(p1, k), r); }

function prim(context is Context, id is Id, p is map)
{
    if (p.t == "box")
        fCuboid(context, id, { "corner1" : v3(p.a), "corner2" : v3(p.b) });
    else if (p.t == "cyl")
        fCylinder(context, id, { "bottomCenter" : v3(p.a), "topCenter" : v3(p.b), "radius" : p.r * millimeter });
    else if (p.t == "sph")
        opSphere(context, id, { "center" : v3(p.a), "radius" : p.r * millimeter });
    else if (p.t == "cone")
        fCone(context, id, { "bottomCenter" : v3(p.a), "topCenter" : v3(p.b), "bottomRadius" : p.r * millimeter, "topRadius" : p.r1 * millimeter });
}

// union of pos minus union of neg, named
function solid(context is Context, id is Id, pos is array, neg is array, name is string) returns Query
{
    for (var i = 0; i < size(pos); i += 1)
        prim(context, id + "pos" + ("p" ~ i), pos[i]);
    if (size(pos) > 1)
        opBoolean(context, id + "u", { "tools" : qCreatedBy(id + "pos", EntityType.BODY), "operationType" : BooleanOperationType.UNION });
    if (size(neg) > 0)
    {
        for (var i = 0; i < size(neg); i += 1)
            prim(context, id + "neg" + ("n" ~ i), neg[i]);
        opBoolean(context, id + "s", { "tools" : qCreatedBy(id + "neg", EntityType.BODY), "targets" : qCreatedBy(id + "pos", EntityType.BODY), "operationType" : BooleanOperationType.SUBTRACTION });
    }
    const q = qCreatedBy(id + "pos", EntityType.BODY);
    setProperty(context, { "entities" : q, "propertyType" : PropertyType.NAME, "value" : name });
    return q;
}

function tint(context is Context, q is Query, c is Color)
{
    setProperty(context, { "entities" : q, "propertyType" : PropertyType.APPEARANCE, "value" : c });
}


function compPockets(list is array, grow is number, zTop is number, zExtra is number) returns array
{
    var out = [];
    for (var b in list)
        out = append(out, pBox([b[0] - grow, b[1] - grow, b[2] - zExtra], [b[3] + grow, b[4] + grow, zTop]));
    return out;
}


function buildKeyword(context is Context, id is Id, w is number, depth is number, showMagnets is boolean)
{
    const ZBOT = -depth;
    const HOLDZ = ZBOT + 2.4;
    const FLOOR = 0.6;
    const h = w / 2;
    // sides with a flap, as quarter turns of the north side: 0 = north, 1 = west, 3 = east. 2 (south) is the USB side.
    const flapSides = [0, 1, 3];
    // ---------- shell
    var neg = [
        pBox([-6.75, -6.75, ZBOT - 1], [6.75, 6.75, ZL]),            // lower cavity (tray goes here)
        pBox([-7.75, -7.75, ZL], [7.75, 7.75, ZTOP + 1]),            // flaps (B face at 7.643) and the lid plug
        pBox([-8.0, -8.0, 4.7], [8.0, 8.0, ZTOP + 1]),               // lid flange
        pBox([-8.3, -7.35, 0.35], [8.3, 7.35, 2.65]),                // FET / resistor band on the W and E flaps
        pBox([-7.35, 6.0, 0.35], [7.35, 8.3, 2.65]),                 // same on the N flap
        pBox([-6.3, -h - 1, -1.95], [6.3, -6.0, 4.85])               // USB-C plug overmold (12.6 x 6.8) through the south wall
    ];
    for (var k in flapSides)
        for (var t in PADT)
            neg = append(neg, cylr([t, 6.9, PADZ], [t, h + 1, PADZ], 0.6, k));          // 1x2 contact magnets
    if (HOLDERS)
        for (var k in flapSides)
            for (var t in HOLDT)
                neg = append(neg, cylr([t, 6.0, HOLDZ], [t, h + 1, HOLDZ], 2.075, k));  // J7 4x3 holders, through
    const shell = solid(context, id + "shell", [pBox([-h, -h, ZBOT], [h, h, ZTOP])], neg, "Keyword V2 Shell");
    tint(context, shell, color(0.85, 0.85, 0.82));

    // ---------- base: cover under the flex, pockets for the back side parts
    var bneg = compPockets(BCOMP, 0.25, ZB + 1, 0.25);
    bneg = append(bneg, pBox([-5.65, -5.65, ZBOT + FLOOR], [5.65, 5.65, ZB + 1]));     // parts cavity, 0.6 floor, 0.8 rim
    bneg = append(bneg, pBox([-6.6, -7.0, -1.6], [6.6, -4.6, ZB + 1]));                // south rim lowered for the USB-C shell and legs
    const base = solid(context, id + "base", [pBox([-6.45, -6.45, ZBOT], [6.45, 6.45, ZB])], bneg, "Keyword V2 Base");
    tint(context, base, color(0.6, 0.6, 0.58));

    // ---------- lid: plug that sits on the stiffener between the flaps, flange closes the top
    var lneg = [];
    for (var b in FCOMP)
        lneg = append(lneg, pBox([b[0] - 0.25, b[1] - 0.25, -1], [b[3] + 0.25, b[4] + 0.25, b[5] + 0.2]));
    lneg = append(lneg, pBox([-USBX - 0.3, -h - 1, -1], [USBX + 0.3, -1.1, USBZ1 + 0.2]));     // USB-C body
    lneg = append(lneg, pBox([-6.3, -h - 1, -1], [6.3, -7.3, 4.85]));                          // plug overmold relief at the wall
    lneg = append(lneg, pBox([BAT[0], BAT[1], -1], [BAT[3], BAT[4], BAT[5]]));                 // battery cavity, open towards the board (covers the LED and the SWD pads)
    // battery pads TP11 / TP12 on the top side beside the USB-C: solder room and a wire channel up to the battery
    lneg = append(lneg, pBox([-6.9, -6.95, -1], [-4.77, BAT[1] + 0.1, 2.2]));
    lneg = append(lneg, pBox([4.77, -6.95, -1], [6.9, BAT[1] + 0.1, 2.2]));
    const lid = solid(context, id + "lid",
        [pBox([-6.6, -7.6, 0.02], [6.6, 6.6, 1.0]),      // foot on the stiffener, clear of the bends
         pBox([-7.3, -7.6, 1.0], [7.3, 7.3, 5.0]),        // plug between the flaps
         pBox([-7.9, -7.9, 5.0], [7.9, 7.9, ZTOP])],      // flange over the flap tops (4.94)
        lneg, "Keyword V2 Lid");
    tint(context, lid, color(0.25, 0.25, 0.27));

    if (showMagnets)
    {
        var mc = [];
        var mh = [];
        for (var k in flapSides)
            for (var t in PADT)
                mc = append(mc, cylr([t, 7.643, PADZ], [t, 9.643, PADZ], 0.5, k));
        if (HOLDERS)
            for (var k in flapSides)
                for (var t in HOLDT)
                    mh = append(mh, cylr([t, h + 0.75 - 3, HOLDZ], [t, h + 0.75, HOLDZ], 2.0, k));
        for (var i = 0; i < size(mc); i += 1)
            tint(context, solid(context, id + ("mc" ~ i), [mc[i]], [], "Magnet 1x2 N52 (contact)"), color(0.3, 0.3, 0.35));
        for (var i = 0; i < size(mh); i += 1)
            tint(context, solid(context, id + ("mh" ~ i), [mh[i]], [], "Magnet 4x3 (J7 holder)"), color(0.3, 0.3, 0.35));
        tint(context, solid(context, id + "bat", [pBox([BAT[0] + 0.35, BAT[1] + 0.25, BAT[2] + 0.05], [BAT[3] - 0.35, BAT[4] - 0.25, BAT[2] + 3.05])], [], "LiPo 3 x 7 x 12.5 (placeholder)"), color(0.75, 0.75, 0.8));
    }
}

annotation { "Feature Type Name" : "Keyword V2 Case" }
export const keywordV2Case = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Case width" }
        isLength(definition.caseWidth, { (millimeter) : [15, 17.55, 25] } as LengthBoundSpec);
        annotation { "Name" : "Depth below board top" }
        isLength(definition.depth, { (millimeter) : [1.9, 2.2, 8] } as LengthBoundSpec);
        annotation { "Name" : "Show magnets and battery" }
        definition.showMagnets is boolean;
    }
    {
        buildKeyword(context, id, definition.caseWidth / millimeter, definition.depth / millimeter, definition.showMagnets);
    });

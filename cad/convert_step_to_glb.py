#!/usr/bin/env python3
"""
Convert every STEP file in ./cad to GLB (glTF 2.0 binary) in ../blender.

GLB is the best interchange format for Blender: it imports natively
(File > Import > glTF 2.0), keeps real-world scale, and carries materials.

Run from the repo root (or anywhere) once dependencies are installed:

    pip install cascadio trimesh
    python cad/convert_step_to_glb.py

cascadio is a small wrapper around OpenCASCADE that tessellates STEP/BREP
geometry; trimesh writes the GLB. No CAD app required.

Optional: pass a linear tolerance (mesh fineness in mm, smaller = finer) as arg 1,
and an angular tolerance (radians, smaller = smoother curves) as arg 2.
Defaults are tuned for nice-looking Blender renders of small parts.
"""
import sys
from pathlib import Path

try:
    import cascadio
    import trimesh
except ImportError:
    sys.exit(
        "Missing dependencies. Run:\n    pip install cascadio trimesh\n"
        "then re-run this script."
    )

HERE = Path(__file__).resolve().parent          # .../cad
CAD_DIR = HERE
OUT_DIR = HERE.parent / "blender"               # .../blender
OUT_DIR.mkdir(exist_ok=True)

TOL_LINEAR = float(sys.argv[1]) if len(sys.argv) > 1 else 0.05   # mm  (chord error)
TOL_ANGULAR = float(sys.argv[2]) if len(sys.argv) > 2 else 0.3    # rad (normal deviation)

step_files = sorted(
    p for p in CAD_DIR.iterdir()
    if p.suffix.lower() in (".step", ".stp")
)
if not step_files:
    sys.exit(f"No .step/.stp files found in {CAD_DIR}")

print(f"Converting {len(step_files)} file(s) -> {OUT_DIR}\n")
for src in step_files:
    dst = OUT_DIR / (src.stem + ".glb")
    tmp = dst.with_suffix(".tmp.glb")
    try:
        # cascadio writes a glb directly from the STEP
        cascadio.step_to_glb(
            str(src), str(tmp),
            tol_linear=TOL_LINEAR,
            tol_angular=TOL_ANGULAR,
        )
        # round-trip through trimesh to validate + normalize the output
        scene = trimesh.load(str(tmp), force="scene")
        scene.export(str(dst))
        tmp.unlink(missing_ok=True)
        n_faces = sum(len(g.faces) for g in scene.geometry.values())
        print(f"  OK  {src.name:24s} -> {dst.name:24s} ({n_faces:,} faces)")
    except Exception as e:
        tmp.unlink(missing_ok=True)
        print(f"  FAIL {src.name}: {e}")

print("\nDone. In Blender: File > Import > glTF 2.0 (.glb/.gltf)")

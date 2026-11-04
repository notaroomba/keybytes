import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def run_easyeda2kicad_from_file(input_file, output_dir="./lib/lcsc", python_exec="python"):
    input_file = os.path.expanduser(input_file)
    output_dir = os.path.expanduser(output_dir)
    python_exec = python_exec or "python"

    if not os.path.isfile(input_file):
        print(f"Error: File not found: {input_file}")
        return 2

    # If a simple name was provided, check PATH; if an absolute path, check that file exists.
    found = shutil.which(python_exec) if os.path.basename(python_exec) == python_exec else os.path.exists(python_exec)
    if not found:
        print(f"Warning: Python executable '{python_exec}' not found in PATH or as given path. Trying anyway.")

    # easyeda2kicad 1.x writes <dir>/easyeda2kicad.* if --output is an existing folder.
    # We want <parent>/lcsc.{kicad_sym,pretty,3dshapes}, so make sure only the parent exists.
    if os.path.isdir(output_dir):
        try:
            os.rmdir(output_dir)
        except OSError:
            print(f"Error: {output_dir} is a non-empty folder; easyeda2kicad would write into it.")
            return 4
    os.makedirs(os.path.dirname(os.path.abspath(output_dir)), exist_ok=True)

    with open(input_file, "r", encoding="utf-8") as f:
        # ignore blank lines and comments
        lines = [line.strip() for line in f if line.strip() and not line.lstrip().startswith("#")]

    if not lines:
        print("No LCSC IDs found in input file.")
        return 0

    for idx, lcsc_id in enumerate(lines, start=1):
        base = ["easyeda2kicad"] if sys.platform == "darwin" else [python_exec, "-m", "easyeda2kicad"]
        cmd = base + [
            "--full",
            f"--lcsc_id={lcsc_id}",
            f"--output={output_dir}",
            "--overwrite"
        ]
        print(f"[{idx}/{len(lines)}] Running: {' '.join(cmd)}")
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError as e:
            print(f"❌ Error processing {lcsc_id}: {e}")
        except FileNotFoundError as e:
            print(f"❌ Executable not found: {e}")
            return 3

    # easyeda2kicad writes 3D model paths relative to cwd (or absolute); anchor them to the
    # project dir. The KiCad projects live one level down (hardware/keybit, hardware/keyword),
    # so the library is at ${KIPRJMOD}/../<output_dir relative to this script>.
    shapes = output_dir + ".3dshapes"
    anchored = "${KIPRJMOD}/../" + os.path.relpath(shapes, HERE).replace(os.sep, "/")
    written = {os.path.relpath(shapes).replace(os.sep, "/"), os.path.abspath(shapes).replace(os.sep, "/")}
    pretty = output_dir + ".pretty"
    for name in os.listdir(pretty) if os.path.isdir(pretty) else []:
        path = os.path.join(pretty, name)
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        fixed = text
        for w in written:
            fixed = fixed.replace(f'(model "{w}/', f'(model "{anchored}/')
        if fixed != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(fixed)

    # some EasyEDA models come out fully transparent (invisible in KiCad's 3D viewer)
    for name in os.listdir(shapes) if os.path.isdir(shapes) else []:
        if not name.endswith(".wrl"):
            continue
        path = os.path.join(shapes, name)
        with open(path, "r", encoding="utf-8") as f:
            text = f.read()
        fixed = text.replace("transparency 1.0", "transparency 0.0")
        if fixed != text:
            with open(path, "w", encoding="utf-8") as f:
                f.write(fixed)

    print("✅ All commands completed.")
    return 0

def main(argv=None):
    parser = argparse.ArgumentParser(description="Run easyeda2kicad for a list of LCSC IDs.")
    parser.add_argument("input_file", nargs="?", default=os.path.join(HERE, "lcsc.txt"), help="Path to file with one LCSC ID per line")
    parser.add_argument("output_dir", nargs="?", default=os.path.join(HERE, "lib", "lcsc"), help="Output directory")
    parser.add_argument("--python", dest="python_exec", default="python",
                        help="Python executable to use (default: 'python')")
    args = parser.parse_args(argv)

    return_code = run_easyeda2kicad_from_file(args.input_file, args.output_dir, args.python_exec)
    sys.exit(return_code if isinstance(return_code, int) else 0)

if __name__ == "__main__":
    main()

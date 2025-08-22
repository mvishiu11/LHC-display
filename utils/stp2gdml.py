# stp2gdml.py
import sys, os
import FreeCAD as App, Part

if len(sys.argv) < 4:
    print("Usage: freecadcmd stp2gdml.py <in.stp> <out.gdml> <label>")
    sys.exit(1)

in_stp   = os.path.abspath(sys.argv[1])
out_gdml = os.path.abspath(sys.argv[2])
label    = sys.argv[3]

if not os.path.isfile(in_stp):
    print("ERROR: Input STEP not found:", in_stp); sys.exit(1)

# Make sure GDML exporter is importable
candidates = [
    os.path.join(App.getUserAppDataDir(), 'Mod'),  # ~/.local/share/FreeCAD/Mod
    '/usr/share/freecad/Mod',
    '/usr/lib/freecad/Mod',
    os.path.join(App.getHomePath(), 'Mod'),
]
for p in candidates:
    if os.path.isdir(p) and p not in sys.path:
        sys.path.append(p)

try:
    from freecad.gdml import exportGDML as egdml
except Exception:
    try:
        import exportGDML as egdml
    except Exception as e:
        print("ERROR: Could not import GDML exporter. Install the GDML workbench in FreeCAD.", e)
        sys.exit(1)

print("[INFO] Loading STEP:", in_stp)
doc = App.newDocument(label)
obj = doc.addObject("Part::Feature", label)
obj.Shape = Part.read(in_stp)
doc.recompute()

print("[INFO] Exporting GDML:", out_gdml)
egdml.export([obj], out_gdml)
print("[OK] Wrote", out_gdml)

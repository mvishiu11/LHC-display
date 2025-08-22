# utils/stp2gdml_gui.py
# Run with the FreeCAD GUI binary (not freecadcmd), in offscreen mode.
# Usage example:
#   QT_QPA_PLATFORM=offscreen QT_OPENGL=software QTWEBENGINE_DISABLE_SANDBOX=1 \
#   freecad -c "import sys; sys.argv=['.../stp2gdml_gui.py', IN_STP, OUT_GDML, LABEL]; \
#               exec(open('.../stp2gdml_gui.py').read())"

import sys, os

try:
    import FreeCAD as App
    import FreeCADGui as Gui
    import Part
except Exception as e:
    print("ERROR: Run with the FreeCAD GUI binary (freecad), not freecadcmd.", e)
    sys.exit(2)

if len(sys.argv) < 4:
    print("Usage: freecad -c with sys.argv = [script, IN.stp, OUT.gdml, LABEL]")
    sys.exit(1)

in_stp   = os.path.abspath(sys.argv[1])
out_gdml = os.path.abspath(sys.argv[2])
label    = sys.argv[3]

if not os.path.isfile(in_stp):
    print("ERROR: Input STEP not found:", in_stp); sys.exit(1)

# Make sure the GDML exporter is importable
candidates = [
    os.path.join(App.getUserAppDataDir(), 'Mod'),  # ~/.local/share/FreeCAD/Mod
    '/usr/share/freecad/Mod',
    '/usr/lib/freecad/Mod',
    os.path.join(App.getHomePath(), 'Mod'),
]
for p in candidates:
    if os.path.isdir(p) and p not in sys.path:
        sys.path.append(p)

egdml = None
try:
    from freecad.gdml import exportGDML as egdml  # preferred
except Exception:
    try:
        import exportGDML as egdml  # fallback
    except Exception as e:
        print("ERROR: Could not import GDML exporter. Install/enable the GDML workbench.", e)
        print("Searched paths:", candidates)
        sys.exit(1)

print("[INFO] Loading STEP:", in_stp)

# Create both App and GUI docs, and make them active
doc = App.newDocument(label)        # application document
gdoc = Gui.newDocument(label)       # GUI document
App.setActiveDocument(doc.Name)
Gui.ActiveDocument = gdoc
Gui.ActiveDocument.Document = doc
Gui.showMainWindow()                # headless because of QT_QPA_PLATFORM=offscreen
Gui.activateWorkbench("PartWorkbench")

# Load STEP into the App document
obj = doc.addObject("Part::Feature", label)
obj.Shape = Part.read(in_stp)
doc.recompute()

print("[INFO] Exporting GDML:", out_gdml)
egdml.export([obj], out_gdml)
print("[OK] Wrote", out_gdml)

# Clean shutdown
Gui.getMainWindow().close()

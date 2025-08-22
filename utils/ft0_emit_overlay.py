#!/usr/bin/env python3
import csv, json, math, sys

# Usage:
#   python3 ft0_emit_overlay.py FT0_LUT.csv amplitudes.csv out.json
# where amplitudes.csv has rows: channel_id, amplitude
# e.g.:  12, 153.4

lut_path = sys.argv[1]
amps_path = sys.argv[2]
out_path  = sys.argv[3]

# --- 1) read LUT: must provide channel_id -> (x,y,z) in cm or mm (we'll declare units)
lut = {}
with open(lut_path) as f:
    r = csv.DictReader(f)
    # Expected columns; adapt names if your LUT differs:
    # channel, x_cm, y_cm, z_cm  (or x_mm,y_mm,z_mm — if mm, set units below accordingly)
    print(r.fieldnames)
    for row in r:
        ch = int(row['channel #'])
        x = float(row.get('coordinate X in mm', 0.0))
        y = float(row.get('coordinate Y in mm', 0.0))
        z = float(row.get('coordinate Z in mm', 0.0))
        lut[ch] = (x,y,z)

# --- 2) read per-channel amplitudes
events = []
with open(amps_path) as f:
    for line in f:
        if not line.strip() or line.strip().startswith('#'): continue
        ch, amp = line.strip().split(',')[:2]
        ch = int(ch); amp = float(amp)
        if ch in lut:
            x,y,z = lut[ch]
            events.append({"id": ch, "amp": amp, "pos": [x,y,z]})

# --- 3) normalize into viewer overlay JSON
# keep it minimal; the viewer will render these as 3D billboards/boxes when we route them to the generic overlay sink
doc = {
  "format": "o2-eve-overlay@1",
  "metadata": {"detector": "FT0", "units": {"length": "cm", "amplitude": "ADC"}},
  "points": [
    {
      "name": "FT0 amplitudes",
      "style": {"glyph": "box", "size_cm": 1.0},
      "values": [{"pos": e["pos"], "v": e["amp"], "id": e["id"]} for e in events]
    }
  ]
}

with open(out_path, 'w') as f:
    json.dump(doc, f, indent=2)
print(f"Wrote {out_path} with {len(events)} points")

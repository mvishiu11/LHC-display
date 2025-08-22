#include "TGeoManager.h"

void gdml2root_ft0(const char* gdml="FT0.gdml", const char* out="FT0.root")
{
  auto gm = TGeoManager::Import(gdml);
  if (!gm) { ::Error("gdml2root_single","Import %s failed", gdml); return; }
  gm->CheckOverlaps(0.1);
  gm->Export(out);
  printf("Wrote %s\n", out);
}

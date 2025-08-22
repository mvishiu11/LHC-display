#include "TGeoManager.h"
#include "TGeoMatrix.h"
#include "TGDMLParse.h"

void wrap_ft0_world(const char* inGDML_orROOT,
                    const char* outROOT = "FT0_wrapped.root",
                    double world_half_cm = 1000.0,   // 10 m box, adjust if needed
                    bool isGDML = false,
                    bool g4units = false)            // set true if your export was in Geant4 units (mm)
{
  // Load source geometry
  TGeoManager* gIn = nullptr;
  if (isGDML) {
    if (g4units) TGeoManager::SetDefaultUnits(kG4Units);
    TGDMLParse p;
    gIn = p.GDMLReadFile(inGDML_orROOT);
  } else {
    gIn = TGeoManager::Import(inGDML_orROOT);
  }
  if (!gIn) { ::Error("wrap_ft0_world","Import failed"); return; }

  TGeoVolume* vIn = gIn->GetTopVolume();
  if (!vIn) { ::Error("wrap_ft0_world","No top volume"); return; }

  // New manager with a solid world (gives a real bbox)
  auto gOut = new TGeoManager("FT0wrap","FT0 wrapped in world");
  auto mat  = new TGeoMaterial("Vac",0,0,0);
  auto med  = new TGeoMedium("Vacuum",1,mat);
  auto world= gOut->MakeBox("WORLD", med, world_half_cm, world_half_cm, world_half_cm);
  gOut->SetTopVolume(world);

  // Clone original tree and insert at origin
  TGeoVolume* vCl = (TGeoVolume*)vIn->Clone();
  vCl->SetName("FT0");
  world->AddNode(vCl, 1, new TGeoTranslation(0,0,0));

  gOut->CloseGeometry();
  gOut->CheckOverlaps(0.1);
  gOut->Export(outROOT);
  ::Info("wrap_ft0_world","Wrote %s", outROOT);
}

// Usage:
// root -l -q 'make_simple_geom_fv0_onlybarrel.C+("/home/ed/geom/FV0_full.root",
//                                                "/home/ed/geom/simple_geom_FV0.root",
//                                                "/home/ed/geom/simple_geom_FV0.txt")'

#include <cstdio>
#include <vector>
#include <string>
#include <set>

#include "TFile.h"
#include "TKey.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TGeoBBox.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TString.h"

#include "TEveManager.h"
#include "TEveGeoNode.h"
#include "TEveElement.h"

static TGeoManager* loadGeoFrom(const char* inFullRoot)
{
  if (TGeoManager::Import(inFullRoot)) return gGeoManager;

  std::unique_ptr<TFile> f(TFile::Open(inFullRoot));
  if (!f || f->IsZombie()) return nullptr;

  TIter next(f->GetListOfKeys());
  while (auto* key = (TKey*)next()) {
    if (strcmp(key->GetClassName(), "TGeoManager") == 0) {
      return (TGeoManager*)key->ReadObj();
    }
  }
  return nullptr;
}

static void setVolVisLeafOnly(TGeoNode* n)
{
  if (!n) return;
  const int nd = n->GetNdaughters();
  if (auto* v = n->GetVolume()) v->SetVisibility(nd == 0); // leaves shown
  for (int i = 0; i < nd; ++i) setVolVisLeafOnly(n->GetDaughter(i));
}

static bool isLeafGeo(const TEveGeoNode* eg)
{
  auto* gn = eg ? eg->GetNode() : nullptr;
  return gn && gn->GetNdaughters() == 0;
}

static void setEveLeafOnly(TEveElement* el)
{
  if (!el) return;

  el->SetRnrChildren(kTRUE);

  if (auto* eg = dynamic_cast<TEveGeoNode*>(el)) {
    el->SetRnrSelf(isLeafGeo(eg));
  } else {
    el->SetRnrSelf(kFALSE);
  }

  for (TEveElement::List_i it = el->BeginChildren(); it != el->EndChildren(); ++it) {
    setEveLeafOnly(*it);
  }
}

static TGeoNode* findNodeByName(TGeoNode* n, const char* wanted)
{
  if (!n) return nullptr;
  if (TString(n->GetName()) == wanted) return n;
  const int nd = n->GetNdaughters();
  for (int i = 0; i < nd; ++i)
    if (auto* r = findNodeByName(n->GetDaughter(i), wanted)) return r;
  return nullptr;
}

static void collectLeafPaths(TGeoNode* n, const std::string& cur, std::vector<std::string>& out)
{
  if (!n) return;
  std::string here = cur;
  here += "/";
  here += n->GetName();
  const int nd = n->GetNdaughters();
  if (nd == 0) { out.push_back(here); return; }
  for (int i = 0; i < nd; ++i) collectLeafPaths(n->GetDaughter(i), here, out);
}

static void removeUnwantedBranches(TEveElement* parent, const std::set<TString>& unwantedNames)
{
  if (!parent) return;
  
  std::vector<TEveElement*> toRemove;
  
  for (TEveElement::List_i it = parent->BeginChildren(); it != parent->EndChildren(); ++it) {
    TEveElement* child = *it;
    TString childName = child->GetElementName();
    
    bool shouldRemove = false;
    for (const auto& unwanted : unwantedNames) {
      if (childName.Contains(unwanted)) {
        shouldRemove = true;
        printf("[INFO] Marking for removal: %s (matches pattern: %s)\n", 
               childName.Data(), unwanted.Data());
        break;
      }
    }
    
    if (shouldRemove) {
      toRemove.push_back(child);
    } else {
      removeUnwantedBranches(child, unwantedNames);
    }
  }
  
  for (auto* element : toRemove) {
    parent->RemoveElement(element);
  }
}

void make_simple_geom_fv0(const char* inFullRoot,
                          const char* outSimpleGeomRoot,
                          const char* outSimpleGeomTxt)
{
  std::set<TString> unwantedBranches = {
    "FV0CONTAINER_0",
    "CONTAINER",
    "PLAST",
    "PMTS",
    "FIBERS",
    "SCREWS",
    "RODS"
  };

  TGeoManager* geo = loadGeoFrom(inFullRoot);
  if (!geo) { Error("make_simple_geom_fv0","Cannot load TGeoManager from %s", inFullRoot); return; }
  geo->GetTopVolume()->InvisibleAll();
  auto* top = geo->GetTopNode();
  if (!top) { Error("make_simple_geom_fv0","No top node in %s", inFullRoot); return; }

  TGeoNode* barrel = findNodeByName(top, "barrel_1");
  if (!barrel) {
    Error("make_simple_geom_fv0","Cannot find node 'barrel_1' in the geometry");
    return;
  }
  setVolVisLeafOnly(barrel);

  printf("[INFO] Replacing barrel_1 volume with invisible container\n");
  
  TGeoVolume* originalVolume = barrel->GetVolume();
  const int nDaughters = barrel->GetNdaughters();
  
  TGeoMaterial* dummyMat = new TGeoMaterial("DummyMat", 0, 0, 0);
  TGeoMedium* dummyMed = new TGeoMedium("DummyMed", 1, dummyMat);
  TGeoBBox* dummyBox = new TGeoBBox("DummyBox", 0.001, 0.001, 0.001);
  TGeoVolume* dummyVol = new TGeoVolume("FV0_invisible", dummyBox, dummyMed);
  
  dummyVol->SetVisibility(kFALSE);
  dummyVol->SetTransparency(100);
  dummyVol->VisibleDaughters(kFALSE);
  
  barrel->SetVolume(dummyVol);
  
  for (int i = 0; i < nDaughters; ++i) {
    TGeoNode* daughter = originalVolume->GetNode(i);
    if (daughter) {
      dummyVol->AddNode(daughter->GetVolume(), daughter->GetNumber(), daughter->GetMatrix());
    }
  }

  TEveManager::Create(kFALSE);
  
  auto* en = new TEveGeoTopNode(geo, barrel);
  en->SetElementName("FV0_1");
  en->SetRnrSelf(kFALSE);
  en->SetRnrChildren(kTRUE);
  
  gEve->AddGlobalElement(en);
  en->ExpandIntoListTreesRecursively();
  setEveLeafOnly(en);

  printf("[INFO] Removing dummy container from tree structure\n");
  
  TEveElement::List_i it = en->BeginChildren();
  while (it != en->EndChildren()) {
    TEveElement* child = *it;
    if (auto* geoChild = dynamic_cast<TEveGeoNode*>(child)) {
      if (geoChild->GetNode() && geoChild->GetNode()->GetVolume()) {
        TString volName = geoChild->GetNode()->GetVolume()->GetName();
        if (volName.Contains("FV0_invisible") || volName.Contains("DummyBox")) {
          printf("[INFO] Found and removing dummy element: %s\n", volName.Data());
          
          TEveElement::List_i dummyChildIt = geoChild->BeginChildren();
          std::vector<TEveElement*> childrenToMove;
          
          while (dummyChildIt != geoChild->EndChildren()) {
            childrenToMove.push_back(*dummyChildIt);
            ++dummyChildIt;
          }
          
          for (auto* childToMove : childrenToMove) {
            geoChild->RemoveElement(childToMove);
            en->AddElement(childToMove);
          }

          en->RemoveElement(geoChild);
          break;
        }
      }
    }
    ++it;
  }

  printf("[INFO] Removing unwanted branches from tree\n");
  removeUnwantedBranches(en, unwantedBranches);

  en->Save(outSimpleGeomRoot, "FV0");

  barrel->SetVolume(originalVolume);
  std::vector<std::string> paths;
  collectLeafPaths(barrel, "", paths);

  FILE* fp = fopen(outSimpleGeomTxt, "w");
  if (!fp) { Error("make_simple_geom_fv0","Cannot write %s", outSimpleGeomTxt); return; }
  for (auto& p : paths) {
    // ensure leading '/'
    if (p.empty() || p[0] != '/') fprintf(fp, "/%s\n", p.c_str());
    else                          fprintf(fp, "%s\n",  p.c_str());
  }
  fclose(fp);

  printf("[OK] Wrote extract: %s (key: FV0, cleaned tree with %d original daughters)\n", outSimpleGeomRoot, nDaughters);
  printf("[OK] Wrote paths:   %s (n=%zu, rooted at /barrel_1)\n", outSimpleGeomTxt, paths.size());
}

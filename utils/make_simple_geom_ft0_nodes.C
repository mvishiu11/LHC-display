// make_simple_geom_ft0_nodes.C
// root -l -q 'make_simple_geom_ft0_nodes.C+(".../FT0_full.root",
//                                           ".../simple_geom_FT0.root",
//                                           ".../simple_geom_FT0.txt")'

#include "TFile.h"
#include "TKey.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TString.h"
#include "TEveManager.h"
#include "TEveGeoNode.h"
#include "TEveElement.h"
#include <vector>
#include <string>
#include <memory>

static TGeoManager* loadGeo(const char* f) {
  if (TGeoManager::Import(f)) return gGeoManager;
  std::unique_ptr<TFile> tf(TFile::Open(f));
  if (!tf || tf->IsZombie()) return nullptr;
  TIter it(tf->GetListOfKeys());
  while (auto* k=(TKey*)it()) if (!strcmp(k->GetClassName(),"TGeoManager")) return (TGeoManager*)k->ReadObj();
  return nullptr;
}

static void collectLeafPaths(TGeoNode* n, const std::string& cur, std::vector<std::string>& out) {
  if (!n) return;
  std::string here = cur + "/" + n->GetName();
  int nd=n->GetNdaughters();
  if (nd==0) { out.push_back(here); return; }
  for (int i=0;i<nd;++i) collectLeafPaths(n->GetDaughter(i), here, out);
}

static void findByPrefix(TGeoNode* n, const char* pfx, std::vector<TGeoNode*>& out) {
  if (!n) return;
  if (TString(n->GetName()).BeginsWith(pfx)) out.push_back(n);
  for (int i=0;i<n->GetNdaughters();++i) findByPrefix(n->GetDaughter(i), pfx, out);
}

void make_simple_geom_ft0_nodes(const char* inFullRoot,
                                const char* outSimpleRoot,
                                const char* outTxt)
{
  TGeoManager* geo = loadGeo(inFullRoot);
  if (!geo) { Error("make_simple_geom_ft0_nodes","cannot load %s", inFullRoot); return; }
  TGeoNode* top = geo->GetTopNode();
  if (!top) { Error("make_simple_geom_ft0_nodes","no top node"); return; }

  // Find actual FT0 placements (adjust prefixes if your names differ)
  std::vector<TGeoNode*> ft0nodes;
  findByPrefix(top, "FT0A", ft0nodes);
  findByPrefix(top, "FT0C", ft0nodes);
  if (ft0nodes.empty()) { Warning("make_simple_geom_ft0_nodes","No FT0A*/FT0C* nodes found; exporting nothing"); }

  // Build an EVE group called "FT0" that contains ONLY those nodes
  TEveManager::Create(kFALSE);
  auto* group = new TEveElementList("FT0"); // This becomes the top-key name in the .root
  gEve->AddGlobalElement(group);

  for (auto* n : ft0nodes) {
    auto* tn = new TEveGeoTopNode(geo, n);
    tn->SetVisLevel(10);
    // ensure parent containers invisible even if present on the path
    if (n->GetVolume()) n->GetVolume()->SetVisibility(kTRUE);
    group->AddElement(tn);
    tn->ExpandIntoListTreesRecursively();
  }

  group->Save(outSimpleRoot, "FT0"); // no "cave" saved

  // Build the .txt list: full paths to those FT0 nodes (and their leaves)
  std::vector<std::string> paths;
  for (auto* n : ft0nodes) collectLeafPaths(n, "", paths);

  FILE* f = fopen(outTxt,"w");
  if (!f) { Error("make_simple_geom_ft0_nodes","cannot write %s", outTxt); return; }
  for (auto& p : paths) fprintf(f, "%s%s\n", (p.empty()||p[0]=='/')?"":"/", p.c_str());
  fclose(f);

  printf("[OK] wrote %s (key: FT0) and %s (paths=%zu)\n", outSimpleRoot, outTxt, paths.size());
}

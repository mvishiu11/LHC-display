// drop_topshape_move.C
#include "TFile.h"
#include "TSystem.h"
#include "TEveElement.h"

void drop_topshape(const char* file="simple_geom_FT0.root",
                        const char* key ="FT0")
{
  gSystem->Load("libEve"); // just in case

  TFile f(file,"UPDATE");
  auto* oldTop = dynamic_cast<TEveElement*>(f.Get(key));
  if (!oldTop) { Error("drop_topshape_move","Key '%s' not found in %s", key, file); return; }

  // Make a pure container for the new top (no fShape at all)
  auto* newTop = new TEveElementList(key);
  newTop->SetTitle(oldTop->GetElementTitle());
  newTop->SetRnrSelf(kFALSE);
  newTop->SetRnrChildren(kTRUE);

  // Snapshot current children, then MOVE them from oldTop to newTop
  std::vector<TEveElement*> kids;
  for (auto it = oldTop->BeginChildren(); it != oldTop->EndChildren(); ++it)
    kids.push_back(*it);

  for (auto* ch : kids) {
    oldTop->RemoveElement(ch);
    newTop->AddElement(ch); // takes ownership; parent becomes newTop
  }

  // Overwrite the FT0 key with the list-only object
  f.WriteTObject(newTop, key, "SingleKey");
  f.Close();

  printf("[OK] Replaced top '%s' with list-only container (no fShape).\n", key);
}

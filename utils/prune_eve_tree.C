// Minimal ROOT-directory pruner (no EVE headers).
// Works when your geometry is stored as a TDirectory tree like:
//   simple_geom_FT0.root / FT0 / { fElements/..., cave, fName, fTitle, ... }
//
// Two entry points:
//   void dir_prune_remove(in, out, top, "path1;path2;...");
//   void dir_prune_keep_only(in, out, top, "path1;path2;...");
//
// Paths are absolute from the top directory, e.g.:
//   "/cave" or "/fElements/caveRB24_1" or "/fElements/barrel_1"
//
// You can also do in-place removal: dir_prune_remove_inplace(file, top, "…")

#include "TFile.h"
#include "TDirectoryFile.h"
#include "TKey.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TError.h"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

static std::vector<std::string> splitList(const char* csv_or_semi) {
  std::vector<std::string> out;
  if (!csv_or_semi) return out;
  TString s(csv_or_semi);
  s.ReplaceAll(";", ",");
  std::unique_ptr<TObjArray> toks(s.Tokenize(","));
  if (!toks) return out;
  for (int i = 0; i < toks->GetEntries(); ++i) {
    TString t = static_cast<TObjString*>(toks->At(i))->String();
    t = t.Strip(TString::kBoth, ' ');
    if (!t.IsNull()) {
      if (!t.BeginsWith("/")) t.Prepend("/");
      out.emplace_back(t.Data());
    }
  }
  return out;
}

static bool startsWith(const std::string& a, const std::string& pref) {
  return a.size() >= pref.size()
      && std::equal(pref.begin(), pref.end(), a.begin());
}

static bool keepDecision(const std::string& path, const std::vector<std::string>& keepPrefixes) {
  if (keepPrefixes.empty()) return true;
  for (auto& kp : keepPrefixes) {
    if (path == kp) return true;               // exact
    if (startsWith(path, kp)) return true;     // inside a kept subtree
    if (startsWith(kp, path + "/")) return true; // ancestor of kept subtree
  }
  return false;
}

static bool removeDecision(const std::string& path, const std::vector<std::string>& rmPrefixes) {
  for (auto& rp : rmPrefixes) {
    if (startsWith(path, rp)) return true;
  }
  return false;
}

static void copyDirFiltered(TDirectory* src, TDirectory* dst,
                            const std::string& curPath,
                            const std::vector<std::string>& rmPref,
                            const std::vector<std::string>& keepPref,
                            bool keepMode)
{
  // iterate over keys in src
  TIter nextKey(src->GetListOfKeys());
  while (auto* key = static_cast<TKey*>(nextKey())) {
    const char* name = key->GetName();
    const char* clnm = key->GetClassName();
    std::string full = curPath + "/" + name;

    bool drop = false;
    if (keepMode) {
      if (!keepDecision(full, keepPref)) drop = true;
    } else {
      if (removeDecision(full, rmPref)) drop = true;
    }
    if (drop) continue;

    // directories vs objects
    if (TString(clnm).EqualTo("TDirectoryFile") || TString(clnm).EqualTo("TDirectory")) {
      // create subdir and recurse
      TDirectory* srcSub = static_cast<TDirectory*>(key->ReadObj());
      TDirectory* dstSub = dst->mkdir(name, srcSub->GetTitle());
      dstSub->cd();
      copyDirFiltered(srcSub, dstSub, full, rmPref, keepPref, keepMode);
    } else {
      // plain object: copy as object
      std::unique_ptr<TObject> obj(key->ReadObj());
      if (obj) {
        dst->cd();
        obj->Write(name, TObject::kOverwrite);
      }
    }
  }
}

static TDirectory* getTopDir(TFile& f, const char* topKey) {
  TObject* obj = nullptr;
  f.GetObject(topKey, obj);
  if (!obj) {
    TString k = TString::Format("%s;1", topKey);
    f.GetObject(k.Data(), obj);
  }
  auto* d = dynamic_cast<TDirectory*>(obj);
  if (!d) {
    ::Error("prune_dir_tree", "Top key '%s' is not a TDirectory in %s", topKey, f.GetName());
  }
  return d;
}

// ---------------- public APIs ----------------

void dir_prune_remove(const char* inFile,
                      const char* outFile,
                      const char* topKey,
                      const char* removePathsCSV)
{
  std::unique_ptr<TFile> fin(TFile::Open(inFile, "READ"));
  if (!fin || fin->IsZombie()) { ::Error("dir_prune_remove","Cannot open %s", inFile); return; }

  TDirectory* top = getTopDir(*fin, topKey);
  if (!top) return;

  auto rm = splitList(removePathsCSV);

  TFile fout(outFile, "RECREATE");
  if (fout.IsZombie()) { ::Error("dir_prune_remove","Cannot create %s", outFile); return; }

  // Make a top dir in the output with the same name
  TDirectory* outTop = fout.mkdir(topKey, top->GetTitle());
  copyDirFiltered(top, outTop, std::string(""), rm, /*keep*/{}, /*keepMode*/false);
  fout.Write();
  fout.Close();
  printf("[OK] Wrote %s (removed %zu subtree prefix(es)).\n", outFile, rm.size());
}

void dir_prune_keep_only(const char* inFile,
                         const char* outFile,
                         const char* topKey,
                         const char* keepPathsCSV)
{
  std::unique_ptr<TFile> fin(TFile::Open(inFile, "READ"));
  if (!fin || fin->IsZombie()) { ::Error("dir_prune_keep_only","Cannot open %s", inFile); return; }

  TDirectory* top = getTopDir(*fin, topKey);
  if (!top) return;

  auto kp = splitList(keepPathsCSV);
  if (kp.empty()) { ::Error("dir_prune_keep_only","No keep paths specified."); return; }

  TFile fout(outFile, "RECREATE");
  if (fout.IsZombie()) { ::Error("dir_prune_keep_only","Cannot create %s", outFile); return; }

  TDirectory* outTop = fout.mkdir(topKey, top->GetTitle());
  copyDirFiltered(top, outTop, std::string(""), /*rm*/{}, kp, /*keepMode*/true);
  fout.Write();
  fout.Close();
  printf("[OK] Wrote %s (kept %zu subtree(s)).\n", outFile, kp.size());
}

// Optional: in-place delete (UPDATE). Uses TDirectory::Delete on exact names.
// Only removes *exact* subdirs/objects (no recursion), so pass deepest paths or call twice.
void dir_prune_remove_inplace(const char* file, const char* topKey, const char* removePathsCSV)
{
  std::unique_ptr<TFile> f(TFile::Open(file, "UPDATE"));
  if (!f || f->IsZombie()) { ::Error("dir_prune_remove_inplace","Cannot open %s", file); return; }

  TDirectory* top = getTopDir(*f, topKey);
  if (!top) return;

  auto rm = splitList(removePathsCSV);
  for (auto& p : rm) {
    // Navigate to parent dir
    TString sp(p.c_str());
    sp = sp.Strip(TString::kBoth, '/');
    Ssiz_t slash = sp.Last('/');
    TString parent = (slash>=0) ? sp(0, slash) : "";
    TString leaf   = (slash>=0) ? sp(slash+1, sp.Length()-slash-1) : sp;

    TDirectory* dir = top;
    if (!parent.IsNull()) {
      if (!dir->cd(parent)) { ::Warning("dir_prune_remove_inplace","Parent '%s' not found, skipping %s", parent.Data(), p.c_str()); continue; }
      dir = gDirectory;
    }
    dir->Delete(TString::Format("%s;*", leaf.Data()));
  }
  f->Write();
  f->Close();
  printf("[OK] In-place removal done in %s.\n", file);
}

// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   ChannelGeometry.h
/// \author Artur Furs afurs@cern.ch
///

#ifndef QC_MODULE_FT0_CHANNELGEOMETRY_H_CUSTOM
#define QC_MODULE_FT0_CHANNELGEOMETRY_H_CUSTOM

#include <TH2Poly.h>

#include <map>
#include <memory>
#include <string>

#include <ROOT/RCsvDS.hxx>
#include <Framework/Logger.h>
#include <unordered_map>
#include <cstdlib>
#include <iostream>
#include "DataFormatsGlobalTracking/RecoContainer.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsFT0/ChannelData.h"
#include "DataFormatsFT0/RecPoints.h"
namespace o2::ft0

{
// Test entity for holding hit positions in Event

struct HitFT0  {
  HitFT0() = default;
  HitFT0(double val, double x, double y, double z, int channelID): mVal(val), mX(x), mY(y), mZ(z), mChannelID(channelID) {};
  HitFT0(const HitFT0&) = default;
  ~HitFT0() = default;
  double mVal;
  double mX;
  double mY;
  double mZ;
  double mChannelID;
};
class ChannelGeometry
{
 public:
  ChannelGeometry() = default;
  ~ChannelGeometry() = default;
  typedef TH2Poly Hist_t;
  typedef std::map<int, int> ChannelMap_t;   // chID -> bin
  typedef std::pair<double, double> Point_t; // X/y coordinates
  typedef std::map<int, Point_t> ChannelGeometryMap_t;

  ChannelGeometryMap_t mChannelGeometryMap{};
  ChannelGeometryMap_t mChannelGeometryMap_sideA{};
  ChannelGeometryMap_t mChannelGeometryMap_sideC{};
  ChannelMap_t mChannelMapA{}; // A-side
  ChannelMap_t mChannelMapC{}; // C-side
  double mMargin{ 10. };       // margin between channels
  void parseChannelTable(const std::string& filepath, char delimeter = ';', bool usePolyHist = false);
  void makeChannel(int chID, double x, double y);
  void initHists(double xMin, double xMax, double yMin, double yMax);
  void init(double xMin, double xMax, double yMin, double yMax, double margin, const std::string &filepath);
  void clear();

  std::unique_ptr<Hist_t> makeHistSideA(const std::string& histName, const std::string& histTitle);
  std::unique_ptr<Hist_t> makeHistSideC(const std::string& histName, const std::string& histTitle);
  void setBinContent(Hist_t* histSideA, Hist_t* histSideC, int chID, double val);

  HitFT0 createHit(double val, int channelID) {
    double z{};
    Point_t point{};
    if(channelID<96) {
      z=3.3;
      const auto it = mChannelGeometryMap_sideA.find(channelID);
      if(it!=mChannelGeometryMap_sideA.end()) {
        point = it->second;
      }
    }
    else if(channelID<208) {
      z=-0.83;
      const auto it = mChannelGeometryMap_sideC.find(channelID);
      if(it!=mChannelGeometryMap_sideC.end()) {
        point = it->second;
      }
    }
    const auto &x = point.first;
    const auto &y = point.second;
    return HitFT0(val, x, y, z, channelID);
  }

  template <typename HistSrcType>
  void convertHist1D(HistSrcType* histSrc, Hist_t* histSideA, Hist_t* histSideC)
  {
    for (int iBin = 0; iBin < histSrc->GetNbinsX(); iBin++) {
      const auto val = histSrc->GetBinContent(iBin + 1);
      setBinContent(histSideA, histSideC, iBin, val);
    }
  }

  static std::string getFilepath(const std::string& filename = "FT0_LUT.csv")
  {
    const auto pathEnv = std::getenv("QUALITYCONTROL_ROOT");
    const std::string subfilepath = "/etc/Modules/FIT/FT0/etc/" + filename;
    if (pathEnv) {
      return pathEnv + subfilepath;
    }
    return std::string{ "" };
  }

  //Example, non-optimized
  typedef std::map<o2::InteractionRecord, std::vector<HitFT0>> EventHitMap;
  EventHitMap getMapOfHits(const o2::globaltracking::RecoContainer &recoCont) {
    const auto vecRecPoints = recoCont.getFT0RecPoints();
    const auto vecChannels = recoCont.getFT0ChannelsData();
    EventHitMap eventHitMap{};
    for(const auto &recPoint: vecRecPoints) {
      const auto &ir = recPoint.getInteractionRecord();
      const auto& channels = recPoint.getBunchChannelData(vecChannels);
      auto itPair = eventHitMap.insert({ir, {}});
      for(const auto &channel: vecChannels) {
        const auto &val = channel.QTCAmpl;
        const auto &chID = channel.ChId;
        itPair.first->second.push_back(createHit(static_cast<double>(val), static_cast<int>(chID)));
      }
    }
    return eventHitMap;
  }
 private:
  std::unique_ptr<Hist_t> mHistSideA; //! hist template for A-side, use Clone()
  std::unique_ptr<Hist_t> mHistSideC; //! hist template for C-side, use Clone()
  bool mIsOk{ true };
};
void ChannelGeometry::parseChannelTable(const std::string& filepath, char delimiter, bool usePolyHist)
{
  clear();
  try {
    std::unordered_map<std::string, char> colTypes = { { "Long signal cable #", 'T' } };
    auto dataframe = ROOT::RDF::FromCSV(filepath.c_str(), true, delimiter, -1LL, std::move(colTypes));
    dataframe.Foreach([&channelGeometryMap = this->mChannelGeometryMap, &mapSideA = this->mChannelGeometryMap_sideA, &mapSideC = this->mChannelGeometryMap_sideC](const Long64_t& chID, const double& x, const double& y) {
      channelGeometryMap.insert({ static_cast<int>(chID), { x, y } });
      if(chID<96) {
        mapSideA.insert({ static_cast<int>(chID), { x, y } });
      }
      else if(chID<208) {
        mapSideC.insert({ static_cast<int>(chID), { x, y } });
      }
    },
                      { "channel #", "coordinate X in mm", "coordinate Y in mm" });
    if(mHistSideA && mHistSideC && usePolyHist) {
      for (const auto& [chID, point] : mChannelGeometryMap) {
        // temporary hardcoded
        const auto& x = point.first;
        const auto& y = point.second;
        makeChannel(chID, x, y);
      }
    }
    else if(usePolyHist) {
      LOG(error)<<"Expects init for TH2Poly";
    }
  } catch (std::exception const& e) {
    mIsOk = false;
    LOG(error)<<"FT0 channel map arsing error: " << e.what();
  }
}

void ChannelGeometry::makeChannel(int chID, double x, double y)
{
  // For further development
  std::array<double, 4> x_borders = { x - mMargin, x + mMargin, x + mMargin, x - mMargin };
  std::array<double, 4> y_borders = { y + mMargin, y + mMargin, y - mMargin, y - mMargin };
  // side temporary hardcoded for FT0, should be taken from csv
  if (chID < 96) {
    const auto bin = mHistSideA->AddBin(4, x_borders.data(), y_borders.data());
    mChannelMapA.insert({ chID, static_cast<int>(bin) });
  } else if (chID < 208) {
    const auto bin = mHistSideC->AddBin(4, x_borders.data(), y_borders.data());
    mChannelMapC.insert({ chID, static_cast<int>(bin) });
  }
}

void ChannelGeometry::initHists(double xMin, double xMax, double yMin, double yMax)
{
  mHistSideA = std::make_unique<ChannelGeometry::Hist_t>("hDummyGeometryFT0A", "hDummyGeometryFT0A", xMin, xMax, yMin, yMax);
  mHistSideC = std::make_unique<ChannelGeometry::Hist_t>("hDummyGeometryFT0C", "hDummyGeometryFT0C", xMin, xMax, yMin, yMax);
}

void ChannelGeometry::init(double xMin, double xMax, double yMin, double yMax, double margin, const std::string &filepath)
{
  mMargin = margin;
  initHists(xMin, xMax, yMin, yMax);
//  const auto& filepath = ChannelGeometry::getFilepath();
  parseChannelTable(filepath);
}

std::unique_ptr<ChannelGeometry::Hist_t> ChannelGeometry::makeHistSideA(const std::string& histName, const std::string& histTitle)
{
  std::unique_ptr<ChannelGeometry::Hist_t> histPtr(dynamic_cast<ChannelGeometry::Hist_t*>(mHistSideA->Clone(histName.c_str())));
  histPtr->SetTitle(histTitle.c_str());
  return std::move(histPtr);
}

std::unique_ptr<ChannelGeometry::Hist_t> ChannelGeometry::makeHistSideC(const std::string& histName, const std::string& histTitle)
{
  std::unique_ptr<ChannelGeometry::Hist_t> histPtr(dynamic_cast<ChannelGeometry::Hist_t*>(mHistSideC->Clone(histName.c_str())));
  histPtr->SetTitle(histTitle.c_str());
  return std::move(histPtr);
}
void ChannelGeometry::setBinContent(Hist_t* histSideA, Hist_t* histSideC, int chID, double val)
{
  const auto itSideA = mChannelMapA.find(chID);
  const auto itSideC = mChannelMapC.find(chID);
  if (histSideA && itSideA != mChannelMapA.end()) {
    histSideA->SetBinContent(itSideA->second, val);
  } else if (histSideC && itSideC != mChannelMapC.end()) {
    histSideC->SetBinContent(itSideC->second, val);
  }
}

void ChannelGeometry::clear()
{
  mChannelGeometryMap.clear();
  mChannelMapA.clear();
  mChannelMapA.clear();
  if(mHistSideA) mHistSideA->Reset("");
  if(mHistSideC) mHistSideC->Reset("");
  mIsOk = true;
}



} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_CHANNELGEOMETRY_H_CUSTOM

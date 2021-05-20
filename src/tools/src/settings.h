#ifndef NPDET_TOOLS_SETTINGS_H
#define NPDET_TOOLS_SETTINGS_H

#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/VectorUtil.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TTree.h"
#include "TF1.h"

#include <vector>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <thread>
#include <filesystem>
namespace fs = std::filesystem;

#include "DD4hep/Detector.h"
#include "DD4hep/Printout.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/SurfaceManager.h"
#include "DDRec/Surface.h"
#include "DD4hep/DD4hepUnits.h"

#include "TApplication.h"
#include "ROOT/TFuture.hxx"

#include "TGeoManager.h"
#include "Math/DisplacementVector3D.h"
#include "TEveManager.h"
#include "TEveGeoNode.h"
#include "TEveGeoShapeExtract.h"
#include "TEveGeoShape.h"
#include "TEveBrowser.h"


enum class mode { none, help, list, part };

struct settings {
  using string   = std::string;
  using part_map = std::map<string,int>;
  bool            help              = false;
  bool            success           = false;
  string          infile            = "";
  string          outfile           = "detector_geometry";
  string          part_name         = "";
  int             part_level        = -1;
  mode            selected          = mode::list;
  bool            level_set         = false;
  int             global_level      = 1;
  bool            list_all          = false;
  int     color          = 1;
  double  alpha          = 1;
  std::map<std::string,int> part_name_levels;
  std::map<std::string, int>   part_name_colors;
  std::map<std::string,double> part_name_alphas;
};

void print_daughter_nodes(TGeoNode* node, int print_depth) ;
void run_list_mode(const settings& s);

//struct settings2 {
//  bool help           = false;
//  bool success        = false;
//  std::string infile  = "";
//  std::string outfile = "detector_geometry";
//  std::string p_name  = "";
//  int     part_level  = -1;
//  bool    level_set      = false;
//  int     geo_level      = -1;
//  bool    list_all       = false;
//  mode    selected       = mode::list;
//};
////______________________________________________________________________________

#endif

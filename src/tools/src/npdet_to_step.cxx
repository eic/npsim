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

// DD4hep
// -----
// In .rootlogon.C
//  gSystem->Load("libDDDetectors");
//  gSystem->Load("libDDG4IO");
//  gInterpreter->AddIncludePath("/opt/software/local/include");
#include "DD4hep/Detector.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/SurfaceManager.h"
#include "DDRec/Surface.h"
#include "DD4hep/DD4hepUnits.h"

#include "TApplication.h"
#include "TMultiGraph.h"
#include "TGraph.h"
#include "TGeoToStep.h"
#include "TGeoManager.h"

#include "Math/DisplacementVector3D.h"

#include <iostream>
#include <string>
#include "clipp.h"

using namespace clipp;

struct settings {
  bool success = false;
  std::string infile = "";
  ROOT::Math::XYZVector  start_position{0.0,0.0,0.0};
  ROOT::Math::XYZVector  direction{0.0,0.0,1.0};
  double step_size   = 1.0;
  bool   rec    = false; 
  bool   utf16  = false;
  bool   search = false;
  int    geo_level = 0;
  std::vector<std::string> field_comps;
  std::string fmt    = "json";
  std::vector<std::string> axes;
  double x0 = 0.0;
  double y0 = 0.0;
  double z0 = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  double z1 = 0.0;
  double x2 = 0.0;
  double y2 = 0.0;
  double z2 = 0.0;
};

settings cmdline_settings(int argc, char* argv[]) {
  settings s;
  auto lastOpt = " options:" % (
    option("-l","--level") & integer("level",s.geo_level),
    value("file",s.infile).if_missing([]{ std::cout << "You need to provide an input xml filename as the last argument!\n"; } )
    % "input xml file",
    //option("-r", "--recursive") % "descend into subdirectories",
    option("-h", "--help")      % "show help"
    );

  auto cli = (
    lastOpt
    );
  
  if(!parse(argc, argv, cli)) {
    s.success = false;
    std::cout << make_man_page(cli, argv[0]).prepend_section("DESCRIPTION",
                                                             "    The best thing since sliced bread.");
    return s;
  }
  using namespace dd4hep;

  s.direction.SetXYZ(s.x2*cm,s.y2*cm,s.z2*cm);
  s.direction = s.direction.Unit();
  s.start_position.SetXYZ(s.x0*cm,s.y0*cm,s.z0*cm);
  s.success = true;
  return s;
}
//______________________________________________________________________________


int main (int argc, char *argv[]) {

  settings s = cmdline_settings(argc,argv);
  if( !s.success ) {
    return 1;
  }

  // ------------------------
  // TODO: CLI Checks 

  using namespace dd4hep;
  
  // -------------------------
  // Get the DD4hep instance
  // Load the compact XML file
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  TGeoToStep * mygeom= new TGeoToStep( &(detector.manager()) );
  mygeom->CreateGeometry(s.geo_level);
  //detector.manager().Export("geometry.gdml");
  std::cout << "saved as geometry.stp\n";
  return 0;
} 
//______________________________________________________________________________



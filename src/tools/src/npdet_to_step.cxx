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
  int    search_level = 0;
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

  auto helpMode = "help mode:" % (
    command("help")
    );
 
  auto drawMode = "draw mode:" % (
    command("draw"),
    values("component").set(s.field_comps), 
    option("--step") & number("step",s.step_size),
    option("--start") & (
      number("x",s.x0 ).if_missing([]{ std::cout << "x missing!\n"; } ),
      number("y",s.y0 ).if_missing([]{ std::cout << "y missing!\n"; } ),
      number("z",s.z0 ).if_missing([]{ std::cout << "z missing!\n"; } )),
    option("--end") & (
      number("x1",s.x1 ).if_missing([]{ std::cout << "x1 missing!\n"; } ),
      number("y1",s.y1 ).if_missing([]{ std::cout << "y1 missing!\n"; } ),
      number("z1",s.z1 ).if_missing([]{ std::cout << "z1 missing!\n"; } )),
    option("--direction") & (
      number("x2",s.x2 ).if_missing([]{ std::cout << "x2 missing!\n"; } ),
      number("y2",s.y2 ).if_missing([]{ std::cout << "y2 missing!\n"; } ),
      number("z2",s.z2 ).if_missing([]{ std::cout << "z2 missing!\n"; } )),
    //required("--vs","--Vs")    & values("axes",s.axes ), % "axes"
    option("-l","--level") & integers("level",s.search_level) % "search level"
    );

  auto printMode = "print mode:" % (
    command("print") | command("dump"),
    option("--all")         % "copy all",
    option("--replace")     % "replace existing files",
    option("-f", "--force") % "don't ask for confirmation"
    );

  auto firstOpt = "user interface options:" % (
    joinable(
      option("-v", "--verbose")     % "show detailed output",
      option("-i", "--interactive") % "use interactive mode"
      )
    );
  auto lastOpt = "mode-independent options:" % (
    value("file",s.infile).if_missing([]{ std::cout << "You need to provide an input xml filename as the last argument!\n"; } )
    % "input xml file",
    //option("-r", "--recursive") % "descend into subdirectories",
    option("-h", "--help")      % "show help"
    );

  auto cli = (
    //firstOpt,
    //drawMode | printMode | command("list"),
    lastOpt
    );
  //auto cli = (
  //  command("help")
  //  command("search")
  //  );
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
  mygeom->CreateGeometry();
  //detector.manager().Export("geometry.gdml");
  std::cout << "saved as geometry.stp\n";
  return 0;
} 
//______________________________________________________________________________



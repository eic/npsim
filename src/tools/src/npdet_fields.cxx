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


#include "Math/DisplacementVector3D.h"

TGraph* build_1D_field_graph(dd4hep::Detector& detector, ROOT::Math::XYZVector start, ROOT::Math::XYZVector dir, double step, std::function<double(ROOT::Math::XYZVector)> B_comp);

/** Cell size example.
 *
 */
void mag_field(dd4hep::Detector& detector){

  double z0 = 0.0;
  using namespace ROOT::Math;
  using namespace dd4hep;

  double x_min = -100.0;
  double x_max =  100.0;
  double y_min = -100.0;
  double y_max =  100.0;
  double z_min = -100.0;
  double z_max =  100.0;
  int nx_bins = 200;
  int ny_bins = 200;
  int nz_bins = 200;
  double dx = (x_max -x_min)/nx_bins;
  double dy = (y_max -y_min)/ny_bins;
  double dz = (z_max -z_min)/nz_bins; 

  double y0 =  20.0*cm;

  TH2F* hBx = new TH2F("hBx","; z [cm]; x [cm]; B_{x} [T]",nz_bins,z_min,z_max,nx_bins,x_min,x_max);
  TH2F* hBz = new TH2F("hBz","; z [cm]; x [cm]; B_{z} [T]",nz_bins,z_min,z_max,nx_bins,x_min,x_max);
  TH2F* hBr = new TH2F("hBr","; z [cm]; x [cm]; B_{r} [T]",nz_bins,z_min,z_max,nx_bins,x_min,x_max);
  TH2F* hBphi = new TH2F("hBphi","; z [cm]; x [cm]; B_{#phi} [T]",nz_bins,z_min,z_max,nx_bins,x_min,x_max);


  for(int ix = 0; ix< nx_bins; ix++) {
    for(int iz = 0; iz< nz_bins; iz++) {

      double x = (x_min + ix*dx)*cm;
      double z = (z_min + iz*dz)*cm;

      XYZVector pos{ x, y0, z };
      XYZVector field = detector.field().magneticField(pos);

      hBz->Fill(z,x,   field.z()/tesla);
      hBx->Fill(z,x,   field.x()/tesla);
      hBr->Fill(z,x,   field.rho()/tesla);
      hBphi->Fill(z,x, field.phi()/tesla);
    }

  }

  auto c = new TCanvas();
  c->Divide(2,2);

  c->cd(1);
  hBz->Draw("colz");

  c->cd(2);
  hBx->Draw("colz");

  c->cd(3);
  hBr->Draw("colz");

  c->cd(4);
  hBphi->Draw("colz");
}

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
    firstOpt,
    drawMode | printMode | command("list"),
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


  int root_argc = 0;
  char *root_argv[1];
  argv[0] = "npdet_fields";

  TApplication theApp("tapp", &root_argc, root_argv);

  TMultiGraph* mg = new TMultiGraph();
  for(const auto& comp : s.field_comps){
    std::cout << comp << "\n";

    auto gr = build_1D_field_graph(detector, 
                                   s.start_position, 
                                   s.start_position+100.0*s.step_size*s.direction, 
                                   s.step_size,
                                   [](ROOT::Math::XYZVector B){return B.z();});
    gr->SetLineColor(2);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{z}");
    mg->Add(gr,"l");

    gr = build_1D_field_graph(detector, 
                                   s.start_position, 
                                   s.start_position+100.0*s.step_size*s.direction, 
                                   s.step_size,
                                   [&](ROOT::Math::XYZVector B){return B.Dot(s.direction);});
    gr->SetLineColor(1);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{parallel}");
    mg->Add(gr,"l");

    gr = build_1D_field_graph(detector, 
                                   s.start_position, 
                                   s.start_position+100.0*s.step_size*s.direction, 
                                   s.step_size,
                                   [&](ROOT::Math::XYZVector B){return (B - B.Unit()*(B.Dot(s.direction))).r();});
    gr->SetLineColor(4);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{perp}");
    mg->Add(gr,"l");

  }

  auto c = new TCanvas();
  mg->Draw("a");
  //mag_field( detector );
  c->BuildLegend();

  std::cout << "input file : " << s.infile << "\n";
  std::cout << "     start : " << s.start_position << "\n";
  std::cout << " direction : " << s.direction << "\n";
  std::cout << "      step : " << s.step_size << "\n";


  theApp.Run();
  return 0;
} 
//______________________________________________________________________________

TGraph* build_1D_field_graph(dd4hep::Detector&     detector,
                             ROOT::Math::XYZVector start,
                             ROOT::Math::XYZVector end,
                             double                step,
                             std::function<double(ROOT::Math::XYZVector)> B_comp)
{
  using namespace dd4hep;
  
  int N_steps = (end-start).r()/step;
  std::cout << "Steps : " << N_steps << "\n"; 

  auto dir = (end-start).Unit();

  auto gr = new TGraph();
  for(int i = 0; i< N_steps; i++) {
    ROOT::Math::XYZVector pos = start + double(i)*step*dir;
    ROOT::Math::XYZVector field = detector.field().magneticField(pos);
    gr->SetPoint(i,pos.r(),B_comp(field)/tesla);
  }

  return gr;
}


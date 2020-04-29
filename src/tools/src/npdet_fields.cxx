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

#include <algorithm>
#include <iterator>
#include <tuple>
#include <vector>

#include "DD4hep/DD4hepUnits.h"
#include "DD4hep/Detector.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/Surface.h"
#include "DDRec/SurfaceManager.h"
#include "TApplication.h"
#include "TGraph.h"
#include "TMultiGraph.h"

#include "Math/DisplacementVector3D.h"

#include <iostream>
#include <string>
#include "clipp.h"

using namespace clipp;
using namespace ROOT::Math;
using namespace dd4hep;

struct settings {
  bool success = false;
  std::string infile = "";
  ROOT::Math::XYZVector  start_position{0.0,0.0,0.0};
  ROOT::Math::XYZVector  direction{0.0,0.0,1.0};
  double step_size   = 0.5*cm;
  bool   rec    = false; 
  bool   utf16  = false;
  bool   search = false;
  int    search_level = 0;
  std::vector<std::string> field_comps = {"Bz"};
  std::string format    = "json";
  std::vector<std::string> axes;
  int    Nsteps = 200;
  double x0 = 0.0;
  double y0 = 0.0;
  double z0 = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
  double z1 = 0.0;
  double x2 = 0.0;
  double y2 = 0.0;
  double z2 = 1.0;
  bool   by_step_size = false;
  bool   with_end_point = false;
  bool verbose = false;
};

settings cmdline_settings(int argc, char* argv[]) {
  settings s;

  auto helpMode = "help mode:" % (
    command("help")
    );

  auto drawMode =
      "draw mode:" %
      (command("draw"), // values("component").set(s.field_comps),
       option("--Nsteps") & number("Nsteps", s.step_size),
        option("--step") & number("step", s.step_size),
       option("--start") & (number("x", s.x0).if_missing([] { std::cout << "x missing!\n"; }),
                            number("y", s.y0).if_missing([] { std::cout << "y missing!\n"; }),
                            number("z", s.z0).if_missing([] { std::cout << "z missing!\n"; })),
       (option("--end").set(s.with_end_point, true) & (number("x1", s.x1).if_missing([] { std::cout << "x1 missing!\n"; }),
                           number("y1", s.y1).if_missing([] { std::cout << "y1 missing!\n"; }),
                           number("z1", s.z1).if_missing([] { std::cout << "z1 missing!\n"; })) |
        option("--direction") &
            (number("x2", s.x2).if_missing([] { std::cout << "x2 missing!\n"; }),
             number("y2", s.y2).if_missing([] { std::cout << "y2 missing!\n"; }),
             number("z2", s.z2).if_missing([] { std::cout << "z2 missing!\n"; })))
       // required("--vs","--Vs")    & values("axes",s.axes ), % "axes"
       //option("-l", "--level") & integers("level", s.search_level) % "search level"
       );

  auto printMode = "integral mode (not implemented):" % (
    command("integrate") 
    //option("--all")         % "copy all"
    //option("--replace")     % "replace existing files",
    //option("-f", "--force") % "don't ask for confirmation"
    );

  auto firstOpt = "user interface options:" % (
    //joinable(
    option("-v", "--verbose").set(s.verbose)     % "show detailed output",
    option("-h", "--help")      % "show help"
    //option("-i", "--interactive") % "use interactive mode (not implemented)"
     // )
    );
  auto lastOpt = value("file", s.infile).if_missing([] {
    std::cout << "You need to provide an input xml filename as the last argument!\n";
  }) % "compact description should probly have a \"fields\" sect4ion with more than one magnetic "
       "field defined.";

  auto cli = (
    firstOpt,
    drawMode,// (| printMode | command("list")),
    lastOpt
    );
  //auto cli = (
  //  command("help")
  //  command("search")
  //  );
  if(!parse(argc, argv, cli)) {
    s.success = false;
    std::cout << make_man_page(cli, argv[0])
    .prepend_section("DESCRIPTION", " Tool for quickly looking at magnetic fields.")
    .append_section("EXAMPLES","   npdet_fields --start 0 0 -600 solid_sidis.xml\n");
    return s;
  }

  s.start_position.SetXYZ(s.x0*cm,s.y0*cm,s.z0*cm);
  if(s.with_end_point){
    s.direction = ROOT::Math::XYZVector(s.x1 * cm, s.y1 * cm, s.z1 * cm) - s.start_position;
    s.step_size = s.direction.R()/s.Nsteps;
    s.direction = s.direction.Unit();
  } else {
    s.direction.SetXYZ(s.x2 * cm, s.y2 * cm, s.z2 * cm);
    //s.step_size = s.direction.R()/s.Nsteps;
    s.direction = s.direction.Unit();
  }
  s.success   = true;

  std::cout << "start [cm] : " << s.start_position/cm <<"\n";
  std::cout << "dir   [cm] : " << s.direction/cm <<"\n";
  std::cout << "step  [cm] : " << s.step_size/cm <<"\n";
  std::cout << "nstep      : " << s.Nsteps <<"\n";

  return s;
}
//______________________________________________________________________________

TGraph* build_1D_field_graph(dd4hep::Detector& detector, const settings& s,
                             std::function<double(ROOT::Math::XYZVector)> B_comp);

/** Cell size example.
 *
 */
void mag_field(dd4hep::Detector& detector){

  double z0 = 0.0;

  double x_min   = -100.0;
  double x_max   = 100.0;
  double y_min   = -100.0;
  double y_max   = 100.0;
  double z_min   = -100.0;
  double z_max   = 100.0;
  int    nx_bins = 200;
  int    ny_bins = 200;
  int    nz_bins = 200;
  double dx      = (x_max - x_min) / nx_bins;
  double dy      = (y_max - y_min) / ny_bins;
  double dz      = (z_max - z_min) / nz_bins;

  double y0 = 20.0 * cm;

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

    auto gr = build_1D_field_graph(detector, s, [](ROOT::Math::XYZVector B) { return B.z(); });
    gr->SetLineColor(2);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{z}");
    mg->Add(gr,"l");

    gr = build_1D_field_graph(detector, s,
                              [&](ROOT::Math::XYZVector B) { return B.Rho(); });
    gr->SetLineColor(1);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{#perp}");
    mg->Add(gr,"l");

    gr = build_1D_field_graph(detector, s, [&](ROOT::Math::XYZVector B) {
      return (B - B.Unit() * (B.Dot(s.direction))).r();
    });
    gr->SetLineColor(4);
    gr->SetLineWidth(2);
    gr->SetFillColor(0);
    gr->SetTitle("B_{perp}");
    //mg->Add(gr,"l");

  }

  auto c = new TCanvas();
  mg->Draw("a");
  //mag_field( detector );
  c->BuildLegend();

  std::cout << "input file : " << s.infile << "\n";
  std::cout << "     start : " << s.start_position << "\n";
  std::cout << " direction : " << s.direction << "\n";
  std::cout << "      step : " << s.step_size << "\n";
  std::cout << "\n";


  theApp.Run();
  return 0;
} 
//______________________________________________________________________________

TGraph* build_1D_field_graph(dd4hep::Detector& detector, const settings& s,
                             std::function<double(ROOT::Math::XYZVector)> B_comp) {
  
  int N_steps = s.Nsteps;
  std::cout << "Steps : " << N_steps << "\n"; 

  auto dir = s.direction.Unit();

  auto gr = new TGraph();
  for (int i = 0; i < N_steps; i++) {
    ROOT::Math::XYZVector pos   = s.start_position + double(i) * s.step_size * dir;
    ROOT::Math::XYZVector field = detector.field().magneticField(pos);

    if (s.verbose) {
      std::cout << "x  : " << pos / mm << "\n";
      std::cout << "B  : " << field / tesla << "\n";
    }
    gr->SetPoint(i, pos.z(), B_comp(field) / tesla);
  }

  return gr;
}


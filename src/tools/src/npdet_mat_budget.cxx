//==========================================================================
//  AIDA Detector description implementation
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
//==========================================================================
//
//  Compute the  material budget in terms of integrated radiation and ineraction
//  lengths as seen from the IP for various subdetector regions given in
//  a simple steering file. Creates a root file w/ histograms and dumps numbers
//  the screen.
//
//  Author     : F.Gaede, DESY
//
//==========================================================================

#include "TError.h"

// Framework include files
#include "DD4hep/Detector.h"

#include "DD4hep/DetType.h"
#include "DD4hep/Printout.h"
#include "DD4hep/DetectorTools.h"
#include "DDRec/MaterialManager.h"
#include "DDRec/CellIDPositionConverter.h"

// #include "TGeoVolume.h"
// #include "TGeoManager.h"
// #include "TGeoNode.h"
#include "TFile.h"
#include "TH1F.h"

#include <fstream>
#include "clipp.h"
using namespace clipp;
enum class mode { none, help, list, part };

struct settings {
  bool                          help       = false;
  bool                          success    = false;
  std::string                   infile     = "";
  std::string                   steering   = "mat_steering.txt";
  std::string                   outfile    = "mat_budget.root";
  std::string                   p_name     = "";
  int                           part_level = -1;
  bool                          level_set  = false;
  int                           geo_level  = -1;
  int                           nbins      = 90;
  bool                          list_all   = false;
  mode                          selected   = mode::list;
  int                           color      = 1;
  double                        alpha      = 1;
  std::vector<int>              ids;
  std::map<std::string, int>    part_name_levels;
  std::map<std::string, int>    part_name_colors;
  std::map<std::string, double> part_name_alphas;
};
//______________________________________________________________________________

using namespace dd4hep;
using namespace dd4hep::rec;

void dumpExampleSteering();

namespace {

  struct SDetHelper {
    std::string name;
    double      r0;
    double      r1;
    double      z0;
    double      z1;
    TH1F*       hx;
    TH1F*       hl;
  };

  /// comput a point on a cylinder (r,z) for a given direction (theta,phi) from the IP
  Vector3D pointOnCylinder(double theta, double r, double z, double phi) {
    double theta0 = atan2(r, z);

    Vector3D v = (theta > theta0 ? Vector3D(r, phi, r / tan(theta), Vector3D::cylindrical)
                                 : Vector3D(z * tan(theta), phi, z, Vector3D::cylindrical));
    return v;
  }

} // namespace

template <typename T>
void print_usage(T cli, const char* argv0) {
  // used default formatting
  std::cout << "Usage:\n" << usage_lines(cli, argv0) << "\nOptions:\n" << documentation(cli) << '\n';
}
//______________________________________________________________________________

template <typename T>
void print_man_page(T cli, const char* argv0) {
  // all formatting options (with their default values)
  auto fmt = clipp::doc_formatting{}
                 .start_column(8)                    // column where usage lines and documentation starts
                 .doc_column(20)                     // parameter docstring start col
                 .indent_size(4)                     // indent of documentation lines for children of a documented group
                 .line_spacing(0)                    // number of empty lines after single documentation lines
                 .paragraph_spacing(1)               // number of empty lines before and after paragraphs
                 .flag_separator(", ")               // between flags of the same parameter
                 .param_separator(" ")               // between parameters
                 .group_separator(" ")               // between groups (in usage)
                 .alternative_param_separator("|")   // between alternative flags
                 .alternative_group_separator(" | ") // between alternative groups
                 .surround_group("(", ")")           // surround groups with these
                 .surround_alternatives("(", ")")    // surround group of alternatives with these
                 .surround_alternative_flags("", "") // surround alternative flags with these
                 .surround_joinable("(", ")")        // surround group of joinable flags with these
                 .surround_optional("[", "]")        // surround optional parameters with these
                 .surround_repeat("", "...");        // surround repeatable parameters with these
  //.surround_value("<", ">")                  //surround values with these
  //.empty_label("")                           //used if parameter has no flags and no label
  //.max_alternative_flags_in_usage(1)         //max. # of flags per parameter in usage
  //.max_alternative_flags_in_doc(2)           //max. # of flags per parameter in detailed documentation
  //.split_alternatives(true)                  //split usage into several lines for large alternatives
  //.alternatives_min_split_size(3)            //min. # of parameters for separate usage line
  //.merge_alternative_flags_with_common_prefix(false)  //-ab(cdxy|xy) instead of -abcdxy|-abxy
  //.merge_joinable_flags_with_common_prefix(true);    //-abc instead of -a -b -c

  auto mp = make_man_page(cli, argv0, fmt);
  //mp.prepend_section("DESCRIPTION", "Geometry tool for converting compact files to STEP (cad) files.");
  //mp.append_section("EXAMPLES", " $ npdet_to_teve list compact.xml");
  std::cout << mp << "\n";
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[]) {
  settings s;

  auto lastOpt =
      (option("-x").call([]() {
        dumpExampleSteering();
        exit(0);
      }) % "dump example steering to std out",
       option("-d", "--ID") & integers("id",s.ids) % "detector IDs to include in the budget",
       option("-h", "--help").set(s.selected, mode::help) % "show help",
       option("-n", "--n-bins") & integer("nbins", s.nbins) % "number of bins",
       option("-o", "--output") & value("mat_budget.root", s.outfile) % "output root file",
       required("-i","--input") & value("file", s.infile).if_missing([] {
         std::cout << "You need to provide an input xml filename as the last argument!\n";
       }) % "input compact detector description xml file");

  std::string wrong;
  auto        cli = (command("help").set(s.selected, mode::help) | (lastOpt), any_other(wrong));

  assert(cli.flags_are_prefix_free());

  auto res = parse(argc, argv, cli);

  // if( res.any_error() ) {
  //  s.success = false;
  //  std::cout << make_man_page(cli, argv[0]).prepend_section("error: ",
  //                                                           "    The best thing since sliced bread.");
  //  return s;
  //}
  s.success = true;

  if (s.selected == mode::help || !(wrong.empty())) {
    print_man_page<decltype(cli)>(cli, argv[0]);
  };
  return s;
}
//______________________________________________________________________________

int main(int argc, char* argv[]) {
  settings s = cmdline_settings(argc, argv);
  if (!s.success) {
    return 1;
  }

  if (s.selected == mode::help) {
    return 0;
  }

  for (auto id1 : s.ids) {
    std::cout << " checking for ID = " << id1 << "\n";
  }

  // ================== parse steering file ========================================================
  int                     nbins = s.nbins;
  double                  phi0  = M_PI / 2.;
  std::string             outFileName(s.outfile);
  std::vector<SDetHelper> subdets;

  std::ifstream infile(s.steering);
  std::string   line;

  while (std::getline(infile, line)) {
    // ignore empty lines and comments
    if (line.empty() || line.find("#") == 0)
      continue;

    std::istringstream iss(line);
    std::string        token;
    iss >> token;

    if (token == "nbins") {
      iss >> nbins;
    } else if (token == "rootfile") {
      iss >> outFileName;
    } else if (token == "phi") {
      iss >> phi0;
      phi0 = phi0 / 180. * M_PI;
    } else if (token == "subdet") {
      SDetHelper det;
      iss >> det.name >> det.r0 >> det.z0 >> det.r1 >> det.z1;
      subdets.emplace_back(det);
    }

    if (!iss.eof() || iss.fail()) {
      std::cout << " ERROR parsing line : " << line << std::endl;
      exit(1);
    }
  }

  // =================================================================================================
  setPrintLevel(WARNING);

  Detector& description = Detector::getInstance();
  description.fromXML(s.infile);
  CellIDPositionConverter  pos_converter(description);

  //for(const auto& d : description.detectors())
  //{
  //  std::cout << d.first << " -- " << d.second.name() << "\n";
  //  auto d2 =  description.detector(d.first);
  //  std::cout << "id : " << d2.id() << "\n";
  //  if(d2.id() == 0 ){
  //    continue;
  //  }
  //  std::cout << "volumeId : " << d2.volumeID() << "\n";
  //  std::cout << "path : " << d2.path() << "\n";
  //  std::cout << "placement volIDs : " << d2.placement().volIDs().size() << "\n";
  //  for(auto i : d2.placement().volIDs()){
  //    std::cout << i.first << " -- " << i.second << ",";
  //  }
  //  std::cout << "\n";
  //  std::cout << "child not found : " << d2.child("DFasdflkjadsf").name() << "\n";
  //}
  //----- open root file and book histograms
  TFile* rootFile = new TFile(outFileName.c_str(), "recreate");

  for (auto& det : subdets) {
    std::string hxn(det.name), hxnn(det.name);
    std::string hln(det.name), hlnn(det.name);
    hxn += "x0";
    hxnn += " integrated X0 vs -theta";
    det.hx = new TH1F(hxn.c_str(), hxnn.c_str(), nbins, -90., 0.);

    hln += "lambda";
    hlnn += " integrated int. lengths vs -theta";
    det.hl = new TH1F(hln.c_str(), hlnn.c_str(), nbins, -90., 0.);
  }
  //-------------------------

  Volume world = description.world().volume();

  MaterialManager matMgr(world);

  double dTheta = 0.5 * M_PI / nbins; // bin size

  std::cout << "===================================================================================================="
            << std::endl;

  std::cout << "theta:f/";
  for (auto& det : subdets) {
    std::cout << det.name << "_x0:f/" << det.name << "_lam:f/";
  }
  std::cout << std::endl;

  for (int i = 0; i < nbins; ++i) {
    double theta = (0.5 + i) * dTheta;

    std::cout << std::scientific << theta << " ";

    for (auto& det : subdets) {
      Vector3D p0 = pointOnCylinder(theta, det.r0, det.z0, phi0); // double theta, double r, double z, double phi)

      Vector3D p1 = pointOnCylinder(theta, det.r1, det.z1, phi0); // double theta, double r, double z, double phi)

      const PlacementVec& placements = matMgr.placementsBetween(p0, p1);

      //std::cout << " testing \n";
      //for(auto id1 : s.ids) {
      //  std::cout << " checking for ID = " << id1 << "\n";
      //  auto pv0 = pos_converter.findContext(id1)->elementPlacement();
      //  for(const auto& plmnt: placements)
      //  {
      //    dd4hep::detail::tools::PlacementPath pp;
      //    auto de = pos_converter.findDetElement( plmnt.first.position() );
      //    //if (dd4hep::detail::tools::findChild(pv0,de.placement(),pp))
      //    {
      //    std::cout << "derp\n";
      //    } else {
      //    std::cout << "child not found\n";

      //    }
      //    //for (const auto& j : pv.volIDs()) {
      //    //  std::cout << j.first << " -- " << j.second << "\n";
      //    //}
      //    //double length = plmnt.second;
      //  }
      //}


      const MaterialVec& materials = matMgr.materialsBetween(p0, p1);

      double sum_x0(0.), sum_lambda(0.), path_length(0.);

      for (auto amat : materials) {
        TGeoMaterial* mat    = amat.first->GetMaterial();
        double        length = amat.second;
        double        nx0    = length / mat->GetRadLen();
        sum_x0 += nx0;
        double nLambda = length / mat->GetIntLen();
        sum_lambda += nLambda;
        path_length += length;
      }

      det.hx->Fill(-theta / M_PI * 180., sum_x0);
      det.hl->Fill(-theta / M_PI * 180., sum_lambda);

      std::cout << std::scientific << sum_x0 << "  " << sum_lambda << "  "; // << path_length ;
    }
    std::cout << std::endl;
  }
  std::cout << "===================================================================================================="
            << std::endl;

  rootFile->Write();
  rootFile->Close();

  return 0;
}

void dumpExampleSteering() {
  std::cout << "# Example steering file for materialBudget  (taken from ILD_l5_v02)" << std::endl;
  std::cout << std::endl;
  std::cout << "# root output file" << std::endl;
  std::cout << "rootfile material_budget.root" << std::endl;
  std::cout << "# number of bins for polar angle (default 90)" << std::endl;
  std::cout << "nbins 90" << std::endl;
  std::cout << std::endl;
  std::cout << "# phi direction in deg (default: 90./y-axis)" << std::endl;
  std::cout << "phi 90." << std::endl;
  std::cout << std::endl;
  std::cout << "# names and subdetector ranges given in [rmin,zmin,rmax,zmax] - e.g. for ILD_l5_vo2  (run dumpdetector "
               "-d to get numbers... ) "
            << std::endl;
  std::cout << std::endl;
  std::cout << "subdet vxd    0. 0. 6.549392e+00 1.450000e+01" << std::endl;
  std::cout << "subdet sitftd 0. 0. 3.024000e+01 2.211800e+02" << std::endl;
  std::cout << "subdet tpc    0. 0. 1.692100e+02 2.225000e+02" << std::endl;
  std::cout << "subdet outtpc 0. 0. 1.769800e+02 2.350000e+02" << std::endl;
  std::cout << "subdet set    0. 0. 1.775200e+02 2.300000e+02" << std::endl;
  exit(0);
}


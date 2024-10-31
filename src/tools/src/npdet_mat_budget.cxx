#include "TError.h"
#include "DD4hep/Detector.h"

#include "DD4hep/DetType.h"
#include "DD4hep/Printout.h"
#include "DD4hep/DetectorTools.h"
#include "DDRec/MaterialManager.h"
#include "DDRec/CellIDPositionConverter.h"
#include "Math/Vector3D.h"


#include "DDRec/MaterialScan.h"

// #include "TGeoVolume.h"
// #include "TGeoManager.h"
// #include "TGeoNode.h"
#include "TFile.h"
#include "TH1F.h"

#include <fmt/core.h>
#include <fstream>
#include <regex>
#include "clipp.h"
using namespace clipp;
enum class mode { none, help, list, line, scan, rad };// Todo , maybe change rad to something else

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
  int                           nbins      = 30;
  int                           phi0       = M_PI / 2.;
  bool                          list_all   = false;
  mode                          selected   = mode::list;
  int                           color      = 1;
  double                        alpha      = 1;
  bool                          all_detectors = true;
  std::vector<std::string>      subsystem_names = {};
  std::vector<int>              ids;
  std::map<std::string, int>    part_name_levels;
  std::map<std::string, int>    part_name_colors;
  std::map<std::string, double> part_name_alphas;
  std::string                   scan_var = "eta";
  std::string                   line_var = "eta";
  double                        line_val = 0;
  double                        eta      = 0;
  double                        theta    = 0;
  std::array<double, 2>         eta_limits = {-4, 4};
  std::array<double, 2>         r_limits = {0, 150};
  bool print_only = false;
};
//______________________________________________________________________________

auto getMeanRadiationLength(std::string infile){
  // Material Stats and the line of interest for eta = 0.3
  // |     \   Material      Atomic                       Radiation   Interaction               Path   Integrated  Integrated    Material
  // | Num. \  Name          Number/Z   Mass/A  Density    Length       Length    Thickness   Length      X0        Lambda      Endpoint  
  // | Layer \                        [g/mole]  [g/cm3]     [cm]        [cm]          [cm]      [cm]     [cm]        [cm]     (     cm,     cm,     cm)
  // | 0    Average Material    53     130.887  6.4748     1.1163     28.5924       41.096     41.10   36.815629   1.437312     ( 77.53, 120.75, 43.70)
  std::ifstream file(infile);
  std::string instr;
  while(getline(file, instr)){
    if (instr.find("Average")!=std::string::npos){
      break;
    }
  }
  // Lines have a variable number of spaces between the elements, this reduces them all to 1 space
  for (int i = 10; i > 1; i--){
    std::string emptyStr(i, ' ');
    instr = std::regex_replace(instr, std::regex(emptyStr), " ");
  }
  
  // Finds the Radiation length
  std::stringstream ss(instr);
  int count = 0;
  while(getline(ss, instr, ' ') && count < 8){
    count++;
  }
  return instr;
}

using namespace dd4hep::rec;

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
                 .first_column(8)                    // column where usage lines and documentation starts
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

  auto mp = make_man_page(cli, argv0, fmt);
  //mp.prepend_section("DESCRIPTION", "Geometry tool for converting compact files to STEP (cad) files.");
  //mp.append_section("EXAMPLES", " $ npdet_to_teve list compact.xml");
  std::cout << mp << "\n";
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[]) {
  settings s;

  auto scanOpt = "Scan mode does multiple material lines, typically sweeping out "
                 "a range in eta or theta. This mode is used to produce plots or "
                 "look at many different parts at once." %
                     command("scan").set(s.selected, mode::scan) &
                 value("variable", s.scan_var) % "variable to scan (either eta or theta)";
  auto lineOpt = "line mode does a single extraction along a ray "
                 "defined by supplied variable value. " %
                     command("line").set(s.selected, mode::line) &
                 value("variable", s.line_var) % "(either eta or theta)" &
                 value("val", s.line_val) % "value of the variable defining the traced material line";
  auto radOpt = "gathers mean radiation length using line modelike methods "
                 "defined by supplied variable value. " %
                     command("rad").set(s.selected, mode::rad) &
                 value("variable", s.line_var) % "(either eta or theta)" &
                 value("val", s.line_val) % "value of the variable defining the traced material line";
  auto lastOpt = (
       repeatable(option("-d", "--subsystem") & values("detector",s.subsystem_names).set(s.all_detectors,false) % "detector subsystem to include in the budget"),
       option("-h", "--help").set(s.selected, mode::help) % "show help",
       option("-n", "--n-bins") & integer("nbins", s.nbins) % "number of bins",
       option("-o", "--output") & value("mat_budget.root", s.outfile) % "output root file"
       );

  auto compactArg =  required("-c","--compact") & value("file", s.infile).if_missing([] {
         std::cout << "You need to provide an input xml filename as the last argument!\n";
       }) % "compact detector description xml file";

  auto helpOpt = command("help").set(s.selected, mode::help) % "print help";
  auto cli     = ((helpOpt | scanOpt | lineOpt | radOpt | lastOpt), compactArg);

  std::vector<std::string> wrong;
  assert(cli.flags_are_prefix_free());
  // if(parse(argc, argv, cli) && wrong.empty()) {
  //  std::cout << "OK\n";
  //    /* ... */
  //} else {
  //    for(const auto& arg : wrong) std::cout << "'" << arg << "' is not a valid argument\n";
  //    std::cout << "Usage:" << usage_lines(cli,argv[0]) << '\n';
  //}
  auto result = parse(argc, argv, cli);
  if(s.help){
    std::cout << make_man_page(cli, argv[0]);
    std::exit(0);
  }

  //// parse debugging:
  //auto doc_label = [](const parameter& p) {
  //  if (!p.flags().empty())
  //    return p.flags().front();
  //  if (!p.label().empty())
  //    return p.label();
  //  return doc_string{"<?>"};
  //};

  //std::cout << "args -> parameter mapping:\n";
  //for (const auto& m : result) {
  //  std::cout << "#" << m.index() << " " << m.arg() << " -> ";
  //  auto p = m.param();
  //  if (p) {
  //    std::cout << doc_label(*p) << " \t";
  //    if (m.repeat() > 0) {
  //      std::cout << (m.bad_repeat() ? "[bad repeat " : "[repeat ") << m.repeat() << "]";
  //    }
  //    if (m.blocked())
  //      std::cout << " [blocked]";
  //    if (m.conflict())
  //      std::cout << " [conflict]";
  //    std::cout << '\n';
  //  } else {
  //    std::cout << " [unmapped]\n";
  //  }
  //}
  //std::cout << "missing parameters:\n";
  //for (const auto& m : result.missing()) {
  //  auto p = m.param();
  //  if (p) {
  //    std::cout << doc_label(*p) << " \t";
  //    std::cout << " [missing after " << m.after_index() << "]\n";
  //  }
  //}
  //std::cout << bool(result) << "\n";

  if (!result) {
    s.success = false;
    fmt::print("Parse fail\n");
    std::cout << make_man_page(cli, argv[0])
                     .prepend_section("DESCRIPTION", " Tool for quickly looking at magnetic fields.")
                     .append_section("EXAMPLES", "   npdet_info search Tracker  compact.xml\n");
    return s;
  }

  if (s.selected == mode::help) {
    std::cout << make_man_page(cli, argv[0])
                     .prepend_section("DESCRIPTION", " Tool for quickly looking at magnetic fields.")
                     .append_section("EXAMPLES", "   npdet_info search Tracker  compact.xml\n");
    exit(0);
  }

  s.success = true;

  return s;
}
//______________________________________________________________________________

int main(int argc, char* argv[]) {
  settings s = cmdline_settings(argc, argv);
  if (!s.success) {
    return 1;
  }
  dd4hep::setPrintLevel(dd4hep::ERROR);
  gErrorIgnoreLevel = kError;// kPrint, kInfo, kWarning,

  //  fmt::print("Scanning detector\n");
  //  fmt::print("including subsystems:\n");
  //  for (const auto& det_name : s.subsystem_names ) {
  //    fmt::print(" - {}\n",det_name);
  //  }
  //}

  dd4hep::Detector& description = dd4hep::Detector::getInstance();
  description.fromXML(s.infile);
  //CellIDPositionConverter  pos_converter(description);

  std::vector<std::string> good_subsystem_names;
  if(! s.all_detectors ) {
    for(const auto& subsys : s.subsystem_names){
      auto regex = std::regex(subsys);
      bool found_1 = false;
      for(const auto& d : description.detectors()){
        bool detector_matches = std::regex_search(d.first, regex);
        if (detector_matches) {
          //fmt::print("{} and {} match\n", d.first, subsys);
          good_subsystem_names.push_back(d.first);
          found_1 = true;
        }
      }
      if (!found_1) {
        fmt::print("No matches for subsystem {} found!\n", subsys);
      }
    }
  }

  TFile* rootFile = new TFile(s.outfile.c_str(), "recreate");

  dd4hep::Volume world = description.world().volume();
  MaterialManager matMgr(world);

  using ROOT::Math::XYZVector;
  using ROOT::Math::Polar3DVector;

  Vector3D starting_point(0, 0, 0);
  // Line Mode
  if (s.selected == mode::line) {
    double        eta   = s.line_val; // assuming only eta for now
    double        theta = 2.0 * std::atan(std::exp(-1.0 * eta));
    Polar3DVector direction_step(s.r_limits.at(1), theta, s.phi0);

    Vector3D p0 = starting_point;
    Vector3D p1 = starting_point + Vector3D(direction_step.x(), direction_step.y(), direction_step.z());
    MaterialScan ms(description);

    // const PlacementVec& placements = matMgr.placementsBetween(p0,p1);
    // const MaterialVec& mats = matMgr.materialsBetween(p0,p1);
    if (good_subsystem_names.size() > 0) {
      for (const auto& aname : good_subsystem_names) {
        //fmt::print(" Subsystem: {}\n", aname);
        ms.setDetector(aname.c_str());
        // const MaterialVec& mats = ms.scan(p0.x(),p0.y(),p0.z(),p1.x(),p1.y(),p1.z());
        ms.print(p0.x(), p0.y(), p0.z(), p1.x(), p1.y(), p1.z());
      }
    } else {
      ms.print(p0.x(), p0.y(), p0.z(), p1.x(), p1.y(), p1.z());
    }
  }

  // Rad Mode
  // More or less a copy of line mode with some added functions
  if( s.selected ==  mode::rad ) {
    double eta   = s.line_val; // assuming only eta for now
    double theta = 2.0 * std::atan(std::exp(-1.0 * eta));
    Polar3DVector direction_step(s.r_limits.at(1), theta, s.phi0);

    Vector3D p0 = starting_point;
    Vector3D p1 = starting_point + Vector3D(direction_step.x(), direction_step.y(), direction_step.z());
    MaterialScan ms(description);

    // const PlacementVec& placements = matMgr.placementsBetween(p0,p1);
    // const MaterialVec& mats = matMgr.materialsBetween(p0,p1);
    std::string tempFile = "temp_material_scan_file.txt";
    if (!freopen(tempFile.c_str(), "w", stdout)) {
      std::exit(-1);
    }
    if (good_subsystem_names.size() > 0) {
      for (const auto& aname : good_subsystem_names) {
        //fmt::print(" Subsystem: {}\n", aname);
        ms.setDetector(aname.c_str());
        // const MaterialVec& mats = ms.scan(p0.x(),p0.y(),p0.z(),p1.x(),p1.y(),p1.z());
        ms.print(p0.x(), p0.y(), p0.z(), p1.x(), p1.y(), p1.z());
      }
    }
    if (!freopen("/dev/tty", "w", stdout)) {
      std::exit(-1);
    }
    fmt::print("Average Radiation Length {}, full results in file {}.\n", getMeanRadiationLength(tempFile), tempFile);
  }
  
  // Scan Mode
  if (s.selected == mode::scan) {
    double d_eta = (s.eta_limits.at(1) - s.eta_limits.at(0)) / (s.nbins - 1);

    for (int i = 0; i < s.nbins; ++i) {
      double        eta   = s.eta_limits.at(0) + i * d_eta;
      double        theta = 2.0 * std::atan(std::exp(-1.0 * eta));
      Polar3DVector direction_step(s.r_limits.at(1), theta, s.phi0);

      Vector3D p0 = starting_point;
      Vector3D p1 = starting_point + Vector3D(direction_step.x(), direction_step.y(), direction_step.z());

      // Material scan does all the heavy lifting.
      // https://dd4hep.web.cern.ch/dd4hep/reference/classdd4hep_1_1rec_1_1MaterialScan.html#a1b6e0d33d679f87c476e5beca6a33717
      MaterialScan ms(description);

      // const PlacementVec& placements = matMgr.placementsBetween(p0,p1);
      // const MaterialVec& mats = matMgr.materialsBetween(p0,p1);

      if (s.subsystem_names.size() > 0) {
        for (const auto& aname : s.subsystem_names) {
          fmt::print(" Subsystem: {}\n", aname);
          ms.setDetector(aname.c_str());
          // const MaterialVec& mats = ms.scan(p0.x(),p0.y(),p0.z(),p1.x(),p1.y(),p1.z());
          ms.print(p0.x(), p0.y(), p0.z(), p1.x(), p1.y(), p1.z());
        }
      }
      fmt::print(" {}  {} \n", eta, theta);
    }
  }

    //for(auto amat : mats ) {
    //  fmt::print("mats: {} {}\n",amat.second, amat.second , amat.first.toString());
    //}
      //auto matdat = matMgr.createAveragedMaterial(mats);
      //fmt::print(" rad len : {}\n", matdat.radiationLength());

      //fmt::print("{} placments \n",placements.size());
      //for(const auto& plmnt: placements)
      //{
      //  if( plmnt.first.data() == nullptr) {
      //    continue;
      //  }
      //  //fmt::print("placement: {}\n",plmnt.first.toString());
      //  fmt::print("ids:");
      //  for(const auto& id:plmnt.first.volIDs()){
      //    fmt::print("{}={}",id.first,id.second);
      //  }
      //  fmt::print("\n");
      //  //dd4hep::detail::tools::PlacementPath pp;
      //  //auto                                 de = pos_converter.findDetElement(plmnt.first.position());
      //  //// if (dd4hep::detail::tools::findChild(pv0,de.placement(),pp))
      //  //{ std::cout << "derp\n"; }
      //  //else {
      //  //  std::cout << "child not found\n";
      //  //}
      //  //// for (const auto& j : pv.volIDs()) {
      //  ////  std::cout << j.first << " -- " << j.second << "\n";
      //  ////}
      //  //// double length = plmnt.second;
      //}


    //for (auto& det : subdets) {
    //  Vector3D p0 = pointOnCylinder(theta, det.r0, det.z0, phi0); // double theta, double r, double z, double phi)
    //  Vector3D p1 = pointOnCylinder(theta, det.r1, det.z1, phi0); // double theta, double r, double z, double phi)

    //  const PlacementVec& placements = matMgr.placementsBetween(p0, p1);

    //  //std::cout << " testing \n";
    //  //for(auto id1 : s.ids) {
    //  //  std::cout << " checking for ID = " << id1 << "\n";
    //  //  auto pv0 = pos_converter.findContext(id1)->elementPlacement();
    //  //  for(const auto& plmnt: placements)
    //  //  {
    //  //    dd4hep::detail::tools::PlacementPath pp;
    //  //    auto de = pos_converter.findDetElement( plmnt.first.position() );
    //  //    //if (dd4hep::detail::tools::findChild(pv0,de.placement(),pp))
    //  //    {
    //  //    std::cout << "derp\n";
    //  //    } else {
    //  //    std::cout << "child not found\n";

    //  //    }
    //  //    //for (const auto& j : pv.volIDs()) {
    //  //    //  std::cout << j.first << " -- " << j.second << "\n";
    //  //    //}
    //  //    //double length = plmnt.second;
    //  //  }
    //  //}

    //  const MaterialVec& materials = matMgr.materialsBetween(p0, p1);

    //  double sum_x0(0.), sum_lambda(0.), path_length(0.);

    //  for (auto amat : materials) {
    //    TGeoMaterial* mat    = amat.first->GetMaterial();
    //    double        length = amat.second;
    //    double        nx0    = length / mat->GetRadLen();
    //    sum_x0 += nx0;
    //    double nLambda = length / mat->GetIntLen();
    //    sum_lambda += nLambda;
    //    path_length += length;
    //  }

    //  det.hx->Fill(-theta / M_PI * 180., sum_x0);
    //  det.hl->Fill(-theta / M_PI * 180., sum_lambda);

    //  std::cout << std::scientific << sum_x0 << "  " << sum_lambda << "  "; // << path_length ;
    //}

  rootFile->Write();
  rootFile->Close();

  return 0;
}

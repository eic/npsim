#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/VectorUtil.h"

#include <algorithm>
#include <iterator>
#include <tuple>
#include <vector>
#include <regex>

#include "DD4hep/Detector.h"
#include "DD4hep/Printout.h"
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
#include <fmt/core.h>
#include "DD4hep/OpticalSurfaces.h"
#include "DD4hep/detail/OpticalSurfaceManagerInterna.h"
using namespace clipp;
using namespace ROOT::Math;
using namespace dd4hep;

enum class Mode { Print, List, Search, None };

struct settings {
  Mode                     mode         = Mode::None;
  bool                     success      = false;
  std::string              infile  = "";
  std::string              variable  = "";
  std::vector<std::string> constants  = {};
  std::string              format       = "stdout";
  std::string              search_str   = "";
  bool                     all          = false;
  bool                     units        = true;
  bool                     search       = false;
  bool                     header       = false;
  bool                     verbose      = false;
  bool                     surfaces     = false;
  bool                     help         = false;
  bool                     val_only     = false;
  bool                     value        = false;
  bool                     material     = false;
};

settings cmdline_settings(int argc, char* argv[]) {
  settings s;

  auto helpMode = "help mode:" % (
    command("help")
    );

  auto printMode =
      "Print mode" % (command("print").set(s.mode,Mode::Print) & value("variable",s.variable),
                      option("--value-only").set(s.val_only) % "print only the variable value"
                     );
                      //option("--units").set(s.units) % "print units too (default: mm, rad )",
                      //option("-m", "--mat", "--material").set(s.material) % "print materials",
                      //option("--all").set(s.all) % "print all constants");

  auto listMode =
      "List mode" % (command("list").set(s.mode,Mode::List) & values("variables",s.constants),
                      option("--units").set(s.units) % "print units too (default: mm, rad )",
                      option("-m", "--mat", "--material").set(s.material) % "print materials",
                      option("--all").set(s.all) % "print all constants");


  auto searchMode =
      "Search mode" % (command("search").set(s.mode,Mode::Search) & value("variable",s.search_str),
                       option("-m", "--mat", "--material").set(s.material) % "search materials",
                       option("--value").set(s.value) % "print associated value too"
                       );

  auto firstOpt = "user interface options:" % (option("-v", "--verbose").set(s.verbose) % "show detailed output",
                                               option("--header").set(s.header) % "print detector header information",
                                               option("-h", "--help").set(s.help) % "show help");

  auto lastOpt = "Compact detector description XML file." %  value("compact_xml", s.infile);
  //.if_missing([] {
  //  std::cout << "You need to provide an input xml filename as the last argument!\n";
  //}) % "compact description should probly have a \"fields\" sect4ion with more than one magnetic "
  //     "field defined.";

  std::vector<std::string> wrong; 
  auto cli = (
    firstOpt,
    printMode | listMode | searchMode,// (| printMode | command("list")),
    lastOpt
    );
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
  dd4hep::setPrintLevel(dd4hep::ERROR);
  gErrorIgnoreLevel = kError;// kPrint, kInfo, kWarning,


  // -------------------------
  // Get the DD4hep instance
  // Load the compact XML file
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  if (s.mode == Mode::Search) {
    //std::cout << "regex string = " << s.search_str << "\n";
    const std::regex txt_regex(s.search_str);
    auto       constants = detector.constants();
    for (const auto& c : constants) {
      //fmt::print("{} {}\n", c.first,std::regex_search(c.first, txt_regex));
      if (std::regex_search(c.first, txt_regex)) {
        fmt::print("{}", c.first);
        if(s.value) {
          fmt::print(" = {}", detector.constantAsDouble(c.first));
        }
        fmt::print("\n");
      }
    }
  }

  //for(const auto& c: s.constants) {
  //  std::cout << c << " = " << detector.constantAsDouble(c) << "\n";
  //}
  
  if (s.mode == Mode::Print) {
    fmt::print("{}\n",detector.constantAsDouble(s.variable));
  }

  //auto constants = detector.constants();
  //for(const auto& c : constants){
  //  std::cout << c.first << "," << detector.constant(c.first).dataType() << "\n";
  //  std::cout << c.first << "," << detector.constantAsString(c.first) << "\n";
  //  std::cout << c.first << "," << detector.constantAsDouble(c.first) << "\n";
  //}
  if(s.header){
    fmt::print("title: {}\n",detector.header().title());
    fmt::print("name: {}\n",detector.header().name());
    fmt::print("version: {}\n",detector.header().version());
    fmt::print("comment: {}\n",detector.header().comment());
    fmt::print("url: {}\n",detector.header().url());
    fmt::print("author: {}\n",detector.header().author());
  }

  auto surfman = detector.surfaceManager();
  for(const auto& surf: surfman->opticalSurfaces){
    fmt::print("surface: {}\n",surf.first);
  }

  for(const auto& surf: surfman->borderSurfaces){
    fmt::print("surface: {}\n",surf.first.second);
  }
  for(const auto& surf: surfman->skinSurfaces){
    fmt::print("surface: {}\n",surf.first.second);
  }
  return 0;

}

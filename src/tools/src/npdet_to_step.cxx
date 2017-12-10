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
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

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
#include "TGeoNode.h"
#include "TGeoVolume.h"

#include "Math/DisplacementVector3D.h"

#include <iostream>
#include <string>

#include "clipp.h"
using namespace clipp;
//______________________________________________________________________________

enum class mode { none, help, list, part };

struct settings {
  bool help           = false;
  bool success        = false;
  std::string infile  = "";
  std::string outfile = "detector_geometry";
  std::string p_name  = "";
  int  p_level        = -1;
  std::map<std::string,int> part_name_levels;
  bool level_set      = false; 
  int  geo_level        = -1;
  bool list_all = false;

  mode selected = mode::part;

};
//______________________________________________________________________________

template<typename T>
void print_usage(T cli, const char* argv0 ){
  //used default formatting
  std::cout << "Usage:\n" << usage_lines(cli, argv0)
            << "\nOptions:\n" << documentation(cli) << '\n';
}
//______________________________________________________________________________

template<typename T>
void print_man_page(T cli, const char* argv0 ){
  //all formatting options (with their default values)
  auto fmt = clipp::doc_formatting{}
  .start_column(8)                           //column where usage lines and documentation starts
  .doc_column(20)                            //parameter docstring start col
  .indent_size(4)                            //indent of documentation lines for children of a documented group
  .line_spacing(0)                           //number of empty lines after single documentation lines
  .paragraph_spacing(1)                      //number of empty lines before and after paragraphs
  .flag_separator(", ")                      //between flags of the same parameter
  .param_separator(" ")                      //between parameters 
  .group_separator(" ")                      //between groups (in usage)
  .alternative_param_separator("|")          //between alternative flags 
  .alternative_group_separator(" | ")        //between alternative groups 
  .surround_group("(", ")")                  //surround groups with these 
  .surround_alternatives("(", ")")           //surround group of alternatives with these
  .surround_alternative_flags("", "")        //surround alternative flags with these
  .surround_joinable("(", ")")               //surround group of joinable flags with these
  .surround_optional("[", "]")               //surround optional parameters with these
  .surround_repeat("", "...");                //surround repeatable parameters with these
  //.surround_value("<", ">")                  //surround values with these
  //.empty_label("")                           //used if parameter has no flags and no label
  //.max_alternative_flags_in_usage(1)         //max. # of flags per parameter in usage
  //.max_alternative_flags_in_doc(2)           //max. # of flags per parameter in detailed documentation
  //.split_alternatives(true)                  //split usage into several lines for large alternatives
  //.alternatives_min_split_size(3)            //min. # of parameters for separate usage line
  //.merge_alternative_flags_with_common_prefix(false)  //-ab(cdxy|xy) instead of -abcdxy|-abxy
  //.merge_joinable_flags_with_common_prefix(true);    //-abc instead of -a -b -c

  auto mp = make_man_page(cli, argv0, fmt);
  mp.prepend_section("DESCRIPTION", "Geometry tool for converting compact files to STEP (cad) files.");
  mp.append_section("EXAMPLES", " $ npdet_to_step list compact.xml");
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[])
{
  settings s;

  auto listMode = "list mode:" % ( 
    command("list").set(s.selected,mode::list) |
    command("info").set(s.selected,mode::list)   % "list detectors and print info about geometry ",
    option("-v","--verbose")
    );

  auto partMode = "part mode:" % repeatable(
    command("part").set(s.selected,mode::part) % "Select only the first level nodes by name", 
    repeatable(
      option("-l","--level").set(s.level_set) & value("level",s.p_level) % "Maximum level navigated to for part",
      value("name")([&](const std::string& p)
                    {
                      s.p_name = p;
                      if(!s.level_set) { s.p_level = -1; }
                      s.part_name_levels[p] = s.p_level;
                      s.level_set = false;
                    })                                                     % "Part/Node name (must be child of top node)"
      )
    );

  auto lastOpt = " options:" % (
    option("-h", "--help").set(s.selected, mode::help)      % "show help",
    option("-g","--global_level") & integer("level",s.geo_level),
    option("-o","--output") & value("out",s.outfile),
    value("file",s.infile).if_missing([]{ std::cout << "You need to provide an input xml filename as the last argument!\n"; } )
    % "input xml file"
    );

  auto cli = (
    command("help").set(s.selected, mode::help) | (partMode | listMode  , lastOpt)
    );

  assert( cli.flags_are_prefix_free() );

  auto res = parse(argc, argv, cli);

  if( res.any_error() ) {
    s.success = false;
    std::cout << make_man_page(cli, argv[0]).prepend_section("error: ",
                                                             "    The best thing since sliced bread.");
    return s;
  }

  s.success = true;

  if(s.selected ==  mode::help) {
    print_man_page<decltype(cli)>(cli,argv[0]);
  };
  return s;
}
//______________________________________________________________________________


int main (int argc, char *argv[]) {

  settings s = cmdline_settings(argc,argv);
  if( !s.success ) {
    return 1;
  }

  if(s.selected ==  mode::help) {
    return 0;
  }

  // ------------------------
  // CLI Checks 
  if( !fs::exists(fs::path(s.infile))  ) {
    std::cerr << "file, " << s.infile << ", does not exist\n";
    return 1;
  }
  auto  has_suffix = [&](const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
    str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  if( !has_suffix(s.outfile,".stp") ) {
    s.outfile += ".stp";
  }
  for(const auto& [part_name, part_level]  : s.part_name_levels ) {
    std::cout << " SOME Part : " << part_name  << ", level = " << part_level <<"\n";
  }

  // -------------------------
  // Get the DD4hep instance
  // Load the compact XML file
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  detector.manager().GetTopVolume()->GetNodes()->Print();
  //detector.manager().GetTopVolume()->GetNode(0)->Dump();
  if(s.selected ==  mode::list) {
    return 0;
  }
  
  TGeoToStep * mygeom= new TGeoToStep( &(detector.manager()) );
  if( s.part_name_levels.size() > 1 ) {
    mygeom->CreatePartialGeometry( s.part_name_levels, s.outfile.c_str() );
  } else if( s.part_name_levels.size() == 1 ) {
    // loop of 1
    for(const auto& [n,l] : s.part_name_levels){
      mygeom->CreatePartialGeometry( n.c_str(), l, s.outfile.c_str() );
    }
  } else {
    mygeom->CreateGeometry(s.outfile.c_str(), s.geo_level);
  }
  //detector.manager().Export("geometry.gdml");
  return 0;
} 
//______________________________________________________________________________



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

// DD4hep
// -----
// In .rootlogon.C
//  gSystem->Load("libDDDetectors");
//  gSystem->Load("libDDG4IO");
//  gInterpreter->AddIncludePath("/opt/software/local/include");
#include "DD4hep/Detector.h"
#include "DD4hep/Printout.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/SurfaceManager.h"
#include "DDRec/Surface.h"
#include "DD4hep/DD4hepUnits.h"

#include "TApplication.h"
#include "TMultiGraph.h"
#include "TGraph.h"

#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TEveManager.h"
#include "TEveGeoNode.h"
#include "TEveGeoShapeExtract.h"
#include "TEveGeoShape.h"
#include "TEveBrowser.h"
#include "TSystem.h"
#include "ROOT/TFuture.hxx"

#include "Math/DisplacementVector3D.h"

#include <iostream>
#include <string>
#include <chrono>

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
  int     part_level  = -1;
  bool    level_set      = false; 
  int     geo_level      = -1;
  bool    list_all       = false;
  mode    selected       = mode::list;
  int     color          = 1;
  double  alpha          = 1;
  std::map<std::string,int> part_name_levels;
  std::map<std::string,int> part_name_colors;
  std::map<std::string,double> part_name_alphas;
};
//______________________________________________________________________________

void run_list_mode(const settings& s);
void run_part_mode(const settings& s);
//______________________________________________________________________________

template<typename T>
void print_usage(T cli, const char* argv0 )
{
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
  mp.append_section("EXAMPLES", " $ npdet_to_teve list compact.xml");
  std::cout << mp << "\n";
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[])
{
  settings s;
  auto     listMode =
      "list mode:" %
      repeatable(command("list").set(s.selected, mode::list) % "list detectors and print info about geometry ",
                 repeatable(option("-l", "--level").set(s.level_set) & value("level", s.part_level) &
                            value("name")([&](const std::string& p) {
                              s.p_name = p;
                              if (!s.level_set) {
                                s.part_level = -1;
                              }
                              s.part_name_levels[p] = s.part_level;
                              s.level_set           = false;
                            }) % "Part/Node name (must be child of top node)"));

  auto partMode = "part mode:" %
                  repeatable(command("part").set(s.selected, mode::part) % "Select only the first level nodes by name",
                             repeatable(required("-l", "--level").set(s.level_set) &
                                            integer("level", s.part_level) % "Maximum level navigated to for part",
                                        option("-c", "--color") & integer("color", s.color),
                                        option("-a", "--alpha") & number("alpha", s.alpha),
                                        value("name")([&](const std::string& p) {
                                          s.p_name = p;
                                          if (!s.level_set) {
                                            s.part_level = -1;
                                          }
                                          s.level_set = false;
                                          std::cout << "s.color " << s.color << "\n";
                                          std::cout << "s.alpha " << s.alpha << "\n";
                                          s.part_name_levels[p] = s.part_level;
                                          s.part_name_colors[p] = s.color;
                                          s.part_name_alphas[p] = s.alpha;
                                        }) % "Part/Node name (must be child of top node)"));

  auto lastOpt = " options:" % (
    option("-h", "--help").set(s.selected, mode::help)      % "show help",
    option("-g","--global_level") & integer("level",s.geo_level),
    option("-o","--output") & value("out",s.outfile),
    value("file",s.infile).if_missing([]{ std::cout << "You need to provide an input xml filename as the last argument!\n"; } )
    % "input xml file"
    );

  std::string wrong;
  auto cli = (
    command("help").set(s.selected, mode::help) | (partMode | listMode  , lastOpt),
    any_other(wrong)
    );


  assert( cli.flags_are_prefix_free() );

  auto res = parse(argc, argv, cli);

  //std::cout << "wrong " << wrong << std::endl;

  //if( res.any_error() ) {
  //  s.success = false;
  //  std::cout << make_man_page(cli, argv[0]).prepend_section("error: ",
  //                                                           "    The best thing since sliced bread.");
  //  return s;
  //}
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
  if( !has_suffix(s.outfile,".root") ) {
    s.outfile += ".root";
  }
  //for(const auto& [part_name, part_level]  : s.part_name_levels ) {
  //  std::cout << " SOME Part : " << part_name  << ", level = " << part_level <<"\n";
  //}

  // ---------------------------------------------
  // Run modes 
  //
  switch(s.selected) {
    case mode::list:
      run_list_mode(s);
      break;
    case mode::part:
      run_part_mode(s);
      break;
    default:
      break;
  }

  return 0;
} 
//______________________________________________________________________________


void run_list_mode(const settings& s)
{
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;// kPrint, kInfo, kWarning,

  // -------------------------
  // Get the DD4hep instance
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  std::cout << gGeoManager->GetPath() << "\n";

  for(const auto& [p,l] : s.part_name_levels) {
    bool dir = gGeoManager->cd(p.c_str());
    if (!dir) {
      std::cerr << p << " not found!\n";
      continue;
    }
    TGeoNode *node = gGeoManager->GetCurrentNode();
    int level = gGeoManager->GetLevel();
    if( level > l ){
      std::cout << "\n" << p << " found at level " << level << " but above selected level of " << l << ")\n";
      continue;
    }
    std::cout << "\n";
    std::cout << "Subnodes for \"" << p << "\" (level = "  << level << ")\n";
    if(node->GetNdaughters() == 0) {
      continue;
    }
    TObjArrayIter node_array_iter(node->GetVolume()->GetNodes());
    TGeoNode* a_node = nullptr;
    while( (a_node = dynamic_cast<TGeoNode*>(node_array_iter.Next())) ) {
      std::cout << p << "/" << a_node->GetName() << "\n";
    }

    //TGeoNode* currentNode = nullptr;
    //auto res = new TEveGeoNode(node);
    //TGeoIterator nextNode( node->GetVolume() );
    //nextNode.SetType(1);
    //while( (currentNode = nextNode()) ) {
    //  auto nlevel = nextNode.GetLevel();
    //  if( nlevel > l-level ) {
    //   break;
    //  }
    //  //auto daughter = new TEveGeoNode( currentNode );
    //  //res->AddElement(daughter);
    //  currentNode->ls();
    //}
  }
}
//______________________________________________________________________________

void run_part_mode(const settings& s)
{
  int         root_argc = 0;
  char *root_argv[1] = {"npdet_to_teve"};
  //argv[0] = "npdet_fields";
  
  gErrorIgnoreLevel = kWarning;// kPrint, kInfo, kWarning,
  TApplication app("tapp", &root_argc, root_argv);

  // Get the DD4hep instance
  dd4hep::setPrintLevel(dd4hep::WARNING);
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  TEveManager::Create();//true,"");

  TEveGeoTopNode*  eve_top_n = new TEveGeoTopNode(&(detector.manager()),detector.manager().GetTopNode());
  detector.manager().GetTopNode()->SetVisibility(kFALSE);
  eve_top_n->SetVisLevel(0);
  std::cout << "Path is " << gGeoManager->GetPath() << "\n";

  for(const auto& [p,l] : s.part_name_levels) {
    bool dir = gGeoManager->cd(p.c_str());
    if (!dir) {
      std::cerr << p << " not found!\n";
      return;
    }
    TGeoNode *node = gGeoManager->GetCurrentNode();
    if(!node){
      std::cerr << " bad node\n";
      return;
    }
    int ilevel = gGeoManager->GetLevel();
    if( ilevel > l ){
      std::cout << p << " found at level " << ilevel << " but above selected level of " << l << "\n";
      //return;
    }

    int         istate = gGeoManager->GetCurrentNodeId();
    std::string ipath  = gGeoManager->GetPath();

    auto pcolor        = s.part_name_colors.find(p)->second;
    auto ptransparency = 100.0*(1.0 - s.part_name_alphas.find(p)->second);

    std::cout << " color         " << pcolor << "\n";
    std::cout << " ptransparency " << ptransparency << "\n";

    node->GetVolume()->SetLineColor(pcolor);
    node->GetVolume()->SetFillColor(pcolor);
    node->GetVolume()->SetTransparency(ptransparency);
    node->GetVolume()->SetTransparency(ptransparency);


    TEveGeoNode* res = new TEveGeoNode(node);
    std::map<int,TEveGeoNode*>  eve_nodes;
    eve_nodes[0] = res;

    TGeoIterator nextNode( node->GetVolume() );
    int path_index = 0;
    TGeoNode* currentNode = nullptr;

    while( (currentNode = nextNode()) ) {
      nextNode.SetType(0);
      // Not iterator level starts at 1 
      auto nlevel = nextNode.GetLevel();
      if( nlevel > l ) {
        continue;
      }
      //if( path_index == nlevel) {
      //  if(pcolor != s.part_name_colors.end() ){
          currentNode->GetVolume()->SetLineColor(pcolor);
          currentNode->GetVolume()->SetFillColor(pcolor);
          currentNode->GetVolume()->SetTransparency(ptransparency);
          currentNode->GetVolume()->SetTransparency(ptransparency);
      // }
        TEveGeoNode* daughter = new TEveGeoNode( currentNode );
        eve_nodes[nlevel] = daughter;
        eve_nodes[nlevel-1]->AddElement(daughter);
      //}
      //std::cout << nlevel << "\n";
      //std::cout << gGeoManager->PushPath() << "\n";

      //res->AddElement(daughter);
      currentNode->ls();
    }
    eve_top_n->AddElement(res);
  }

  //eve_top_n->SaveExtract(s.outfile.c_str(), "extract", kTRUE);

  //--------------------------------------------
  // Now Quit ... it is harder than it should be.
  app.Run(kFALSE);
  TEveManager::Terminate();
  app.Terminate(0);

   //std::cout << "derp1 \n";
  //ROOT::EnableImplicitMT(2);
  ////TEveManager::Terminate();
  //auto wi0 = ROOT::Experimental::Async([&](){app.Run(kFALSE); return std::string("done1");});
  //auto wi1 = ROOT::Experimental::Async([&]() {
  //  //using namespace std::chrono_literals;
  //  std::cout << "Hello waiter" << std::endl;
  //  gEve->GetBrowser()->CloseWindow();
  //  gSystem->ProcessEvents() ;
  //  std::chrono::seconds sec(2);
  //  std::this_thread::sleep_for(sec);
  //  std::cout << "Hello waiter" << std::endl;
  //  app.Terminate(0);
  //  std::cout << " " << wi0.get() << std::endl;
  //  return std::string("done 2");
  //});
  ////app.Run(kTRUE);
  //std::cout << " " << wi1.get() << std::endl;
  ////app.Terminate(0);
  //// Pass kFALSE if you want application to terminate by itself.
  //// Then you just need "return 0;" below (to avoid compiler warnings).
  //// Optionally shutdown eve here (not really needed):
  //// TEveManager::Terminate();
  //std::cout << "derp \n";
}


#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "THttpServer.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
namespace fs = std::filesystem;
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "DD4hep/DD4hepUnits.h"
#include "DD4hep/Detector.h"
#include "DD4hep/Printout.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/Surface.h"
#include "DDRec/SurfaceManager.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"


volatile sig_atomic_t sig_caught = 0;

void handle_sig(int signum) {
  /* in case we registered this handler for multiple signals */
  if (signum == SIGINT) {
    sig_caught = 1;
  }
  if (signum == SIGTERM) {
    sig_caught = 2;
  }
  if (signum == SIGABRT) {
    sig_caught = 3;
  }
}

#include "clipp.h"
using namespace clipp;
using std::string;
//______________________________________________________________________________

enum class mode { none, help, list, part };

struct settings {
  bool help           = false;
  bool success        = false;
  std::string compact_file  = "";
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
  bool    export_geometry = false;
  std::map<std::string,int> part_name_levels;
  std::map<std::string,int> part_name_colors;
  std::map<std::string,double> part_name_alphas;
  int    http_port          = 8090;
  string http_host          = "127.0.0.1";
  string in_out_file        = "";
};

void run_http_server(const settings& s);

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
  .first_column(8)                           //column where usage lines and documentation starts
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
  mp.prepend_section("DESCRIPTION", "Web display for compact geometry.");
  mp.append_section("EXAMPLES", " $ dd_web_display compact.xml");
  std::cout << mp << "\n";
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[])
{
  settings s;
  auto lastOpt = " options:" % (
    option("-h", "--help").set(s.selected, mode::help)      % "show help",
    option("-g","--global_level") & integer("level",s.geo_level),
    option("--export").set(s.export_geometry, true) % "Export geometry to rootfile.",
    option("-o","--output").set(s.export_geometry, true) & value("out",s.outfile) % "name of exported geometry (implies --export)",
    value("file",s.infile).if_missing([]{
      std::cout << "You need to provide an input xml filename as the last argument!\n";
    } ) % "input xml file"
    );

  auto server_cli =
  ((option("-p", "--port") & value("http_port", s.http_port)) %
   "port to which the http serve attaches. Default: 8090 ",
   (option("-H", "--host") & value("http_host", s.http_host)) %
   "Http server host name or IP address. Default: 127.0.0.1",
   (option("-f", "--file") & value("io_file", s.in_out_file)) %
   "File used to initialize and save plots. Default: top_folder.root");

  auto cli = (command("help").set(s.selected, mode::help) | (server_cli, lastOpt));

  assert( cli.flags_are_prefix_free() );

  auto res = parse(argc, argv, cli);

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

  run_http_server(s);

  return 0;
} 
//______________________________________________________________________________

void run_http_server(const settings& s) {
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;// kPrint, kInfo, kWarning,

  void (*prev_handler)(int);
  prev_handler = signal(SIGINT, handle_sig);

  // -------------------------
  // Get the DD4hep instance
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  if(s.export_geometry) {
    spdlog::info("running in batch mode to export geometry ");
    detector.manager().Export(s.outfile.c_str());
    std::exit(0);
  }


  auto serv = new THttpServer((std::string("http:") + s.http_host + ":" +
                                               std::to_string(s.http_port) +
                                               std::string("?top=geometry&thrds=1;rw;noglobal"))
                                                  .c_str()); 

  //spdlog::info("Creating display server at http://{}:{}",s.http_host,s.http_port);
  if( !(serv->IsAnyEngine()) ) {
    spdlog::error("Failed to start http server.");
    std::exit(-1);
  }
  //_server->SetDefaultPage("online.htm");
  //_server->SetDefaultPage("draw.htm");
  serv->SetCors();

  detector.manager().GetTopNode()->SetVisibility(kFALSE);
  gGeoManager->SetVisLevel(6);

  serv->Register("/",detector.manager().GetTopNode());
  serv->Register("/geoManager",gGeoManager);

  // Loop until an interrupt (ctrl-c) is issued
  while (1) {
    serv->ProcessRequests();
    if (sig_caught) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  }

  signal(SIGINT, prev_handler);
}


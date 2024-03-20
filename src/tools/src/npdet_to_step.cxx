#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

#include <TError.h>
#include <TApplication.h>

#include <DD4hep/Detector.h>
#include <DD4hep/Printout.h>

#include "clipp.h"
using namespace clipp;

#include "settings.h"
#include "TGeoToStep.h"

void run_part_mode(const settings& s);
//______________________________________________________________________________

template<typename T>
void print_usage(T cli, const char* argv0 ){
  std::cout << "Usage:\n"     << usage_lines(cli, argv0)
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
  mp.prepend_section("DESCRIPTION", "Geometry tool for converting compact files to STEP (cad) files.");
  mp.append_section("EXAMPLES", " $ npdet_to_step list compact.xml");
  std::cout << mp << "\n";
}
//______________________________________________________________________________

settings cmdline_settings(int argc, char* argv[])
{
  settings s;

  //auto listMode = "list mode:" % ( 
  //  command("list").set(s.selected,mode::list)  % "list detectors and print info about geometry ",
  //  option("-v","--verbose")
  //  );
  auto listMode = "list mode:" % repeatable( 
    command("list").set(s.selected,mode::list) % "list detectors and print info about geometry ",
    repeatable(
      option("-l","--level").set(s.level_set)
      & value("level",s.part_level)
      & value("name")([&](const std::string& p)
                      {
                        s.part_name = p;
                        if(!s.level_set) { s.part_level = -1; }
                        s.part_name_levels[p] = s.part_level;
                        s.level_set = false;
                      })                                                     % "Part/Node name (must be child of top node)"
    )
    );

  auto partMode = "part mode:" % repeatable(
    command("part").set(s.selected,mode::part) % "Select only the first level nodes by name", 
    repeatable(
      option("-l","--level").set(s.level_set)
      & value("level",s.part_level) % "Maximum level navigated to for part"
      & value("name")([&](const std::string& p)
                    {
                      s.part_name = p;
                      if(!s.level_set) { s.part_level = -1; }
                      s.part_name_levels[p] = s.part_level;
                      s.level_set = false;
                    })                                                     % "Part/Node name (must be child of top node)"
      )
    );

  auto lastOpt = " options:" % (
    option("-h", "--help").set(s.selected, mode::help)      % "show help",
    option("-g","--global_level") & integer("level",s.global_level),
    option("-o","--output") & value("out",s.outfile),
    value("file",s.infile).if_missing([]{ std::cout << "You need to provide an input xml filename as the last argument!\n"; } )
    % "input xml file"
    );

  auto helpMode = command("help").set(s.selected, mode::help);

  auto cli = (
    helpMode | (partMode | listMode  , lastOpt)
    );

  assert( cli.flags_are_prefix_free() );
  auto res = parse(argc, argv, cli);

  //auto doc_label = [](const parameter& p) {
  //  if(!p.flags().empty()) return p.flags().front();
  //  if(!p.label().empty()) return p.label();
  //  return doc_string{"<?>"};
  //};
  //std::string wrong;

  //auto& os = std::cout;
  //std::cout << "args -> parameter mapping:\n";
  //for(const auto& m : res) {
  //  os << "#" << m.index() << " " << m.arg() << " -> ";
  //  auto p = m.param();
  //  if(p) {
  //    os << doc_label(*p) << " \t";
  //    if(m.repeat() > 0) {
  //      os << (m.bad_repeat() ? "[bad repeat " : "[repeat ")
  //      <<  m.repeat() << "]";
  //    }
  //    if(m.blocked())  os << " [blocked]";
  //    if(m.conflict()) os << " [conflict]";
  //    os << '\n';
  //  }
  //  else {
  //    os << " [unmapped]\n";
  //  }
  //}

  //std::cout << "missing parameters:\n";
  //for(const auto& m : res.missing()) {
  //  auto p = m.param();
  //  if(p) {
  //    os << doc_label(*p) << " \t";
  //    os << " [missing after " << m.after_index() << "]\n";
  //  }
  //}

  //std::cout << "wrong " << wrong << std::endl;
  //if(res.unmapped_args_count()) { std::cout << "a\n"; }
  //if(res.any_bad_repeat()) { std::cout << "b\n"; }
  //if(res.any_blocked())    { std::cout << "c\n"; }
  //if(res.any_conflict())   { std::cout << "d\n"; }

  if(s.selected ==  mode::help) {
    print_man_page<decltype(cli)>(cli,argv[0]);
    return s;
  }

  if( res.any_error() ) {
    s.success = false;
    std::cout << make_man_page(cli, argv[0]).prepend_section("error: ",
                                                             "    The best thing since sliced bread.");
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

  if(s.selected ==  mode::help) {
    return 0;
  }

  // --------------------------------------
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

  // Make things quite
  gErrorIgnoreLevel = kWarning;// kPrint, kInfo, kWarning,
  dd4hep::setPrintLevel(dd4hep::WARNING);

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



void run_part_mode(const settings& s)
{
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;

  // -------------------------
  // Get the DD4hep instance
  // Load the compact XML file
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  TGeoToStep * mygeom= new TGeoToStep( &(detector.manager()) );
  if( s.part_name_levels.size() > 1 ) {
    mygeom->CreatePartialGeometry( s.part_name_levels, s.outfile.c_str() );
  } else if( s.part_name_levels.size() == 1 ) {
    // loop of 1
    for(const auto& [n,l] : s.part_name_levels){
      mygeom->CreatePartialGeometry( n.c_str(), l, s.outfile.c_str() );
    }
  } else {
    mygeom->CreateGeometry(s.outfile.c_str(), s.global_level);
  }
}


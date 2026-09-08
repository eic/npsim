#include "npdet_to_step_cli.h"

#include <cassert>
#include <iostream>
#include <string>

#include "clipp.h"
using namespace clipp;

template<typename T>
void print_man_page(T cli, const char* argv0) {
  auto fmt = clipp::doc_formatting{}
                 .first_column(8)
                 .doc_column(20)
                 .indent_size(4)
                 .line_spacing(0)
                 .paragraph_spacing(1)
                 .flag_separator(", ")
                 .param_separator(" ")
                 .group_separator(" ")
                 .alternative_param_separator("|")
                 .alternative_group_separator(" | ")
                 .surround_group("(", ")")
                 .surround_alternatives("(", ")")
                 .surround_alternative_flags("", "")
                 .surround_joinable("(", ")")
                 .surround_optional("[", "]")
                 .surround_repeat("", "...");

  auto mp = make_man_page(cli, argv0, fmt);
  mp.prepend_section("DESCRIPTION", "Geometry tool for converting compact files to STEP (cad) files.");
  mp.append_section("EXAMPLES", " $ npdet_to_step list compact.xml");
  std::cout << mp << "\n";
}

settings cmdline_settings(int argc, char* argv[]) {
  settings s;

  auto listMode = "list mode:" % repeatable(
    command("list").set(s.selected, mode::list) % "list detectors and print info about geometry ",
    repeatable(option("-l", "--level").set(s.level_set) & value("level", s.part_level) &
               value("name")([&](const std::string& p) {
                 s.part_name = p;
                 if (!s.level_set) {
                   s.part_level = -1;
                 }
                 s.part_name_levels[p] = s.part_level;
                 s.level_set           = false;
               }) % "Part/Node name (must be child of top node)"));

  auto partMode = "part mode:" % repeatable(
    command("part").set(s.selected, mode::part) % "Select only the first level nodes by name",
    repeatable(option("-l", "--level").set(s.level_set) & value("level", s.part_level) % "Maximum level navigated to for part",
               value("name")([&](const std::string& p) {
                 s.part_name = p;
                 if (!s.level_set) {
                   s.part_level = -1;
                 }
                 s.part_name_levels[p] = s.part_level;
                 s.level_set           = false;
               }) % "Part/Node name (must be child of top node)"));

  auto lastOpt = " options:" % (
    option("-h", "--help").set(s.selected, mode::help) % "show help",
    option("-g", "--global_level") & integer("level", s.global_level),
    option("-o", "--output") & value("out", s.outfile),
    value("file", s.infile).if_missing(
      [] { std::cout << "You need to provide an input xml filename as the last argument!\n"; }) %
      "input xml file",
    option("-u", "--unit-factor") & value("unit", s.tgeo_length_unit_in_mm) % "conversion factor of the length unit to mm");

  auto helpMode = command("help").set(s.selected, mode::help);

  auto cli = (helpMode | (partMode | listMode, lastOpt));

  assert(cli.flags_are_prefix_free());
  auto res = parse(argc, argv, cli);

  if (s.selected == mode::help) {
    s.success = true;
    print_man_page<decltype(cli)>(cli, argv[0]);
    return s;
  }

  if (res.any_error()) {
    s.success = false;
    std::cout << make_man_page(cli, argv[0]).prepend_section("error: ", "    The best thing since sliced bread.");
    return s;
  }

  s.success = true;

  return s;
}

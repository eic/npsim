#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

#include <TError.h>

#include <DD4hep/Detector.h>
#include <DD4hep/Printout.h>

#include "TGeoToStep.h"
#include "npdet_to_step_cli.h"
#include "settings.h"

void run_part_mode(const settings& s);
//______________________________________________________________________________

int main(int argc, char* argv[]) {
  settings s = cmdline_settings(argc, argv);
  if (!s.success) {
    return 1;
  }

  if (s.selected == mode::help) {
    return 0;
  }

  if (!fs::exists(fs::path(s.infile))) {
    std::cerr << "file, " << s.infile << ", does not exist\n";
    return 1;
  }
  s.outfile = ensure_step_extension(s.outfile);

  gErrorIgnoreLevel = kWarning;
  dd4hep::setPrintLevel(dd4hep::WARNING);

  switch (s.selected) {
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

void run_part_mode(const settings& s) {
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;

  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  TGeoToStep* mygeom = new TGeoToStep(&(detector.manager()));
  if (s.part_name_levels.size() > 1) {
    mygeom->CreatePartialGeometry(s.part_name_levels, s.outfile.c_str(), s.tgeo_length_unit_in_mm);
  } else if (s.part_name_levels.size() == 1) {
    for (const auto& [n, l] : s.part_name_levels) {
      mygeom->CreatePartialGeometry(n.c_str(), l, s.outfile.c_str(), s.tgeo_length_unit_in_mm);
    }
  } else {
    mygeom->CreateGeometry(s.outfile.c_str(), s.global_level, s.tgeo_length_unit_in_mm);
  }
}

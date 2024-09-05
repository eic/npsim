#ifndef NPDET_TOOLS_SETTINGS_H
#define NPDET_TOOLS_SETTINGS_H

#include <map>
#include <string>

class TGeoNode;

using std::string;

enum class mode { none, help, list, part };

struct settings {
  using string   = std::string;
  using part_map = std::map<string,int>;
  bool            help              = false;
  bool            success           = false;
  string          infile            = "";
  string          outfile           = "detector_geometry";
  string          part_name         = "";
  int             part_level        = -1;
  mode            selected          = mode::list;
  bool            level_set         = false;
  int             global_level      = 1;
  bool            list_all          = false;
  int             color             = 1;
  double          alpha             = 1;
  double          tgeo_length_unit_in_mm  = 10;
  std::map<std::string, int>    part_name_levels;
  std::map<std::string, int>    part_name_colors;
  std::map<std::string, double> part_name_alphas;
};

void print_daughter_nodes(TGeoNode* node, int print_depth) ;
void run_list_mode(const settings& s);

#endif

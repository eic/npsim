// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2025 EIC Collaboration
//
// npdet_to_dot: Generate Graphviz DOT files from a DD4hep/TGeo geometry.
//
// Each unique logical volume (TGeoVolume*) becomes one DOT node, colored by
// how many times it is instantiated in the traversed subtree:
//   gray        - placed exactly once
//   lemonchiffon - placed 2-9 times
//   lightgreen  - placed 10-99 times
//   skyblue     - placed 100+ times
//
// Edges connect parent to child logical volume and are labelled with the
// number of child instances per parent instance (the logical multiplicity).

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>

#include <DD4hep/Detector.h>
#include <DD4hep/Printout.h>

#include "clipp.h"
using namespace clipp;

// ---------------------------------------------------------------------------
// Minimal settings for this tool
// ---------------------------------------------------------------------------

enum class dot_mode { none, help, list, part };

struct dot_settings {
  bool     success        = false;
  dot_mode selected       = dot_mode::list;
  std::string infile      = "";
  std::string outfile     = "geometry";
  int         default_level = 2;
  bool        level_set   = false;
  int         part_level  = 2;
  std::map<std::string, int> part_name_levels;
};

// ---------------------------------------------------------------------------
// DOT helpers
// ---------------------------------------------------------------------------

// Unique node ID derived from pointer address
static std::string dot_id(const TGeoVolume* vol) {
  std::ostringstream ss;
  ss << "v" << reinterpret_cast<uintptr_t>(vol);
  return ss.str();
}

// Escape a volume name for use inside a DOT quoted string label
static std::string dot_escape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

// Node fill color based on total placement count in the traversed subtree
static std::string placement_color(int count) {
  if (count < 2)   return "#d3d3d3";  // lightgray
  if (count < 10)  return "#fffacd";  // lemonchiffon
  if (count < 100) return "#90ee90";  // lightgreen
  return "#87ceeb";                   // skyblue
}

// ---------------------------------------------------------------------------
// Core DOT writer
// ---------------------------------------------------------------------------

void write_dot(std::ostream& out, TGeoManager* geo,
               const std::map<std::string, int>& part_name_levels,
               int default_max_level, const std::string& title) {

  TGeoVolume* world      = geo->GetTopVolume();
  TGeoNode*   world_node = geo->GetTopNode();

  bool part_mode = !part_name_levels.empty();

  // Traversal state
  std::map<TGeoVolume*, int>                              vol_count;   // total placements in traversal
  std::map<std::pair<TGeoVolume*, TGeoVolume*>, int>      edge_total;  // total (parent,child) visits
  std::set<TGeoVolume*>                                   volumes;

  volumes.insert(world);
  vol_count[world] = 1;

  bool        found_in_level_1 = !part_mode;
  std::string matched_part;

  TGeoIterator it(world);
  TGeoNode*    currentNode = nullptr;
  it.SetType(0);

  while ((currentNode = it())) {
    it.SetType(0);
    int level = it.GetLevel();

    // Part-mode gate at level 1
    if (part_mode && level == 1) {
      found_in_level_1 = false;
      matched_part.clear();
      for (auto& [name, _] : part_name_levels) {
        if (std::string(currentNode->GetVolume()->GetName()) == name) {
          found_in_level_1 = true;
          matched_part     = name;
          break;
        }
      }
    }
    if (!found_in_level_1) { it.Skip(); continue; }

    int cur_max = part_mode ? part_name_levels.at(matched_part) : default_max_level;
    if (cur_max >= 0 && level > cur_max) { it.Skip(); continue; }

    TGeoVolume* child_vol  = currentNode->GetVolume();
    TGeoNode*   parent_node = (level == 1) ? world_node : it.GetNode(level - 1);
    TGeoVolume* parent_vol  = parent_node->GetVolume();

    volumes.insert(child_vol);
    volumes.insert(parent_vol);
    vol_count[child_vol]++;
    edge_total[{parent_vol, child_vol}]++;

    if (cur_max >= 0 && level == cur_max) it.Skip();
  }

  // Write DOT header
  out << "digraph geometry {\n"
      << "  rankdir=TB;\n"
      << "  label=\"" << dot_escape(title) << "\";\n"
      << "  labelloc=t;\n"
      << "  node [shape=box, style=filled, fontsize=10];\n\n";

  // Nodes (one per unique logical volume)
  for (const auto* vol : volumes) {
    int         count = vol_count.count(const_cast<TGeoVolume*>(vol))
                          ? vol_count.at(const_cast<TGeoVolume*>(vol)) : 1;
    std::string color = placement_color(count);
    std::string label = dot_escape(vol->GetName());
    if (count > 1) label += "\\n(" + std::to_string(count) + "x)";  // x for multiplicity
    out << "  " << dot_id(vol)
        << " [label=\"" << label << "\""
        << ", fillcolor=\"" << color << "\""
        << "];\n";
  }
  out << "\n";

  // Edges (one per unique logical parent→child pair)
  // Label shows children per parent instance (logical multiplicity)
  for (auto& [pair, total] : edge_total) {
    auto [parent, child] = pair;
    int per_parent = vol_count.count(parent) && vol_count.at(parent) > 0
                       ? (total + vol_count.at(parent) - 1) / vol_count.at(parent) : total;
    out << "  " << dot_id(parent) << " -> " << dot_id(child);
    if (per_parent > 1) out << " [label=\"" << per_parent << "x\"]";
    out << ";\n";
  }

  out << "}\n";
}

// ---------------------------------------------------------------------------
// CLI
// ---------------------------------------------------------------------------

template <typename T>
void print_usage(T cli, const char* argv0) {
  std::cout << "Usage:\n"     << usage_lines(cli, argv0)
            << "\nOptions:\n" << documentation(cli) << '\n';
}

dot_settings cmdline_settings(int argc, char* argv[]) {
  dot_settings s;

  auto listMode = "list mode:" % (
    command("list").set(s.selected, dot_mode::list)
      % "Show full geometry tree from world volume"
  );

  // "part" subcommand may be repeated for multiple subtrees
  auto partMode = "part mode:" % repeatable(
    command("part").set(s.selected, dot_mode::part)
      % "Show subtree(s) rooted at named volume(s)",
    repeatable(
      option("-l", "--level").set(s.level_set) & value("level", s.part_level)
        % "Maximum depth for this part",
      value("name")([&](const std::string& p) {
        if (!s.level_set) s.part_level = s.default_level;
        s.part_name_levels[p] = s.part_level;
        s.level_set = false;
      }) % "Volume name (must be child of world)"
    )
  );

  auto lastOpt = "options:" % (
    option("-h", "--help").set(s.selected, dot_mode::help) % "show help",
    option("-d", "--depth") & integer("depth", s.default_level)
      % "Maximum depth for full-tree mode (default: 2)",
    option("-o", "--output") & value("out", s.outfile)
      % "Output file base name (default: geometry)",
    value("file", s.infile).if_missing([] {
      std::cerr << "Error: input XML file required\n";
    }) % "DD4hep compact XML file"
  );

  auto helpMode = command("help").set(s.selected, dot_mode::help);

  auto cli = (helpMode | (partMode | listMode, lastOpt));

  assert(cli.flags_are_prefix_free());
  auto res = parse(argc, argv, cli);

  if (s.selected == dot_mode::help) {
    print_usage(cli, argv[0]);
    return s;
  }
  if (res.any_error()) {
    s.success = false;
    print_usage(cli, argv[0]);
    return s;
  }
  s.success = true;
  return s;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  dot_settings s = cmdline_settings(argc, argv);
  if (!s.success) return 1;
  if (s.selected == dot_mode::help) return 0;

  if (!fs::exists(s.infile)) {
    std::cerr << "File not found: " << s.infile << "\n";
    return 1;
  }

  // Suppress DD4hep/ROOT noise
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;

  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  TGeoManager* geo = gGeoManager;
  if (!geo || !geo->GetTopVolume()) {
    std::cerr << "Failed to load geometry from " << s.infile << "\n";
    return 1;
  }

  std::string dotfile = s.outfile + ".dot";
  std::ofstream out(dotfile);
  if (!out) {
    std::cerr << "Cannot open output file: " << dotfile << "\n";
    return 1;
  }

  std::string title = fs::path(s.infile).stem().string();

  if (s.selected == dot_mode::part) {
    write_dot(out, geo, s.part_name_levels, -1, title);
  } else {
    // list mode: use default_level
    write_dot(out, geo, {}, s.default_level, title);
  }

  out.close();
  std::cout << "DOT file written: " << dotfile << "\n";
  return 0;
}

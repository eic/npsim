//==========================================================================
//  AIDA Detector description implementation
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
//==========================================================================
#ifndef NPDET_OPTICALTRACKERCOMBINEDFILTER_H
#define NPDET_OPTICALTRACKERCOMBINEDFILTER_H

/// Framework include files
#include <DDG4/Geant4SensDetAction.h>

#include <G4OpticalPhoton.hh>
#include <G4Step.hh>
#include <G4VPhysicalVolume.hh>
#include <G4LogicalVolume.hh>

#include <nlohmann/json.hpp>

#include <optional>
#include <regex>
#include <string>
#include <vector>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    /**
     * \addtogroup Geant4SDActionPlugin
     *
     * @{
     * \package OpticalTrackerCombinedFilter
     *
     * \brief Particle filter companion to OpticalTrackerCombinedAction.
     *
     *  Uses the same \c VolumeActions JSON property as OpticalTrackerCombinedAction
     *  to determine how to filter each step:
     *  - Volumes with \c "Action": "Geant4OpticalTrackerAction": accept optical photons only.
     *  - All other matched volumes: accept all particles.
     *  - Unmatched volumes: accept all particles (pass-through).
     *
     * \param string VolumeActions
     *   JSON object \c {"volume_regex": {"Action": "ddg4_action_name", ...}, ...}
     *   in the same format as OpticalTrackerCombinedAction.
     *
     * @}
     */
    class OpticalTrackerCombinedFilter : public Geant4Filter {

      struct VolumeEntry {
        enum class FilterMode { AcceptOpticalOnly, AcceptAll };

        std::string               pattern;
        FilterMode                mode { FilterMode::AcceptAll };
        std::optional<std::regex> compiled_regex;

        static VolumeEntry fromJson(const std::string& volume, const nlohmann::json& cfg) {
          VolumeEntry entry;
          entry.pattern = volume;
          if (cfg.at("Action").get<std::string>() == "Geant4OpticalTrackerAction")
            entry.mode = FilterMode::AcceptOpticalOnly;
          if (!entry.pattern.empty()) {
            try {
              entry.compiled_regex.emplace(
                  entry.pattern,
                  std::regex_constants::ECMAScript | std::regex_constants::optimize);
            } catch (const std::regex_error& e) {
              printout(ERROR, "OpticalTrackerCombinedFilter",
                       "Invalid regex '%s': %s", entry.pattern.c_str(), e.what());
            }
          }
          return entry;
        }

        bool matches(const std::string& lv_name) const {
          return compiled_regex && std::regex_search(lv_name, *compiled_regex);
        }
      };

    public:
      OpticalTrackerCombinedFilter(Geant4Context* c, const std::string& n)
          : Geant4Filter(c, n) {
        declareProperty("VolumeActions", m_volume_actions_json);
      }

      virtual ~OpticalTrackerCombinedFilter() = default;

      void ensureEntries() const {
        if (m_entries_parsed) return;
        m_entries.clear();
        if (!m_volume_actions_json.empty()) {
          try {
            auto j = nlohmann::json::parse(m_volume_actions_json);
            for (const auto& [vol, cfg] : j.items())
              m_entries.push_back(VolumeEntry::fromJson(vol, cfg));
          } catch (const nlohmann::json::exception& e) {
            printout(ERROR, "OpticalTrackerCombinedFilter",
                     "Failed to parse VolumeActions JSON: %s", e.what());
          }
        }
        m_entries_parsed = true;
      }

      virtual bool operator()(const G4Step* step) const override {
        ensureEntries();
        const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
        if (!pv) return true;
        const std::string lv_name = pv->GetLogicalVolume()->GetName();

        for (const auto& entry : m_entries) {
          if (!entry.matches(lv_name)) continue;
          if (entry.mode == VolumeEntry::FilterMode::AcceptOpticalOnly)
            return step->GetTrack()->GetDefinition() ==
                   G4OpticalPhoton::OpticalPhotonDefinition();
          return true;
        }
        return true;
      }

      virtual bool operator()(const Geant4FastSimSpot* /*spot*/) const override {
        return true;
      }

    private:
      std::string                      m_volume_actions_json;
      mutable std::vector<VolumeEntry> m_entries;
      mutable bool                     m_entries_parsed { false };
    };

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_OPTICALTRACKERCOMBINEDFILTER_H

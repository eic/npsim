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

#include <optional>
#include <regex>
#include <sstream>
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
     *  Uses the same \c VolumeActions property as OpticalTrackerCombinedAction
     *  to determine how to filter each step:
     *  - Volumes mapped to \c Geant4OpticalTrackerAction: accept optical photons only.
     *  - All other matched volumes: accept all particles.
     *  - Unmatched volumes: accept all particles (pass-through).
     *
     * \param vector<string> VolumeActions
     *   Same format as OpticalTrackerCombinedAction:
     *   \c "volume_regex:ActionName[:key=value]*"
     *
     * @}
     */
    class OpticalTrackerCombinedFilter : public Geant4Filter {

      /// One routing entry parsed from a VolumeActions string.
      struct VolumeEntry {
        enum class FilterMode { AcceptOpticalOnly, AcceptAll };

        std::string              pattern;
        FilterMode               mode { FilterMode::AcceptAll };
        std::optional<std::regex> compiled_regex;

        static VolumeEntry parse(const std::string& spec) {
          VolumeEntry entry;
          std::istringstream ss(spec);
          std::string token;
          int field = 0;
          while (std::getline(ss, token, ':')) {
            if (field == 0) entry.pattern = token;
            else if (field == 1) {
              if (token == "Geant4OpticalTrackerAction")
                entry.mode = FilterMode::AcceptOpticalOnly;
              // else: AcceptAll for TrackerWeighted and anything else
              break;
            }
            ++field;
          }
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
        declareProperty("VolumeActions", m_volume_actions_raw);
      }

      virtual ~OpticalTrackerCombinedFilter() = default;

      /// Parse VolumeActions on first use (properties are set after construction).
      void ensureEntries() const {
        if (!m_entries_parsed) {
          m_entries.clear();
          for (const auto& spec : m_volume_actions_raw)
            m_entries.push_back(VolumeEntry::parse(spec));
          m_entries_parsed = true;
        }
      }

      /// Filter action. Return true if the step should be processed.
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
          return true; // AcceptAll
        }
        return true; // unmatched volume
      }

      /// GFLASH/FastSim interface (not used; accept all)
      virtual bool operator()(const Geant4FastSimSpot* /*spot*/) const override {
        return true;
      }

    private:
      std::vector<std::string>          m_volume_actions_raw;
      mutable std::vector<VolumeEntry>  m_entries;
      mutable bool                      m_entries_parsed { false };
    };

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_OPTICALTRACKERCOMBINEDFILTER_H

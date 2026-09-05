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
#ifndef NPDET_VOLUMEDISPATCHFILTER_H
#define NPDET_VOLUMEDISPATCHFILTER_H

/// Framework include files
#include <DD4hep/Plugins.h>
#include <DDG4/Geant4Action.h>
#include <DDG4/Geant4SensDetAction.h>
#include <DDG4/Geant4FastSimSpot.h>

#include <G4Step.hh>
#include <G4VPhysicalVolume.hh>
#include <G4LogicalVolume.hh>

#include <nlohmann/json.hpp>

#include <memory>
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
     * \package VolumeDispatchFilter
     *
     * \brief Particle filter companion to VolumeDispatchAction.
     *
     *  Maps logical-volume regexes to existing DDG4/DDSim filter plugins via
     *  the \c Properties JSON property.  Each entry specifies the filter
     *  type/name using DDG4's \c TypeName convention and any additional
     *  filter-specific properties.  Volumes absent from the object are not
     *  filtered (accept all).
     *
     * \param string Properties
     *   JSON object \c {"volume_regex": ["TypeName/Instance", {"key": "val", ...}], ...}
     *
     *   Each value is a JSON array whose first element is the DDG4 TypeName
     *   (type/instance split on the first \c /) and whose optional second
     *   element is a parameter dict.
     *
     *  Example:
     *  \code
     *    Properties = json.dumps({
     *      "mcp_vol": ("ParticleSelectFilter/OpticalPhotonSelector",
     *                  {"particle": "opticalphoton"})
     *    })
     *  \endcode
     *
     * @}
     */
    class VolumeDispatchFilter : public Geant4Filter {

      struct VolumeEntry {
        std::string               pattern;
        Geant4Filter*             filter { nullptr };
        std::optional<std::regex> compiled_regex;

        bool matches(const std::string& lv_name) const {
          return compiled_regex && std::regex_search(lv_name, *compiled_regex);
        }
      };

    public:
      VolumeDispatchFilter(Geant4Context* c, const std::string& n)
          : Geant4Filter(c, n) {
        declareProperty("Properties", m_properties_json);
      }

      virtual ~VolumeDispatchFilter() {
        for (auto& entry : m_entries)
          if (entry.filter) entry.filter->release();
      }

      void ensureEntries() const {
        if (m_entries_parsed) return;
        m_entries.clear();
        if (!m_properties_json.empty()) {
          try {
            auto j = nlohmann::json::parse(m_properties_json);
            for (const auto& [vol, val] : j.items()) {
              VolumeEntry entry;
              entry.pattern = vol;
              if (!entry.pattern.empty()) {
                try {
                  entry.compiled_regex.emplace(
                      entry.pattern,
                      std::regex_constants::ECMAScript | std::regex_constants::optimize);
                } catch (const std::regex_error& e) {
                  printout(ERROR, name().c_str(),
                           "Invalid regex '%s': %s", entry.pattern.c_str(), e.what());
                }
              }
              // Value is a JSON array: ["TypeName/Instance", {"key": "val", ...}]
              // The second element (params dict) is optional.
              if (val.is_array() && !val.empty()) {
                const auto tn = TypeName::split(val[0].get<std::string>());
                auto* act = PluginService::Create<Geant4Action*>(
                    tn.first, const_cast<Geant4Context*>(context()), tn.second);
                if (!act) {
                  printout(ERROR, name().c_str(),
                           "Failed to create filter '%s' for volume '%s'",
                           tn.first.c_str(), vol.c_str());
                } else {
                  entry.filter = dynamic_cast<Geant4Filter*>(act);
                  if (!entry.filter) {
                    printout(ERROR, name().c_str(),
                             "Plugin '%s' is not a Geant4Filter", tn.first.c_str());
                    act->release();
                  } else {
                    // Apply optional params dict (second array element)
                    if (val.size() > 1 && val[1].is_object()) {
                      for (const auto& [key, pval] : val[1].items()) {
                        if (entry.filter->hasProperty(key)) {
                          entry.filter->property(key).str(pval.is_string()
                              ? pval.get<std::string>()
                              : pval.dump());
                        } else {
                          printout(WARNING, name().c_str(),
                                   "Filter '%s' has no property '%s'",
                                   tn.first.c_str(), key.c_str());
                        }
                      }
                    }
                  }
                }
              }
              m_entries.push_back(std::move(entry));
            }
          } catch (const nlohmann::json::exception& e) {
            printout(ERROR, name().c_str(),
                     "Failed to parse Properties JSON: %s", e.what());
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
          // No filter was created (missing "name" key): accept all
          if (!entry.filter) return true;
          return (*entry.filter)(step);
        }
        return true; // unmatched volume
      }

      virtual bool operator()(const Geant4FastSimSpot* spot) const override {
        ensureEntries();
        const G4VPhysicalVolume* pv = spot ? spot->volume() : nullptr;
        if (!pv) return true;
        const std::string lv_name = pv->GetLogicalVolume()->GetName();

        for (const auto& entry : m_entries) {
          if (!entry.matches(lv_name)) continue;
          if (!entry.filter) return true;
          return (*entry.filter)(spot);
        }
        return true;
      }

    private:
      std::string                      m_properties_json;
      mutable std::vector<VolumeEntry> m_entries;
      mutable bool                     m_entries_parsed { false };
    };

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_VOLUMEDISPATCHFILTER_H

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
#ifndef NPDET_VOLUMEROUTERACTION_H
#define NPDET_VOLUMEROUTERACTION_H

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
     * \package VolumeRouterAction
     *
     * \brief Sensitive detector action that routes steps to per-volume DDG4 SD
     *        action plugins, instantiated at run-time via PluginService::Create.
     *
     *  Any registered \c Geant4Sensitive plugin may be used as a per-volume
     *  action.  Hit collections are shared through the common
     *  \c Geant4SensDetActionSequence, so hits from all volumes land in the
     *  same readout.
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
     *      "mcp_vol": ("Geant4OpticalTrackerAction", {}),
     *      "bar_vol": ("Geant4TrackerWeightedAction",
     *                  {"CollectSingleDeposits": "false"}),
     *    })
     *  \endcode
     *
     * @}
     */
    class VolumeRouterAction : public Geant4Sensitive {

      struct VolumeEntry {
        std::string               pattern;
        Geant4Sensitive*          action { nullptr };
        std::optional<std::regex> compiled_regex;

        bool matches(const std::string& lv_name) const {
          return compiled_regex && std::regex_search(lv_name, *compiled_regex);
        }
      };

    public:
      VolumeRouterAction(Geant4Context* ctxt, const std::string& n,
                                   DetElement det, Detector& dsc)
          : Geant4Sensitive(ctxt, n, det, dsc) {
        declareProperty("Properties", m_properties_json);
      }

      virtual ~VolumeRouterAction() {
        for (auto& entry : m_entries)
          if (entry.action) entry.action->release();
      }

      /// Called during setup after setDetector(); create and initialise sub-actions here.
      virtual void defineCollections() override {
        if (m_entries_initialised) return;
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
                Geant4Sensitive* act = PluginService::Create<Geant4Sensitive*>(
                    tn.first, context(), tn.second, &m_detector, &m_detDesc);
                if (!act) {
                  printout(ERROR, name().c_str(),
                           "Failed to create SD action '%s' for volume '%s'",
                           tn.first.c_str(), vol.c_str());
                } else {
                  // Apply optional params dict (second array element)
                  if (val.size() > 1 && val[1].is_object()) {
                    for (const auto& [key, pval] : val[1].items()) {
                      if (act->hasProperty(key)) {
                        act->property(key).str(pval.is_string()
                            ? pval.get<std::string>()
                            : pval.dump());
                      } else {
                        printout(WARNING, name().c_str(),
                                 "Action '%s' has no property '%s'",
                                 tn.first.c_str(), key.c_str());
                      }
                    }
                  }
                  // Share the same Geant4ActionSD so collections are registered together
                  act->setDetector(&detector());
                  act->defineCollections();
                  entry.action = act;
                }
              }
              m_entries.push_back(std::move(entry));
            }
          } catch (const nlohmann::json::exception& e) {
            printout(ERROR, name().c_str(),
                     "Failed to parse Properties JSON: %s", e.what());
          }
        }
        m_entries_initialised = true;
      }

      virtual void begin(G4HCofThisEvent* hce) override {
        for (auto& entry : m_entries)
          if (entry.action) entry.action->begin(hce);
      }

      virtual void end(G4HCofThisEvent* hce) override {
        for (auto& entry : m_entries)
          if (entry.action) entry.action->end(hce);
      }

      virtual void clear(G4HCofThisEvent* hce) override {
        for (auto& entry : m_entries)
          if (entry.action) entry.action->clear(hce);
      }

      virtual bool process(const G4Step* step, G4TouchableHistory* history) override {
        const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
        if (!pv) return false;
        const std::string lv_name = pv->GetLogicalVolume()->GetName();

        for (auto& entry : m_entries) {
          if (!entry.matches(lv_name)) continue;
          if (!entry.action) return false;
          return entry.action->process(step, history);
        }
        return false; // unmatched volume
      }

      virtual bool processFastSim(const Geant4FastSimSpot* spot,
                                   G4TouchableHistory* history) override {
        const G4VPhysicalVolume* pv = spot ? spot->volume() : nullptr;
        if (!pv) return false;
        const std::string lv_name = pv->GetLogicalVolume()->GetName();

        for (auto& entry : m_entries) {
          if (!entry.matches(lv_name)) continue;
          if (!entry.action) return false;
          return entry.action->processFastSim(spot, history);
        }
        return false;
      }

    private:
      std::string              m_properties_json;
      std::vector<VolumeEntry> m_entries;
      bool                     m_entries_initialised { false };
    };

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_VOLUMEROUTERACTION_H

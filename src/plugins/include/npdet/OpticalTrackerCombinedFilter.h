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
#include <string>

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
     *  Routes filtering based on which logical volume the step occurs in:
     *  - Volumes matching \c OpticalVolume (regex): accept optical photons only.
     *  - Volumes matching \c TrackerVolume (regex): accept all particles.
     *  - All other volumes: accept all particles (pass-through).
     *
     * \param string OpticalVolume  Regex matching logical volume names for optical detection (default "mcp_vol")
     * \param string TrackerVolume  Regex matching logical volume names for charged-particle tracking (default "bar_vol")
     *
     * @}
     */
    class OpticalTrackerCombinedFilter : public Geant4Filter {
    public:
      OpticalTrackerCombinedFilter(Geant4Context* c, const std::string& n)
          : Geant4Filter(c, n) {
        declareProperty("OpticalVolume", m_optical_volume);
        declareProperty("TrackerVolume", m_tracker_volume);
      }

      virtual ~OpticalTrackerCombinedFilter() = default;

      /// Filter action. Return true if the step should be processed.
      virtual bool operator()(const G4Step* step) const override {
        const G4VPhysicalVolume* pv = step->GetPreStepPoint()->GetPhysicalVolume();
        if (!pv) return true;
        const std::string lv_name = pv->GetLogicalVolume()->GetName();

        // Optical-detector volumes: only accept optical photons
        update_regex_cache(m_optical_volume, m_cached_optical_volume, m_optical_regex);
        if (m_optical_regex && std::regex_search(lv_name, *m_optical_regex)) {
          return step->GetTrack()->GetDefinition() ==
                 G4OpticalPhoton::OpticalPhotonDefinition();
        }

        // Tracker volumes: accept everything
        update_regex_cache(m_tracker_volume, m_cached_tracker_volume, m_tracker_regex);
        if (m_tracker_regex && std::regex_search(lv_name, *m_tracker_regex)) {
          return true;
        }

        // Default: accept all
        return true;
      }

      /// GFLASH/FastSim interface (not used; accept all)
      virtual bool operator()(const Geant4FastSimSpot* /*spot*/) const override {
        return true;
      }

    private:
      static void update_regex_cache(const std::string&        expression,
                                     std::string&               cached,
                                     std::optional<std::regex>& compiled) {
        if (expression.empty()) {
          cached = expression;
          compiled.reset();
        } else if (expression != cached) {
          try {
            compiled.emplace(expression,
                             std::regex_constants::ECMAScript |
                             std::regex_constants::optimize);
            cached = expression;
          } catch (const std::regex_error& e) {
            cached = expression;
            compiled.reset();
            printout(ERROR, "OpticalTrackerCombinedFilter",
                     "Invalid regex '%s': %s", expression.c_str(), e.what());
          }
        }
      }

      std::string m_optical_volume { "mcp_vol" };
      std::string m_tracker_volume { "bar_vol" };

      mutable std::string               m_cached_optical_volume;
      mutable std::string               m_cached_tracker_volume;
      mutable std::optional<std::regex> m_optical_regex;
      mutable std::optional<std::regex> m_tracker_regex;
    };

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_OPTICALTRACKERCOMBINEDFILTER_H

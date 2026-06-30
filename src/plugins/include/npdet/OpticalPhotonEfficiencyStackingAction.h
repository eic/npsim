//==========================================================================
//  AIDA Detector description implementation 
//--------------------------------------------------------------------------
// Copyright (C) Organisation europeenne pour la Recherche nucleaire (CERN)
// All rights reserved.
//
// For the licensing terms see $DD4hepINSTALL/LICENSE.
// For the list of contributors see $DD4hepINSTALL/doc/CREDITS.
//
// Author     : M.Frank
//
//==========================================================================
#ifndef OPTICALPHOTONEFFICIENCYSTACKINGACTION_H
#define OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

#include "DDG4/Geant4Random.h"
#include "DDG4/Geant4StackingAction.h"
#include "G4LogicalVolume.hh"
#include "G4OpticalPhoton.hh"
#include "G4Region.hh"
#include "G4Track.hh"

#include <optional>
#include <regex>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {
        
  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    class OpticalPhotonEfficiencyStackingAction: public Geant4StackingAction {
    public:
      /// Standard constructor with initializing arguments
      OpticalPhotonEfficiencyStackingAction(Geant4Context* c, const std::string& n)
      : Geant4StackingAction(c, n) {
        declareProperty("LambdaMin", m_lambda_min);
        declareProperty("LambdaMax", m_lambda_max);
        declareProperty("Efficiency", m_efficiency);
        declareProperty("LogicalVolume", m_logical_volume);
        declareProperty("Region", m_region);
      };
      /// Default destructor
      virtual ~OpticalPhotonEfficiencyStackingAction() {
        printout(DEBUG, name(), "Suppressed %d of %d photons in lv regex %s or region regex %s",
          m_killed_photons, m_total_photons, m_logical_volume.c_str(), m_region.c_str());
        printout(DEBUG, name(), "lambda range: [%f,%f] nm",
          m_lambda_min / CLHEP::nm, m_lambda_max / CLHEP::nm);
        std::ostringstream oss_efficiency;
        std::copy(m_efficiency.begin(), m_efficiency.end(),
          std::ostream_iterator<double>(oss_efficiency, " "));
        std::string str_efficiency = oss_efficiency.str();
        printout(DEBUG, name(), "efficiency: %s", str_efficiency.c_str());
      };
      /// New-stage callback
      virtual void newStage(G4StackManager*) override { };
      /// Preparation callback
      virtual void prepare(G4StackManager*) override { };
      /// Return TrackClassification with enum G4ClassificationOfNewTrack or NoTrackClassification
      virtual TrackClassification classifyNewTrack(G4StackManager*, const G4Track* aTrack) override {
        // Only apply to optical photons
        if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
          auto* pv = aTrack->GetVolume();
          if (pv == nullptr) return TrackClassification();
          auto* lv = pv->GetLogicalVolume();
          auto* region = lv->GetRegion();
          const auto& volume_name = lv->GetName();
          const auto region_name = region == nullptr ? std::string{} : region->GetName();
          update_regex_cache(m_logical_volume, m_cached_logical_volume, m_logical_volume_regex);
          update_regex_cache(m_region, m_cached_region, m_region_regex);
          printout(VERBOSE, name(), "photon in pv %s lv %s",
            pv->GetName().c_str(), lv->GetName().c_str());
          // Apply to matching logical volume or region regex
          const bool volume_matches = m_logical_volume_regex && std::regex_search(volume_name, *m_logical_volume_regex);
          const bool region_matches = m_region_regex && std::regex_search(region_name, *m_region_regex);
          if (volume_matches || region_matches) {
            double mom = aTrack->GetMomentum().mag();
            double lambda = CLHEP::hbarc * CLHEP::twopi / mom;
            printout(VERBOSE, name(), "with mom = %f eV, lambda = %f nm",
              mom / CLHEP::eV, lambda / CLHEP::nm);

            m_total_photons++;
            if (m_lambda_min < lambda && lambda < m_lambda_max) {
              double efficiency{0.};
              if (m_efficiency.size() == 0) {
                // No efficiency specified, assume zero
                efficiency = 0.;
                // which means kill
                ++m_killed_photons;
                return TrackClassification(fKill);

              } else if (m_efficiency.size() == 1) {
                // Single constant value over lambda range
                efficiency = m_efficiency.front();

              } else {
                // Linear interpolation on lambda grid
                double lambda_step = (m_lambda_max - m_lambda_min) / (m_efficiency.size() - 1);
                double div = (lambda - m_lambda_min) / lambda_step;
                auto i = std::llround(std::floor(div));
                double t = div - i;
                double a = m_efficiency[i];
                double b = m_efficiency[i+1];
                efficiency = a + t * (b - a);
                printout(VERBOSE, name(), "a = %f, b = %f, t = %f", a, b, t);
                printout(VERBOSE, name(), "efficiency %f", efficiency);
              }

              // Edge cases
              if (efficiency == 0.0) {
                ++m_killed_photons;
                return TrackClassification(fKill);
              }
              if (efficiency == 1.0) return TrackClassification();

              // Throw random value
              Geant4Event&  evt = context()->event();
              Geant4Random& rnd = evt.random();
              double random = rnd.uniform();
              if (random > efficiency) {
                printout(VERBOSE, name(), "photon killed");
                ++m_killed_photons;
                return TrackClassification(fKill);
              }
            } else {
              printout(VERBOSE, name(), "outside lambda range [%f,%f] nm", m_lambda_min / CLHEP::nm, m_lambda_max / CLHEP::nm);
            }
          } else {
            printout(VERBOSE, name(), "no QE match for lv %s against %s or region %s against %s",
              volume_name.c_str(), m_logical_volume.c_str(), region_name.c_str(), m_region.c_str());
          }
        }
        return TrackClassification();
      };
    private:
      static void update_regex_cache(const std::string& expression, std::string& cached_expression,
                                     std::optional<std::regex>& regex) {
        if (expression.empty()) {
          cached_expression = expression;
          regex.reset();
        } else if (expression != cached_expression) {
          try {
            regex.emplace(expression, std::regex_constants::ECMAScript | std::regex_constants::optimize);
            cached_expression = expression;
          } catch (const std::regex_error& e) {
            cached_expression = expression;
            regex.reset();
            printout(ERROR, "OpticalPhotonEfficiencyStackingAction", "Invalid regex '%s': %s", expression.c_str(), e.what());
          }
        }
      }

      double m_lambda_min{0.}, m_lambda_max{0.};
      std::vector<double> m_efficiency;
      std::string m_logical_volume, m_region;
      std::string m_cached_logical_volume, m_cached_region;
      std::optional<std::regex> m_logical_volume_regex, m_region_regex;
      std::size_t m_total_photons{0}, m_killed_photons{0};
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

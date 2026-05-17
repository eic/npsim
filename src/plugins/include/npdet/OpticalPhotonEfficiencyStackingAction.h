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
#include "G4OpticalPhoton.hh"
#include "G4Track.hh"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <mutex>
#include <regex>
#include <stdexcept>

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
        declareProperty("LambdaValues", m_lambda_values);
        declareProperty("Efficiency", m_efficiency);
        declareProperty("LogicalVolume", m_logical_volume);
      };
      /// Default destructor
      virtual ~OpticalPhotonEfficiencyStackingAction() {
        printout(DEBUG, name(), "Suppressed %d of %d photons in lv %s",
          m_killed_photons, m_total_photons, m_logical_volume.c_str());
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
        std::call_once(m_init_once, [this]() { this->initialize(); });
        // Only apply to optical photons
        if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
          auto* pv = aTrack->GetVolume();
          if (pv == nullptr) return TrackClassification();
          auto* lv = pv->GetLogicalVolume();
          printout(VERBOSE, name(), "photon in pv %s lv %s",
            pv->GetName().c_str(), lv->GetName().c_str());
          // Only apply to specified logical volume regex
          if (std::regex_match(lv->GetName().c_str(), m_logical_volume_regex)) {
            double mom = aTrack->GetMomentum().mag();
            double lambda = CLHEP::hbarc * CLHEP::twopi / mom;
            printout(VERBOSE, name(), "with mom = %f eV, lambda = %f nm",
              mom / CLHEP::eV, lambda / CLHEP::nm);

            m_total_photons++;
            auto lambda_min = !m_interp_lambda_values.empty() ? m_interp_lambda_values.front() : m_lambda_min;
            auto lambda_max = !m_interp_lambda_values.empty() ? m_interp_lambda_values.back() : m_lambda_max;
            if (lambda_min < lambda && lambda < lambda_max) {
              double efficiency = interpolate(lambda);
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
              printout(VERBOSE, name(), "outside lambda range [%f,%f] nm", lambda_min / CLHEP::nm, lambda_max / CLHEP::nm);
            }
          } else {
            printout(VERBOSE, name(), "not in volume %s", m_logical_volume.c_str());
          }
        }
        return TrackClassification();
      };
    private:
      void initialize() {
        try {
          m_logical_volume_regex = std::regex(m_logical_volume);
        } catch (const std::regex_error&) {
          throw std::runtime_error("Invalid LogicalVolume regex");
        }
        if (m_efficiency.size() > 1) {
          if (m_lambda_values.size() == m_efficiency.size()) {
            m_interp_lambda_values = m_lambda_values;
          } else {
            auto lambda_min = m_lambda_min;
            auto lambda_max = m_lambda_max;
            m_interp_lambda_values.reserve(m_efficiency.size());
            auto lambda_step = (lambda_max - lambda_min) / (m_efficiency.size() - 1);
            for (std::size_t i = 0; i < m_efficiency.size(); ++i) {
              m_interp_lambda_values.push_back(lambda_min + i * lambda_step);
            }
          }
        } else if (m_lambda_values.size() > 1) {
          m_interp_lambda_values = m_lambda_values;
        }
      }
      inline double interpolate(double lambda) const {
        if (m_efficiency.size() == 1) {
          return m_efficiency.front();
        }
        if (m_efficiency.size() < 2 || m_interp_lambda_values.size() < 2) {
          return 0.0;
        }
        auto upper = std::upper_bound(m_interp_lambda_values.begin(), m_interp_lambda_values.end(), lambda);
        auto i = std::distance(m_interp_lambda_values.begin(), upper) - 1;
        double a_lambda = m_interp_lambda_values[i];
        double b_lambda = m_interp_lambda_values[i+1];
        double t = (lambda - a_lambda) / (b_lambda - a_lambda);
        double a = m_efficiency[i];
        double b = m_efficiency[i+1];
        printout(VERBOSE, name(), "a = %f, b = %f, t = %f", a, b, t);
#if defined(__cpp_lib_interpolate) && __cpp_lib_interpolate >= 201902L
        double efficiency = std::lerp(a, b, t);
#else
        double efficiency = a + t * (b - a);
#endif
        printout(VERBOSE, name(), "efficiency %f", efficiency);
        return efficiency;
      }
      double m_lambda_min{0.}, m_lambda_max{0.};
      std::vector<double> m_lambda_values;
      std::vector<double> m_interp_lambda_values;
      std::vector<double> m_efficiency;
      std::string m_logical_volume;
      std::regex m_logical_volume_regex{};
      std::once_flag m_init_once{};
      std::size_t m_total_photons{0}, m_killed_photons{0};
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

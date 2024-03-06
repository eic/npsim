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
        // Only apply to optical photons
        if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
          auto* pv = aTrack->GetVolume();
          auto* lv = pv->GetLogicalVolume();
          printout(VERBOSE, name(), "photon in pv %s lv %s",
            pv->GetName().c_str(), lv->GetName().c_str());
          // Only apply to specified logical volume
          if (lv->GetName() == m_logical_volume) {
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
            printout(VERBOSE, name(), "not in volume %s", m_logical_volume.c_str());
          }
        }
        return TrackClassification();
      };
    private:
      double m_lambda_min{0.}, m_lambda_max{0.};
      std::vector<double> m_efficiency;
      std::string m_logical_volume;
      std::size_t m_total_photons{0}, m_killed_photons{0};
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

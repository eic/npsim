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
        printout(DEBUG, name(), "lambda range: [%f,%f] nm", m_lambda_min / CLHEP::nm, m_lambda_max / CLHEP::nm);
        printout(DEBUG, name(), "logical volume: %s", m_logical_volume.c_str());
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
        if (aTrack->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {
          double lambda = hbarc * twopi / aTrack->GetMomentum().mag();
          if (m_lambda_min < lambda && lambda < m_lambda_max) {
            double lambda_step = (m_lambda_max - m_lambda_min) / (m_efficiency.size() - 1);
            double div = (lambda - m_lambda_min) / lambda_step;
            auto i = std::llround(std::floor(div));
            double t = div - m_lambda_min - i * lambda_step;
            double a = m_efficiency[i];
            double b = m_efficiency[i+1];
            double efficiency = a + t * (b - a);
            double random = m_rndm->uniform();
            if (random > efficiency) {
              return TrackClassification(fKill);
            }
          }
        }
        return TrackClassification();
      };
    private:
      Geant4Random* m_rndm = Geant4Random::instance();
      double m_lambda_min{0.}, m_lambda_max{0.};
      std::vector<double> m_efficiency;
      std::string m_logical_volume;
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

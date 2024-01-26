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
      : Geant4StackingAction(c, n) { };
      /// Default destructor
      virtual ~OpticalPhotonEfficiencyStackingAction() { };
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
      double m_lambda_min{200. * dd4hep::nm}, m_lambda_max{700. * dd4hep::nm};
      std::vector<double> m_efficiency{
        0,    0,    14.0, 14.8, 14.5, 14.9, 14.4, 14.2, 13.9, 14.6, 15.2, 15.7, 16.4, 16.9, 17.5,
        17.7, 18.1, 18.8, 19.3, 19.8, 20.6, 21.4, 22.4, 23.1, 23.6, 24.1, 24.2, 24.6, 24.8, 25.2,
        25.7, 26.5, 27.1, 28.2, 29.0, 29.9, 30.8, 31.1, 31.7, 31.8, 31.6, 31.5, 31.5, 31.3, 31.0,
        30.8, 30.8, 30.4, 30.2, 30.3, 30.2, 30.1, 30.1, 30.1, 29.8, 29.9, 29.8, 29.7, 29.7, 29.7,
        29.8, 29.8, 29.9, 29.9, 29.8, 29.9, 29.8, 29.9, 29.8, 29.7, 29.8, 29.7, 29.8, 29.6, 29.5,
        29.7, 29.7, 29.8, 30.1, 30.4, 31.0, 31.3, 31.5, 31.8, 31.8, 31.9, 32.0, 32.0, 32.0, 32.0,
        32.2, 32.2, 32.1, 31.8, 31.8, 31.8, 31.7, 31.6, 31.6, 31.7, 31.5, 31.5, 31.4, 31.3, 31.3,
        31.2, 30.8, 30.7, 30.5, 30.3, 29.9, 29.5, 29.3, 29.2, 28.6, 28.2, 27.9, 27.8, 27.3, 27.0,
        26.6, 26.1, 25.9, 25.5, 25.0, 24.6, 24.2, 23.8, 23.4, 23.0, 22.7, 22.4, 21.9, 21.4, 21.2,
        20.7, 20.3, 19.8, 19.6, 19.3, 18.9, 18.7, 18.3, 17.9, 17.8, 17.8, 16.7, 16.5, 16.4, 16.0,
        15.6, 15.6, 15.2, 14.9, 14.6, 14.4, 14.1, 13.8, 13.6, 13.3, 13.0, 12.8, 12.6, 12.3, 12.0,
        11.9, 11.7, 11.5, 11.2, 11.1, 10.9, 10.7, 10.4, 10.3, 9.9,  9.8,  9.6,  9.3,  9.1,  9.0,
        8.8,  8.5,  8.3,  8.3,  8.2,  7.9,  7.8,  7.7,  7.5,  7.3,  7.1,  6.9,  6.7,  6.6,  6.3,
        6.2,  6.0,  5.8,  5.7,  5.6,  5.4,  5.2,  5.1,  4.9,  4.8,  4.6,  4.5,  4.4,  4.2,  4.1,
        4.0,  3.8,  3.7,  3.5,  3.3,  3.2,  3.1,  3.0,  2.9,  2.5,  2.4,  2.4,  2.3,  2.3,  2.1,
        1.8,  1.6,  1.5,  1.5,  1.6,  1.8,  1.9,  1.4,  0.8,  0.9,  0.8,  0.7,  0.6,  0.3,  0.3,
        0.5,  0.3,  0.4,  0.3,  0.1,  0.2,  0.1,  0.2,  0.3,  0.0
      };
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // OPTICALPHOTONEFFICIENCYSTACKINGACTION_H

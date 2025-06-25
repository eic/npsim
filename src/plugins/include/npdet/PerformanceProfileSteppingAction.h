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
#ifndef PERFORMANCEPROFILESTEPPINGACTION_H
#define PERFORMANCEPROFILESTEPPINGACTION_H

#include "DDG4/Geant4SteppingAction.h"
#include "G4Step.hh"
#include "TFile.h"
#include "TH2F.h"

#include <chrono>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    using namespace std::chrono_literals;

    class PerformanceProfileSteppingAction : public Geant4SteppingAction {
     public:
      /// Standard constructor with initializing arguments
      PerformanceProfileSteppingAction(Geant4Context* c, const std::string& n) : Geant4SteppingAction(c, n) {};
      /// Default destructor
      virtual ~PerformanceProfileSteppingAction() {
        std::chrono::milliseconds total_duration = std::chrono::milliseconds::zero();
        for (auto& [track_id, duration] : m_duration) {
          if (std::chrono::abs(duration) > std::chrono::nanoseconds(10ms)) {
            auto p0 = m_prestep_position[track_id] / CLHEP::mm;
            auto p1 = m_poststep_position[track_id] / CLHEP::mm;
            auto E1 = m_pdg[track_id] == -22 ? m_poststep_energy[track_id] / CLHEP::eV
                                             : m_poststep_energy[track_id] / CLHEP::MeV;
            printout(INFO, name(), "track %d (%d): %ld ms (%f %f %f mm -> %f %f %f mm, %f (M?)eV)", track_id,
                     m_pdg[track_id], std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), p0.x(),
                     p0.y(), p0.z(), p1.x(), p1.y(), p1.z(), E1);
          }
          total_duration += std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        }
        printout(INFO, name(), "total duration: %ld ms", total_duration);
        TFile f("histos.root", "recreate");
        m_xy.Write();
        m_zr.Write();
        f.Close();
      };
      /// User stepping callback
      void operator()(const G4Step* step, G4SteppingManager*) override {
        std::chrono::nanoseconds step_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - m_previous_timepoint);

        // Get step info
        auto* track                      = step->GetTrack();
        auto* particle                   = track->GetParticleDefinition();
        auto  pdg                        = particle->GetPDGEncoding();
        auto  track_id                   = track->GetTrackID();
        auto* prestep [[maybe_unused]]   = step->GetPreStepPoint();
        auto  prestep_position           = prestep->GetPosition();
        auto* poststep [[maybe_unused]]  = step->GetPostStepPoint();
        auto  poststep_position          = poststep->GetPosition();
        auto  poststep_energy            = poststep->GetTotalEnergy();
        auto  firststep [[maybe_unused]] = step->IsFirstStepInVolume();
        auto  laststep [[maybe_unused]]  = step->IsLastStepInVolume();

        if (track_id == m_previous_track_id) {
          // Current track
          m_duration[track_id] += step_duration;
          m_poststep_position[track_id] = poststep_position;
          m_poststep_energy[track_id]   = poststep_energy;
          m_xy.Fill(poststep_position.x(), poststep_position.y(), step_duration.count());
          m_zr.Fill(poststep_position.z(), std::hypot(poststep_position.x(), poststep_position.y()), step_duration.count());
        } else {
          // New track
          if (track_id == 0) {
            m_duration.clear();
            m_pdg.clear();
            m_prestep_position.clear();
            m_poststep_position.clear();
            m_poststep_energy.clear();
            printout(INFO, name(), "new event");
          }
          m_previous_track_id          = track_id;
          m_pdg[track_id]              = pdg;
          m_duration[track_id]         = std::chrono::nanoseconds::zero();
          m_prestep_position[track_id] = prestep_position;
        }

        m_previous_timepoint = std::chrono::steady_clock::now();
      };

     private:
      std::map<G4int, std::chrono::nanoseconds>          m_duration;
      std::map<G4int, G4int>                             m_pdg;
      std::map<G4int, G4ThreeVector>                     m_prestep_position;
      std::map<G4int, G4ThreeVector>                     m_poststep_position;
      std::map<G4int, G4double>                          m_poststep_energy;
      G4int                                              m_previous_track_id{0};
      std::chrono::time_point<std::chrono::steady_clock> m_previous_timepoint;
      TH2F m_xy{"m_xy", "xy", 100, -3.*CLHEP::m, +3.*CLHEP::m, 100, -3.*CLHEP::m, +3.*CLHEP::m};
      TH2F m_zr{"m_zr", "zr", 100, -4.5*CLHEP::m, +5.5*CLHEP::m, 100, 0., +3.*CLHEP::m};
    };
  } // End namespace sim
} // End namespace dd4hep

#endif // PERFORMANCEPROFILESTEPPINGACTION_H

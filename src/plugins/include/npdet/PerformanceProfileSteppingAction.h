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
#include "TH2D.h"
#include "npdet/PerformanceProfileTrackingAction.h"

#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#include <unordered_map>

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
        printLongTracks();
        std::chrono::milliseconds total_duration = std::chrono::milliseconds::zero();
        for (auto& [pdg, ns] : m_pdg_duration) {
          total_duration += std::chrono::duration_cast<std::chrono::milliseconds>(ns);
        }
        // Print per-PDG summary: stepping | tracking | remainder (tracking - stepping)
        auto& tracking_dur = performanceProfileSharedData().tracking_duration;
        auto& track_count = performanceProfileSharedData().track_count;
        printout(WARNING, name(), "%-10s %15s %15s %15s %10s %15s", "PDG", "stepping_ms", "tracking_ms", "remainder_ms", "n_tracks", "rem_us/track");
        for (auto& [pdg, step_ns] : m_pdg_duration) {
          auto step_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_ns);
          auto tit = tracking_dur.find(pdg);
          auto track_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
              tit != tracking_dur.end() ? tit->second : std::chrono::nanoseconds::zero());
          auto remainder_ms = track_ms - step_ms;
          auto n_tracks = track_count.count(pdg) ? track_count.at(pdg) : 0;
          auto rem_us_per_track = n_tracks > 0 ? (remainder_ms.count() * 1000) / n_tracks : 0;
          printout(WARNING, name(), "%-10d %15ld %15ld %15ld %10ld %15ld", pdg, step_ms.count(), track_ms.count(), remainder_ms.count(), n_tracks, rem_us_per_track);
        }
        printout(WARNING, name(), "total duration: %ld ms", total_duration.count());
        std::time_t t = std::time(nullptr);
        char ts[32];
        std::tm tm_buf{};
        if (localtime_r(&t, &tm_buf) != nullptr) {
          std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_buf);
        } else {
          std::snprintf(ts, sizeof(ts), "%s", "19700101_000000");
        }
        auto suffix = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::string fname = std::string("histos_") + ts + "_" + std::to_string(suffix) + ".root";
        TFile f(fname.c_str(), "recreate");
        m_xy.Write();
        m_zr.Write();
        for (auto& [pdg, hxy] : m_xy_pdg) {
          hxy.Write();
        }
        for (auto& [pdg, hzr] : m_zr_pdg) {
          hzr.Write();
        }
        f.Close();
      };
      /// User stepping callback
      void operator()(const G4Step* step, G4SteppingManager*) override {
        auto now = std::chrono::steady_clock::now();
        std::chrono::nanoseconds step_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_previous_timepoint);

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
          m_pdg_duration[pdg] += step_duration;
          performanceProfileSharedData().stepping_duration_per_track[track_id] += step_duration;
          m_poststep_position[track_id] = poststep_position;
          m_poststep_energy[track_id]   = poststep_energy;
          m_xy.Fill(poststep_position.x(), poststep_position.y(), step_duration.count());
          m_zr.Fill(poststep_position.z(), std::hypot(poststep_position.x(), poststep_position.y()), step_duration.count());
          // Per-PDG histos: create on first encounter
          auto hname_xy = "m_xy_pdg" + std::to_string(pdg);
          auto hname_zr = "m_zr_pdg" + std::to_string(pdg);
          if (m_xy_pdg.find(pdg) == m_xy_pdg.end()) {
            m_xy_pdg.emplace(std::piecewise_construct, std::forward_as_tuple(pdg),
                             std::forward_as_tuple(hname_xy.c_str(), hname_xy.c_str(),
                                                   100, -3.*CLHEP::m, +3.*CLHEP::m,
                                                   100, -3.*CLHEP::m, +3.*CLHEP::m));
            m_zr_pdg.emplace(std::piecewise_construct, std::forward_as_tuple(pdg),
                             std::forward_as_tuple(hname_zr.c_str(), hname_zr.c_str(),
                                                   1000, -50.*CLHEP::m, +50.*CLHEP::m,
                                                   100, 0., +3.*CLHEP::m));
          }
          m_xy_pdg.at(pdg).Fill(poststep_position.x(), poststep_position.y(), step_duration.count());
          m_zr_pdg.at(pdg).Fill(poststep_position.z(), std::hypot(poststep_position.x(), poststep_position.y()), step_duration.count());
        } else {
          // New track — flush if event action signalled a new event
          if (performanceProfileSharedData().new_event_flag) {
            printLongTracks();
            m_duration.clear();
            m_pdg.clear();
            m_prestep_position.clear();
            m_poststep_position.clear();
            m_poststep_energy.clear();
            performanceProfileSharedData().new_event_flag = false;
            printout(VERBOSE, name(), "new event");
          }
          m_previous_track_id          = track_id;
          m_pdg[track_id]              = pdg;
          m_duration[track_id]         = std::chrono::nanoseconds::zero();
          m_prestep_position[track_id] = prestep_position;
        }

        m_previous_timepoint = now;
      };

     private:
      void printLongTracks() {
        for (auto& [track_id, duration] : m_duration) {
          if (std::chrono::abs(duration) <= std::chrono::nanoseconds(10ms)) {
            continue;
          }
          auto p0 = m_prestep_position[track_id] / CLHEP::mm;
          auto p1 = m_poststep_position[track_id] / CLHEP::mm;
          auto E1 = m_pdg[track_id] == -22 ? m_poststep_energy[track_id] / CLHEP::eV
                                           : m_poststep_energy[track_id] / CLHEP::MeV;
          printout(INFO, name(), "track %d (pdg=%d): %ld ms (%f %f %f mm -> %f %f %f mm, %f (M?)eV)", track_id,
                   m_pdg[track_id], std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), p0.x(),
                   p0.y(), p0.z(), p1.x(), p1.y(), p1.z(), E1);
        }
      }

      std::unordered_map<G4int, std::chrono::nanoseconds>          m_duration;
      std::unordered_map<G4int, G4int>                             m_pdg;
      std::unordered_map<G4int, G4ThreeVector>                     m_prestep_position;
      std::unordered_map<G4int, G4ThreeVector>                     m_poststep_position;
      std::unordered_map<G4int, G4double>                          m_poststep_energy;
      G4int                                                        m_previous_track_id{0};
      std::chrono::time_point<std::chrono::steady_clock>           m_previous_timepoint;
      TH2D m_xy{"m_xy", "xy", 100, -3.*CLHEP::m, +3.*CLHEP::m, 100, -3.*CLHEP::m, +3.*CLHEP::m};
      TH2D m_zr{"m_zr", "zr", 1000, -50.*CLHEP::m, +50.*CLHEP::m, 100, 0., +3.*CLHEP::m};
      std::unordered_map<G4int, TH2D>                          m_xy_pdg;
      std::unordered_map<G4int, TH2D>                          m_zr_pdg;
      std::unordered_map<G4int, std::chrono::nanoseconds>      m_pdg_duration;
    };
  } // End namespace sim
} // End namespace dd4hep

#endif // PERFORMANCEPROFILESTEPPINGACTION_H

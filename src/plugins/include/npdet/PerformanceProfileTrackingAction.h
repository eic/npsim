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
#ifndef PERFORMANCEPROFILETRACKINGACTION_H
#define PERFORMANCEPROFILETRACKINGACTION_H

#include "DDG4/Geant4EventAction.h"
#include "DDG4/Geant4TrackingAction.h"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "G4Track.hh"
#include "CLHEP/Units/SystemOfUnits.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2D.h"

#include <chrono>
#include <ctime>
#include <string>
#include <unordered_map>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    using namespace std::chrono_literals;

    /// Shared state between tracking and stepping actions (per-PDG wall-clock accumulators).
    /// Populated by PerformanceProfileTrackingAction, read by PerformanceProfileSteppingAction.
    struct PerformanceProfileSharedData {
      using time_point = std::chrono::time_point<std::chrono::steady_clock>;
      /// per-PDG total wall-clock time from begin(track) to end(track)
      std::unordered_map<G4int, std::chrono::nanoseconds> tracking_duration;
      /// per-PDG number of tracks seen by tracking action
      std::unordered_map<G4int, long> track_count;
      /// per-track stepping duration, written by stepping action, read+cleared by tracking action at end(track)
      std::unordered_map<G4int, std::chrono::nanoseconds> stepping_duration_per_track;
      /// time of first UserSteppingAction call for each track (written by stepping action)
      std::unordered_map<G4int, time_point> first_step_time;
      /// time of last UserSteppingAction call for each track (written by stepping action, updated each step)
      std::unordered_map<G4int, time_point> last_step_time;
      /// per-PDG accumulated begin overhead (PreUserTrackingAction → first step)
      std::unordered_map<G4int, std::chrono::nanoseconds> begin_overhead_duration;
      /// per-PDG accumulated end overhead (last step → PostUserTrackingAction)
      std::unordered_map<G4int, std::chrono::nanoseconds> end_overhead_duration;
      /// set true by event action at begin of each event; cleared by stepping action
      bool new_event_flag{false};
    };

    /// Global singleton accessor — single-threaded use only.
    inline PerformanceProfileSharedData& performanceProfileSharedData() {
      static PerformanceProfileSharedData instance;
      return instance;
    }

    class PerformanceProfileTrackingAction : public Geant4TrackingAction {
     public:
      PerformanceProfileTrackingAction(Geant4Context* c, const std::string& n)
          : Geant4TrackingAction(c, n) {}
      virtual ~PerformanceProfileTrackingAction() {
        std::time_t t = std::time(nullptr);
        char ts[32];
        std::tm tm_buf{};
        if (localtime_r(&t, &tm_buf) != nullptr) {
          std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_buf);
        } else {
          std::snprintf(ts, sizeof(ts), "unknown_time");
        }
        auto suffix = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::string fname = std::string("histos_overhead_") + ts + "_" + std::to_string(suffix) + ".root";
        TFile f(fname.c_str(), "recreate");
        m_overhead_xy.Write();
        m_overhead_zr.Write();
        m_overhead_dist.Write();
        m_begin_overhead_dist.Write();
        m_end_overhead_dist.Write();
        for (auto& [pdg, h] : m_overhead_dist_pdg) h.Write();
        for (auto& [pdg, h] : m_begin_overhead_dist_pdg) h.Write();
        for (auto& [pdg, h] : m_end_overhead_dist_pdg) h.Write();
        f.Close();
      }

      /// Pre-track callback: record wall-clock start and track start position
      void begin(const G4Track* track) override {
        auto track_id = track->GetTrackID();
        m_begin_time[track_id] = std::chrono::steady_clock::now();
        m_pdg[track_id]        = track->GetParticleDefinition()->GetPDGEncoding();
        m_begin_pos[track_id]  = track->GetPosition();
        performanceProfileSharedData().track_count[m_pdg[track_id]]++;
      }

      /// Post-track callback: accumulate wall-clock duration per PDG; fill overhead spatial histos
      void end(const G4Track* track) override {
        auto track_id = track->GetTrackID();
        auto it = m_begin_time.find(track_id);
        if (it == m_begin_time.end()) return;
        auto end_time = std::chrono::steady_clock::now();
        auto track_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - it->second);
        auto pdg = m_pdg[track_id];
        performanceProfileSharedData().tracking_duration[pdg] += track_duration;

        // Compute per-track overhead = tracking − stepping; fill spatial histo at track start position
        auto& shared = performanceProfileSharedData();
        auto& step_map = shared.stepping_duration_per_track;
        auto sit = step_map.find(track_id);
        auto step_ns = sit != step_map.end() ? sit->second : std::chrono::nanoseconds::zero();
        if (sit != step_map.end()) step_map.erase(sit);
        auto overhead_ns = track_duration > step_ns ? track_duration - step_ns
                                                     : std::chrono::nanoseconds::zero();
        auto pos = m_begin_pos[track_id];
        m_overhead_xy.Fill(pos.x(), pos.y(), overhead_ns.count());
        m_overhead_zr.Fill(pos.z(), std::hypot(pos.x(), pos.y()), overhead_ns.count());
        double overhead_us = overhead_ns.count() / 1000.0;
        m_overhead_dist.Fill(overhead_us);

        // Split overhead into pre-first-step (begin) and post-last-step (end) components
        auto fit = shared.first_step_time.find(track_id);
        auto lit = shared.last_step_time.find(track_id);
        if (fit != shared.first_step_time.end()) {
          auto begin_overhead_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
              fit->second - it->second);
          double begin_us = begin_overhead_ns.count() / 1000.0;
          if (begin_us >= 0.) {
            m_begin_overhead_dist.Fill(begin_us);
            auto bhname = "m_begin_overhead_dist_pdg" + std::to_string(pdg);
            m_begin_overhead_dist_pdg.try_emplace(pdg, bhname.c_str(),
                                                  (bhname + ";overhead [#mus];tracks").c_str(),
                                                  2000, 0., 200.);
            m_begin_overhead_dist_pdg.at(pdg).Fill(begin_us);
            shared.begin_overhead_duration[pdg] += begin_overhead_ns;
          }
          shared.first_step_time.erase(fit);
        }
        if (lit != shared.last_step_time.end()) {
          auto end_overhead_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
              end_time - lit->second);
          double end_us = end_overhead_ns.count() / 1000.0;
          if (end_us >= 0.) {
            m_end_overhead_dist.Fill(end_us);
            auto ehname = "m_end_overhead_dist_pdg" + std::to_string(pdg);
            m_end_overhead_dist_pdg.try_emplace(pdg, ehname.c_str(),
                                                (ehname + ";overhead [#mus];tracks").c_str(),
                                                2000, 0., 200.);
            m_end_overhead_dist_pdg.at(pdg).Fill(end_us);
            shared.end_overhead_duration[pdg] += end_overhead_ns;
          }
          shared.last_step_time.erase(lit);
        }
        // Per-PDG overhead distribution: create on first encounter
        auto hname = "m_overhead_dist_pdg" + std::to_string(pdg);
        m_overhead_dist_pdg.try_emplace(pdg, hname.c_str(),
                                        (hname + ";overhead [#mus];tracks").c_str(),
                                        2000, 0., 200.);
        m_overhead_dist_pdg.at(pdg).Fill(overhead_us);

        m_begin_time.erase(it);
        m_pdg.erase(track_id);
        m_begin_pos.erase(track_id);
      }

     private:
      std::unordered_map<G4int, std::chrono::time_point<std::chrono::steady_clock>> m_begin_time;
      std::unordered_map<G4int, G4int>                                               m_pdg;
      std::unordered_map<G4int, G4ThreeVector>                                       m_begin_pos;
      TH2D m_overhead_xy{"m_overhead_xy", "overhead_xy",
                         100, -3.*CLHEP::m, +3.*CLHEP::m,
                         100, -3.*CLHEP::m, +3.*CLHEP::m};
      TH2D m_overhead_zr{"m_overhead_zr", "overhead_zr",
                         1000, -50.*CLHEP::m, +50.*CLHEP::m,
                         100, 0., +3.*CLHEP::m};
      // Per-track overhead distribution: x = overhead in µs, 2000 bins from 0–200 µs
      TH1F m_overhead_dist{"m_overhead_dist", "per-track overhead (#mus);overhead [#mus];tracks",
                            2000, 0., 200.};
      // Pre-first-step overhead (begin): PreUserTrackingAction → first UserSteppingAction
      TH1F m_begin_overhead_dist{"m_begin_overhead_dist",
                                  "begin overhead (pre-first-step);overhead [#mus];tracks",
                                  2000, 0., 200.};
      // Post-last-step overhead (end): last UserSteppingAction → PostUserTrackingAction
      TH1F m_end_overhead_dist{"m_end_overhead_dist",
                                "end overhead (post-last-step);overhead [#mus];tracks",
                                2000, 0., 200.};
      std::unordered_map<G4int, TH1F> m_overhead_dist_pdg;
      std::unordered_map<G4int, TH1F> m_begin_overhead_dist_pdg;
      std::unordered_map<G4int, TH1F> m_end_overhead_dist_pdg;
    };

    class PerformanceProfileEventAction : public Geant4EventAction {
     public:
      PerformanceProfileEventAction(Geant4Context* c, const std::string& n)
          : Geant4EventAction(c, n) {}
      virtual ~PerformanceProfileEventAction() = default;

      /// Signal stepping action to flush and clear at start of new event
      void begin(const G4Event*) override {
        performanceProfileSharedData().new_event_flag = true;
      }
      void end(const G4Event*) override {}
    };

  } // End namespace sim
} // End namespace dd4hep

#endif // PERFORMANCEPROFILETRACKINGACTION_H

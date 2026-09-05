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
#ifndef NPDET_OPTICALTRACKERCOMBINEDACTION_H
#define NPDET_OPTICALTRACKERCOMBINEDACTION_H

/// Framework include files
#include <DDG4/Geant4SensDetAction.inl>
#include <DDG4/Geant4StepHandler.h>
#include <DDG4/Geant4Data.h>

#include <G4OpticalPhoton.hh>
#include <G4VSolid.hh>

#include <limits>
#include <optional>
#include <regex>
#include <sstream>
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
     * \package OpticalTrackerCombinedAction
     *
     * \brief Sensitive detector that routes steps to different hit-building strategies
     *        depending on the logical volume name, driven by a list of
     *        volume-regex → DDG4-action-name mappings.
     *
     *  Supported action names (per entry in \c VolumeActions):
     *  - \c Geant4OpticalTrackerAction  — one hit per step; optical photons are
     *    absorbed (\c fStopAndKill).
     *  - \c Geant4TrackerWeightedAction — deposits from the same G4Track within
     *    one sensitive element are accumulated into a single hit.
     *
     *  Typical use-case: a ring-imaging Cherenkov detector where MCP pixels and
     *  radiator bars share one sensitive detector but require different strategies.
     *
     * \param vector<string> VolumeActions
     *   One entry per volume group, each formatted as
     *   \c "volume_regex:ActionName[:key=value]*"
     *   Recognised per-entry keys (for \c Geant4TrackerWeightedAction):
     *   - \c CollectSingleDeposits=true|false  (default false)
     *   - \c HitPositionCombination=1|2|3|4    (default 2 = midpoint)
     *
     *  Example (two-entry DIRC configuration):
     *  \code
     *    VolumeActions = {
     *      "mcp_vol:Geant4OpticalTrackerAction",
     *      "bar_vol:Geant4TrackerWeightedAction:CollectSingleDeposits=false"
     *    }
     *  \endcode
     *
     * @}
     */

    /// Data struct holding per-event state for OpticalTrackerCombinedAction.
    struct OpticalTrackerCombined {

      /// Hit-position calculation modes (mirrors Geant4TrackerWeightedAction)
      enum {
        POSITION_WEIGHTED  = 1,
        POSITION_MIDDLE    = 2,
        POSITION_PREPOINT  = 3,
        POSITION_POSTPOINT = 4
      };

      // ------------------------------------------------------------------
      // Per-volume routing entry, built from a "vol_regex:ActionName[:k=v]*"
      // spec string during initialize().
      // ------------------------------------------------------------------
      struct VolumeEntry {
        enum class ActionType { Unknown, OpticalTracker, TrackerWeighted };

        std::string pattern;
        ActionType  action_type             { ActionType::Unknown };
        bool        collect_single_deposits { false };
        int         hit_position_type       { POSITION_MIDDLE };
        std::optional<std::regex> compiled_regex;

        static ActionType parseActionType(const std::string& name) {
          if (name == "Geant4OpticalTrackerAction")
            return ActionType::OpticalTracker;
          if (name == "Geant4TrackerWeightedAction" ||
              name == "Geant4TrackerCombineAction")
            return ActionType::TrackerWeighted;
          printout(WARNING, "OpticalTrackerCombinedAction",
                   "Unknown action name '%s' in VolumeActions entry", name.c_str());
          return ActionType::Unknown;
        }

        /// Parse "volume_regex:ActionName[:key=val]*"
        static VolumeEntry parse(const std::string& spec) {
          VolumeEntry entry;
          std::istringstream ss(spec);
          std::string token;
          int field = 0;
          while (std::getline(ss, token, ':')) {
            switch (field) {
            case 0: entry.pattern = token; break;
            case 1: entry.action_type = parseActionType(token); break;
            default: {
              auto eq = token.find('=');
              if (eq != std::string::npos) {
                std::string key = token.substr(0, eq);
                std::string val = token.substr(eq + 1);
                if (key == "CollectSingleDeposits")
                  entry.collect_single_deposits = (val == "true" || val == "1");
                else if (key == "HitPositionCombination")
                  entry.hit_position_type = std::stoi(val);
              }
              break;
            }
            }
            ++field;
          }
          if (!entry.pattern.empty()) {
            try {
              entry.compiled_regex.emplace(
                  entry.pattern,
                  std::regex_constants::ECMAScript | std::regex_constants::optimize);
            } catch (const std::regex_error& e) {
              printout(ERROR, "OpticalTrackerCombinedAction",
                       "Invalid regex '%s': %s", entry.pattern.c_str(), e.what());
            }
          }
          return entry;
        }

        bool matches(const std::string& lv_name) const {
          return compiled_regex && std::regex_search(lv_name, *compiled_regex);
        }
      };

      // ---- DDG4 property ----
      std::vector<std::string> m_volume_actions_raw;

      // ---- Parsed routing table (populated in initialize) ----
      std::vector<VolumeEntry> m_entries;

      // ---- TrackerWeighted state (shared across all tracker entries) ----
      Geant4Tracker::Hit    pre, post;
      Position              mean_pos;
      Geant4Sensitive*      sensitive          = nullptr;
      G4VSensitiveDetector* thisSD             = nullptr;
      G4VPhysicalVolume*    thisPV             = nullptr;
      double                distance_to_inside = 0.0;
      double                distance_to_outside= 0.0;
      double                mean_time          = 0.0;
      double                step_length        = 0.0;
      double                e_cut              = 0.0;
      int                   current            = -1;
      int                   parent             = 0;
      int                   combined           = 0;
      int                   hit_flag           = 0;
      int                   g4ID               = 0;
      EInside               last_inside        = kOutside;
      long long int         cell               = 0;
      /// Index into m_entries for the entry that started the current tracker hit
      int                   active_entry_idx   = -1;

      // ------------------------------------------------------------------
      // Helpers
      // ------------------------------------------------------------------

      void parseEntries() {
        m_entries.clear();
        for (const auto& spec : m_volume_actions_raw)
          m_entries.push_back(VolumeEntry::parse(spec));
      }

      // ------------------------------------------------------------------
      // TrackerWeighted helpers (adapted from DD4hep Geant4TrackerWeightedSD)
      // ------------------------------------------------------------------

      OpticalTrackerCombined& clear() {
        mean_pos.SetXYZ(0, 0, 0);
        distance_to_inside  = 0;
        distance_to_outside = 0;
        mean_time           = 0;
        step_length         = 0;
        thisPV              = nullptr;
        post.clear();
        pre.clear();
        current          = -1;
        parent           = -1;
        combined         =  0;
        cell             =  0;
        hit_flag         =  0;
        g4ID             =  0;
        active_entry_idx = -1;
        last_inside      = kOutside;
        return *this;
      }

      OpticalTrackerCombined& start(const G4Step* step, const G4StepPoint* point) {
        clear();
        pre.storePoint(step, point);
        pre.truth.deposit  = 0.0;
        post.truth.deposit = 0.0;
        current  = pre.truth.trackID;
        sensitive->mark(step->GetTrack());
        post.copyFrom(pre);
        parent = step->GetTrack()->GetParentID();
        g4ID   = step->GetTrack()->GetTrackID();
        Geant4StepHandler startVolume(step);
        thisPV = startVolume.preVolume();
        return *this;
      }

      OpticalTrackerCombined& update(const G4Step* step) {
        post.storePoint(step, step->GetPostStepPoint());
        Position mean    = (post.position + pre.position) * 0.5;
        double   mean_tm = (post.truth.time + pre.truth.time) * 0.5;
        pre.truth.deposit += post.truth.deposit;
        mean_pos.SetX(mean_pos.x() + mean.x() * post.truth.deposit);
        mean_pos.SetY(mean_pos.y() + mean.y() * post.truth.deposit);
        mean_pos.SetZ(mean_pos.z() + mean.z() * post.truth.deposit);
        mean_time   += mean_tm * post.truth.deposit;
        step_length += step->GetStepLength();
        if (0 == cell) {
          cell = sensitive->cellID(step);
          if (0 == cell) {
            cell = sensitive->volumeID(step);
            sensitive->except("+++ Invalid CELL ID for hit!");
          }
        }
        ++combined;
        return *this;
      }

      OpticalTrackerCombined& calc_dist_out(const G4VSolid* solid) {
        Position v(pre.momentum.unit()), &p = post.position;
        distance_to_outside = solid->DistanceToOut(
            G4ThreeVector(p.X(), p.Y(), p.Z()),
            G4ThreeVector(v.X(), v.Y(), v.Z()));
        return *this;
      }

      OpticalTrackerCombined& calc_dist_in(const G4VSolid* solid) {
        Position v(pre.momentum.unit()), &p = pre.position;
        distance_to_inside = solid->DistanceToOut(
            G4ThreeVector(p.X(), p.Y(), p.Z()),
            G4ThreeVector(v.X(), v.Y(), v.Z()));
        return *this;
      }

      void extractHit(EInside ended) {
        extractHit(sensitive->collection(0), ended);
      }

      void extractHit(Geant4HitCollection* collection, EInside ended) {
        double deposit = pre.truth.deposit;
        if (current != -1) {
          Position pos;
          Momentum mom;
          double time = deposit != 0 ? mean_time / deposit : mean_time;

          const int pos_type =
              (active_entry_idx >= 0 && active_entry_idx < (int)m_entries.size())
              ? m_entries[active_entry_idx].hit_position_type
              : POSITION_MIDDLE;

          switch (pos_type) {
          case POSITION_WEIGHTED:
            pos = deposit != 0 ? mean_pos / deposit : mean_pos;
            mom = 0.5 * (pre.momentum + post.momentum);
            break;
          case POSITION_PREPOINT:
            pos = pre.position;
            mom = pre.momentum;
            break;
          case POSITION_POSTPOINT:
            pos = post.position;
            mom = post.momentum;
            break;
          case POSITION_MIDDLE:
          default:
            pos = (post.position + pre.position) / 2.0;
            mom = 0.5 * (pre.momentum + post.momentum);
            break;
          }

          if (ended == kSurface ||
              distance_to_outside < std::numeric_limits<float>::epsilon())
            hit_flag |= Geant4Tracker::Hit::HIT_ENDED_SURFACE;
          else if (ended == kInside)
            hit_flag |= Geant4Tracker::Hit::HIT_ENDED_INSIDE;
          else if (ended == kOutside)
            hit_flag |= Geant4Tracker::Hit::HIT_ENDED_OUTSIDE;

          auto* hit = new Geant4Tracker::Hit(pre.truth.trackID,
                                             pre.truth.pdgID,
                                             deposit, time,
                                             step_length, pos, mom);
          hit->flag   = hit_flag;
          hit->cellID = cell;
          hit->g4ID   = g4ID;
          collection->add(hit);
        }
        clear();
      }

      // ------------------------------------------------------------------
      // Per-event callbacks
      // ------------------------------------------------------------------

      void startEvent() {
        thisSD = dynamic_cast<G4VSensitiveDetector*>(&sensitive->detector());
      }

      void endEvent() {
        if (current > 0) {
          sensitive->print("+++ OpticalTrackerCombined: flushing dangling tracker hit");
          extractHit(sensitive->collection(0), last_inside);
        }
      }

      // ------------------------------------------------------------------
      // Main process dispatcher
      // ------------------------------------------------------------------

      G4bool process(const G4Step* step, G4TouchableHistory* /*history*/) {
        Geant4StepHandler h(step);

        const G4LogicalVolume* lv = h.logvol(h.pre);
        if (!lv) return false;
        const std::string lv_name = lv->GetName();

        for (int idx = 0; idx < (int)m_entries.size(); ++idx) {
          const VolumeEntry& entry = m_entries[idx];
          if (!entry.matches(lv_name)) continue;

          // ----- Optical-tracker mode: one hit per step -----
          if (entry.action_type == VolumeEntry::ActionType::OpticalTracker) {
            typedef Geant4Tracker::Hit Hit;
            auto      contrib      = Hit::extractContribution(step);
            Direction hit_momentum = 0.5 * (h.preMom() + h.postMom());
            auto*     hit          = new Hit(contrib, hit_momentum, contrib.deposit);

            if (h.trackDef() == G4OpticalPhoton::OpticalPhotonDefinition())
              step->GetTrack()->SetTrackStatus(fStopAndKill);

            hit->cellID = sensitive->cellID(step);
            if (0 == hit->cellID) {
              hit->cellID = sensitive->volumeID(step);
              sensitive->except("+++ Invalid CELL ID for hit!");
            }
            sensitive->collection(0)->add(hit);
            sensitive->mark(h.track);
            return true;
          }

          // ----- Tracker-weighted mode: accumulate per track -----
          if (entry.action_type == VolumeEntry::ActionType::TrackerWeighted) {
            active_entry_idx = idx;
            const bool single_deposit = entry.collect_single_deposits;

            G4VSolid*     preSolid    = h.solid(h.pre);
            G4VSolid*     postSolid   = h.solid(h.post);
            G4ThreeVector local_pre   = h.globalToLocalG4(h.prePosG4());
            G4ThreeVector local_post  = h.globalToLocalG4(h.postPosG4());
            EInside       pre_inside  = preSolid->Inside(local_pre);
            EInside       post_inside = postSolid->Inside(local_post);

            const void* postPV = h.postVolume();
            const void* prePV  = h.preVolume();
            const void* postSD = h.postSD();
            const void* preSD  = h.preSD();
            G4VSolid*   solid  = (preSD == thisSD) ? preSolid : postSolid;

            if (current == h.trkID() && thisPV != nullptr && prePV != thisPV) {
              extractHit(post_inside);
              start(step, h.pre);
              active_entry_idx = idx;
            } else if (current == h.trkID() && !h.trkAlive()) {
              hit_flag |= Geant4Tracker::Hit::HIT_KILLED_TRACK;
              update(step).calc_dist_out(solid).extractHit(post_inside);
              return true;
            } else if (current == h.trkID() && postSD != thisSD) {
              update(step).calc_dist_out(solid).extractHit(kOutside);
              return true;
            } else if (current == h.trkID() && postSD == thisSD && post_inside == kSurface) {
              update(step).calc_dist_out(solid).extractHit(kSurface);
              return true;
            } else if (current == h.trkID() && postSD == thisSD && post_inside == kOutside) {
              update(step).calc_dist_out(solid).extractHit(post_inside);
              return true;
            } else if (current == h.trkID() && postSD == thisSD && post_inside == kInside) {
              last_inside = post_inside;
              update(step).calc_dist_out(solid);
              return true;
            } else if (current != h.trkID() && current >= 0) {
              extractHit(last_inside);
            }

            if (current < 0) {
              EInside inside = pre_inside;
              if (preSD != thisSD) {
                start(step, h.post);
                inside = post_inside;
              } else {
                start(step, h.pre);
              }
              active_entry_idx = idx;
              calc_dist_in(solid);
              if (inside == kSurface)
                hit_flag |= Geant4Tracker::Hit::HIT_STARTED_SURFACE;
              else if (inside == kInside)
                hit_flag |= Geant4Tracker::Hit::HIT_STARTED_INSIDE;
              else if (inside == kOutside)
                hit_flag |= Geant4Tracker::Hit::HIT_STARTED_OUTSIDE;
              if (inside == kInside)
                hit_flag |= Geant4Tracker::Hit::HIT_SECONDARY_TRACK;
            }

            last_inside = post_inside;
            update(step).calc_dist_out(solid);

            if (!h.trkAlive()) {
              hit_flag |= Geant4Tracker::Hit::HIT_KILLED_TRACK;
              extractHit(post_inside);
            } else if (post_inside == kSurface) {
              extractHit(post_inside);
            } else if (thisSD == preSD && (preSD != postSD || prePV != postPV)) {
              extractHit(post_inside);
            } else if (thisSD == postSD && (preSD != postSD || prePV != postPV)) {
              sensitive->error("+++ OpticalTrackerCombined: unexpected volume transition");
              extractHit(post_inside);
            } else if (single_deposit) {
              extractHit(post_inside);
            }
            return true;
          }

          break; // entry matched but action unknown — stop searching
        }

        return false; // no entry matched
      }
    }; // struct OpticalTrackerCombined

    // -----------------------------------------------------------------------
    // Template specialisations for Geant4SensitiveAction<OpticalTrackerCombined>
    // -----------------------------------------------------------------------

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::initialize() {
      declareProperty("VolumeActions", m_userData.m_volume_actions_raw);
      m_userData.e_cut     = m_sensitive.energyCutoff();
      m_userData.sensitive = this;
      m_userData.parseEntries();
    }

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::defineCollections() {
      m_collectionID = declareReadoutFilteredCollection<Geant4Tracker::Hit>();
    }

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::begin(G4HCofThisEvent* /*hce*/) {
      m_userData.startEvent();
    }

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::end(G4HCofThisEvent* /*hce*/) {
      m_userData.endEvent();
    }

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::clear(G4HCofThisEvent* /*hce*/) {
      m_userData.clear();
    }

    template <>
    G4bool Geant4SensitiveAction<OpticalTrackerCombined>::process(
        const G4Step* step, G4TouchableHistory* history) {
      return m_userData.process(step, history);
    }

    typedef Geant4SensitiveAction<OpticalTrackerCombined> OpticalTrackerCombinedAction;

  } // namespace sim
} // namespace dd4hep

#endif // NPDET_OPTICALTRACKERCOMBINEDACTION_H

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

#include <optional>
#include <regex>

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
     *        depending on the logical volume name:
     *
     *  - Volumes matching \c OpticalVolume (regex) receive \c Geant4OpticalTrackerAction
     *    behaviour: one hit per step, optical photons are absorbed (fStopAndKill).
     *  - Volumes matching \c TrackerVolume (regex) receive \c Geant4TrackerWeightedAction
     *    behaviour: deposits from the same G4Track within one sensitive element are
     *    combined into a single hit.
     *
     *  Typical use-case: a ring-imaging Cherenkov detector where MCP pixels and
     *  radiator bars share one sensitive detector but require different hit strategies.
     *
     * \param string OpticalVolume  Regex matching logical volume names for optical detection (default "mcp_vol")
     * \param string TrackerVolume  Regex matching logical volume names for charged-particle tracking (default "bar_vol")
     * \param bool   CollectSingleDeposits  When true, write one hit per step in tracker volumes (default false)
     * \param int    HitPositionCombination  Position strategy for tracker hits:
     *               1=energy-weighted, 2=midpoint (default), 3=pre-point, 4=post-point
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

      // ---- TrackerWeighted state (for tracker volumes) ----
      Geant4Tracker::Hit    pre, post;
      Position              mean_pos;
      Geant4Sensitive*      sensitive            = nullptr;
      G4VSensitiveDetector* thisSD               = nullptr;
      G4VPhysicalVolume*    thisPV               = nullptr;
      double                distance_to_inside   = 0.0;
      double                distance_to_outside  = 0.0;
      double                mean_time            = 0.0;
      double                step_length          = 0.0;
      double                e_cut                = 0.0;
      int                   current              = -1;
      int                   parent               = 0;
      int                   combined             = 0;
      int                   hit_flag             = 0;
      int                   g4ID                 = 0;
      EInside               last_inside          = kOutside;
      long long int         cell                 = 0;

      // ---- Configurable properties ----
      std::string m_optical_volume { "mcp_vol" };
      std::string m_tracker_volume { "bar_vol" };
      bool        m_single_deposit_mode  { false };
      int         m_hit_position_type    { POSITION_MIDDLE };

      // ---- Compiled regex cache ----
      mutable std::string               m_cached_optical_volume;
      mutable std::string               m_cached_tracker_volume;
      mutable std::optional<std::regex> m_optical_regex;
      mutable std::optional<std::regex> m_tracker_regex;

      // ------------------------------------------------------------------
      // Helpers
      // ------------------------------------------------------------------

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
            printout(ERROR, "OpticalTrackerCombinedAction",
                     "Invalid regex '%s': %s", expression.c_str(), e.what());
          }
        }
      }

      bool matches_optical(const std::string& lv_name) const {
        update_regex_cache(m_optical_volume, m_cached_optical_volume, m_optical_regex);
        return m_optical_regex && std::regex_search(lv_name, *m_optical_regex);
      }

      bool matches_tracker(const std::string& lv_name) const {
        update_regex_cache(m_tracker_volume, m_cached_tracker_volume, m_tracker_regex);
        return m_tracker_regex && std::regex_search(lv_name, *m_tracker_regex);
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
        current  = -1;
        parent   = -1;
        combined =  0;
        cell     =  0;
        hit_flag =  0;
        g4ID     =  0;
        last_inside = kOutside;
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

      bool mustSaveTrack(const G4Track* tr) const {
        return current > 0 && current != tr->GetTrackID();
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

          switch (m_hit_position_type) {
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
          Geant4HitCollection* coll = sensitive->collection(0);
          sensitive->print("+++ OpticalTrackerCombined: flushing dangling tracker hit");
          extractHit(coll, last_inside);
        }
      }

      // ------------------------------------------------------------------
      // Main process dispatcher
      // ------------------------------------------------------------------

      G4bool process(const G4Step* step, G4TouchableHistory* /*history*/) {
        Geant4StepHandler h(step);

        // Determine the logical volume name for the pre-step point
        const G4LogicalVolume* lv = h.logvol(h.pre);
        if (!lv) return false;
        const std::string lv_name = lv->GetName();

        // --- Optical-tracker mode: one hit per step, absorb optical photons ---
        if (matches_optical(lv_name)) {
          typedef Geant4Tracker::Hit Hit;
          auto      contrib      = Hit::extractContribution(step);
          Direction hit_momentum = 0.5 * (h.preMom() + h.postMom());
          double    hit_deposit  = contrib.deposit;
          auto*     hit          = new Hit(contrib, hit_momentum, hit_deposit);

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

        // --- Tracker-weighted mode: accumulate deposits per track per volume ---
        if (matches_tracker(lv_name)) {
          G4VSolid*     preSolid   = h.solid(h.pre);
          G4VSolid*     postSolid  = h.solid(h.post);
          G4ThreeVector local_pre  = h.globalToLocalG4(h.prePosG4());
          G4ThreeVector local_post = h.globalToLocalG4(h.postPosG4());
          EInside       pre_inside = preSolid->Inside(local_pre);
          EInside       post_inside= postSolid->Inside(local_post);

          const void* postPV = h.postVolume();
          const void* prePV  = h.preVolume();
          const void* postSD = h.postSD();
          const void* preSD  = h.preSD();
          G4VSolid*   solid  = (preSD == thisSD) ? preSolid : postSolid;

          // Track moved into a new physical volume – save current hit and restart
          if (current == h.trkID() && thisPV != nullptr && prePV != thisPV) {
            extractHit(post_inside);
            start(step, h.pre);
          }
          // Track killed inside SD
          else if (current == h.trkID() && !h.trkAlive()) {
            hit_flag |= Geant4Tracker::Hit::HIT_KILLED_TRACK;
            update(step).calc_dist_out(solid).extractHit(post_inside);
            return true;
          }
          // Track leaving SD volume (SD changed)
          else if (current == h.trkID() && postSD != thisSD) {
            update(step).calc_dist_out(solid).extractHit(kOutside);
            return true;
          }
          // Track leaving SD volume (surface)
          else if (current == h.trkID() && postSD == thisSD && post_inside == kSurface) {
            update(step).calc_dist_out(solid).extractHit(kSurface);
            return true;
          }
          // Track leaving SD volume (outside)
          else if (current == h.trkID() && postSD == thisSD && post_inside == kOutside) {
            update(step).calc_dist_out(solid).extractHit(post_inside);
            return true;
          }
          // Normal intermediate step
          else if (current == h.trkID() && postSD == thisSD && post_inside == kInside) {
            last_inside = post_inside;
            update(step).calc_dist_out(solid);
            return true;
          }
          // New secondary track – flush existing hit
          else if (current != h.trkID() && current >= 0) {
            extractHit(last_inside);
          }

          // Start a new hit
          if (current < 0) {
            EInside inside = pre_inside;
            if (preSD != thisSD) {
              start(step, h.post);
              inside = post_inside;
            } else {
              start(step, h.pre);
            }
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
          } else if (m_single_deposit_mode) {
            extractHit(post_inside);
          }
          return true;
        }

        // Volume matched neither pattern – pass through without recording
        return false;
      }
    }; // struct OpticalTrackerCombined

    // -----------------------------------------------------------------------
    // Template specialisations for Geant4SensitiveAction<OpticalTrackerCombined>
    // -----------------------------------------------------------------------

    template <>
    void Geant4SensitiveAction<OpticalTrackerCombined>::initialize() {
      declareProperty("OpticalVolume",        m_userData.m_optical_volume);
      declareProperty("TrackerVolume",        m_userData.m_tracker_volume);
      declareProperty("CollectSingleDeposits",m_userData.m_single_deposit_mode);
      declareProperty("HitPositionCombination", m_userData.m_hit_position_type);
      m_userData.e_cut      = m_sensitive.energyCutoff();
      m_userData.sensitive  = this;
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

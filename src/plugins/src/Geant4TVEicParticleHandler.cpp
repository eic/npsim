//==========================================================================
//  Geant4TVEicParticleHandler
//--------------------------------------------------------------------------
//  EIC-specific user particle handler.
//
//  Initial implementation: a verbatim clone of DD4hep's
//  Geant4TVUserParticleHandler (author: M.Frank, CERN). Behaves identically
//  to the upstream handler — rejects particles created outside the tracking
//  volume — and is registered as a distinct DD4hep/Geant4 plugin under the
//  name "Geant4TVEicParticleHandler" so that EIC-specific logic can be added
//  in follow-up changes without touching upstream DD4hep.
//
//  The two helpers `setReason` / `setSimulatorStatus` are copied inline from
//  DD4hep's DDG4/plugins/Geant4UserParticleHandlerHelper.{h,cpp}, because
//  that helper is internal to DD4hep's plugins folder and not part of the
//  installed public API.
//==========================================================================

// Framework include files
#include <DD4hep/Primitives.h>
#include <DD4hep/Volumes.h>
#include <DDG4/Factories.h>
#include <DDG4/Geant4Kernel.h>
#include <DDG4/Geant4Particle.h>
#include <DDG4/Geant4UserParticleHandler.h>

#include <array>

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    /// Rejects to keep particles which are created outside a tracking volume.
    /**
     *  EIC clone of Geant4TVUserParticleHandler. TV stands for TrackingVolume.
     *  Intended as the starting point for EIC-specific MC-truth filtering
     *  logic; for now it is behaviour-identical to the upstream handler.
     */
    class Geant4TVEicParticleHandler : public Geant4UserParticleHandler {
      Volume m_trackingVolume;

      /// EIC forward low-momentum rejection cut (configurable properties).
      /// Particles ending at Z > m_zMax with |p| < m_pMin are dropped.
      double m_zMax{5000.0};  ///< end-vertex Z threshold [mm]
      double m_pMin{100.0};   ///< momentum magnitude threshold [MeV]

    public:
      /// Standard constructor
      Geant4TVEicParticleHandler(Geant4Context* context, const std::string& nam);

      /// Default destructor
      virtual ~Geant4TVEicParticleHandler() {}

      /// Post-track action callback
      virtual void end(const G4Track* track, Particle& particle) override;

      /// Post-event action callback (overload to avoid -Woverloaded-virtual)
      virtual void end(const G4Event* event) override;
    };

    // ------------------------------------------------------------------
    // Inlined helpers (copied verbatim from DD4hep's
    // Geant4UserParticleHandlerHelper.cpp — that header is not installed
    // and cannot be #included from an external plugin).
    // ------------------------------------------------------------------
    namespace {

      /// determines if particle should be kept and sets p.reason = 0 otherwise
      void setReason(Geant4Particle& p, bool starts_in_trk_vol, bool ends_in_trk_vol) {
        dd4hep::detail::ReferenceBitMask<int> reason(p.reason);

        if (reason.isSet(G4PARTICLE_PRIMARY)) {
          // do nothing
          return;
        } else if (starts_in_trk_vol && !reason.isSet(G4PARTICLE_ABOVE_ENERGY_THRESHOLD)) {
          // created in tracking volume but below energy cut
          p.reason = 0;
          return;
        }
        // keep particles that left tracker hits if they are above threshold.
        if (reason.isSet(G4PARTICLE_CREATED_TRACKER_HIT) && reason.isSet(G4PARTICLE_ABOVE_ENERGY_THRESHOLD)) {
          return;
        }

        // created and ended in calo but not primary particle
        //
        // we can have particles from the generator only in the calo, if we have a
        // long particle with preassigned decay, we need to keep the reason or the
        // MChistory will not be updated later on
        if (!starts_in_trk_vol) {
          if (!ends_in_trk_vol) {
            p.reason = 0;
          }
          // fg: dont keep backscatter that did not create a tracker hit
          else if (!reason.isSet(G4PARTICLE_CREATED_TRACKER_HIT)) {
            p.reason = 0;
          }
        }
      }

      /// determines if particle has ended in the tracker, calorimeter or if it
      /// is backscatter, and sets the simulator status accordingly
      void setSimulatorStatus(Geant4Particle& p, bool starts_in_trk_vol, bool ends_in_trk_vol) {
        dd4hep::detail::ReferenceBitMask<int> simStatus(p.status);

        if (ends_in_trk_vol) {
          simStatus.set(G4PARTICLE_SIM_DECAY_TRACKER);
        }

        // if the particle doesn't end in the tracker volume it must have ended in the calorimeter
        if (not ends_in_trk_vol && not simStatus.isSet(G4PARTICLE_SIM_LEFT_DETECTOR)) {
          simStatus.set(G4PARTICLE_SIM_DECAY_CALO);
        }

        if (not starts_in_trk_vol && ends_in_trk_vol) {
          simStatus.set(G4PARTICLE_SIM_BACKSCATTER);
        }
      }

    } // anonymous namespace

    /// Standard constructor
    Geant4TVEicParticleHandler::Geant4TVEicParticleHandler(Geant4Context* ctxt, const std::string& nam)
        : Geant4UserParticleHandler(ctxt, nam) {
      m_trackingVolume = ctxt->kernel().detectorDescription().trackingVolume();
      declareProperty("ForwardZMax", m_zMax);
      declareProperty("ForwardMomentumMin", m_pMin);
    }

    /// Post-track action callback
    void Geant4TVEicParticleHandler::end(const G4Track* /* track */, Particle& p) {
      // numbers come from Geant4 and are in mm, we need to convert to ROOT unit system
      constexpr double unitFactor = dd4hep::mm;
      std::array<double, 3> start_point = {p.vsx * unitFactor, p.vsy * unitFactor, p.vsz * unitFactor};
      bool starts_in_trk_vol           = m_trackingVolume.ptr()->Contains(start_point.data());
      std::array<double, 3> end_point  = {p.vex * unitFactor, p.vey * unitFactor, p.vez * unitFactor};
      bool ends_in_trk_vol             = m_trackingVolume.ptr()->Contains(end_point.data());

      setReason(p, starts_in_trk_vol, ends_in_trk_vol);
      setSimulatorStatus(p, starts_in_trk_vol, ends_in_trk_vol);

      // EIC-specific extra cut: drop particles that end far forward
      // (Z > ForwardZMax [mm]) with low momentum (|p| < ForwardMomentumMin
      // [MeV]). Geant4Particle momentum psx/psy/psz are in MeV; vez in mm.
      // Both thresholds are configurable plugin properties.
      const double pmag = std::sqrt(p.psx * p.psx + p.psy * p.psy + p.psz * p.psz);
      if (p.vez > m_zMax && pmag < m_pMin) {
        p.reason = 0;
      }
    }

    /// Post-event action callback
    void Geant4TVEicParticleHandler::end(const G4Event* /* event */) {}

  } // namespace sim
} // namespace dd4hep

using dd4hep::sim::Geant4TVEicParticleHandler;
DECLARE_GEANT4ACTION(Geant4TVEicParticleHandler)

//==========================================================================
//  Geant4TVEicParticleHandler
//--------------------------------------------------------------------------
//  EIC-specific user particle handler. Registered as the DD4hep/Geant4
//  plugin "Geant4TVEicParticleHandler".
//
//  It runs the standard DD4hep tracking-volume filter — verbatim from
//  Geant4TVUserParticleHandler / Geant4UserParticleHandlerHelper
//  — and then layers an additional EIC-specific cut on top:
//
//    Drop low-|p| particles that ended in the far forward or far backward
//    Z regions, unless they are primaries, KEEP_ALWAYS, or contributed to
//    a tracker hit. Calorimeter-hit particles are optionally protected via
//    the KeepCaloHitParticles property.
//
//  Configurable properties (in DD4hep units):
//    ForwardRegionZ, BackwardRegionZ, ForwardMomentumMin,
//    BackwardMomentumMin, KeepCaloHitParticles.
//
//  See Geant4TVEicParticleHandler.md for the full description.
//
//  The helpers `setReason` / `setSimulatorStatus` are inlined from
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

#include <CLHEP/Units/SystemOfUnits.h>

#include <array>


namespace npdet::sim {

  /// EIC user particle handler: tracking-volume filter plus a regional
  /// forward/backward low-|p| cut.
  /**
   *  TV stands for TrackingVolume. The handler first runs the standard
   *  DD4hep tracking-volume filtering, then applies EIC-specific regional cuts
   */
  class Geant4TVEicParticleHandler : public dd4hep::sim::Geant4UserParticleHandler {
    dd4hep::Volume m_trackingVolume;

    double m_forwardZ{335 * dd4hep::cm};               ///< +Z dead-zone boundary (positive)
    double m_backwardZ{-175 * dd4hep::cm};             ///< -Z dead-zone boundary (negative)
    double m_forwardMomentumMin{100 * dd4hep::MeV};    ///< |p| threshold in +Z dead zone
    double m_backwardMomentumMin{100 * dd4hep::MeV};   ///< |p| threshold in -Z dead zone

    /// If true, particles carrying G4PARTICLE_CREATED_CALORIMETER_HIT are saved
    /// (preserves full calo shower MC truth at the cost of much larger MCParticles collections).
    /// Default false — upstream policy
    bool m_keepCaloHitParticles{false};

  public:

    Geant4TVEicParticleHandler(dd4hep::sim::Geant4Context* ctxt, const std::string& nam);

    /// Default destructor
    ~Geant4TVEicParticleHandler() override = default;

    /// Post-track action callback
    void end(const G4Track* track, Particle& particle) override;

    /// Post-event action callback (overload to avoid -Woverloaded-virtual)
    void end(const G4Event* event) override;
  };

  // ------------------------------------------------------------------
  // Inlined helpers (copied verbatim from DD4hep's
  // Geant4UserParticleHandlerHelper.cpp — that header is not installed
  // and cannot be #included from an external plugin).
  // ------------------------------------------------------------------
  namespace {

    /// determines if particle should be kept and sets p.reason = 0 otherwise
    void setReason(dd4hep::sim::Geant4Particle& p, bool starts_in_trk_vol, bool ends_in_trk_vol) {
      using namespace dd4hep::sim;

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
    void setSimulatorStatus(dd4hep::sim::Geant4Particle& p, bool starts_in_trk_vol, bool ends_in_trk_vol) {
      using namespace dd4hep::sim;

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
  Geant4TVEicParticleHandler::Geant4TVEicParticleHandler(dd4hep::sim::Geant4Context* ctxt, const std::string& nam)
      : Geant4UserParticleHandler(ctxt, nam) {
    m_trackingVolume = ctxt->kernel().detectorDescription().trackingVolume();
    if (!m_trackingVolume.isValid()) {
      except("Geant4TVEicParticleHandler requested but no tracking_volume defined in the XML. "
             "ePIC users: consider running with a more recent version of the ePIC geometry "
             "or setting --part.userParticleHandler=Geant4TCUserParticleHandler");
    }

    declareProperty("ForwardRegionZ", m_forwardZ);
    declareProperty("ForwardMomentumMin", m_forwardMomentumMin);
    declareProperty("BackwardRegionZ", m_backwardZ);
    declareProperty("BackwardMomentumMin", m_backwardMomentumMin);
    declareProperty("KeepCaloHitParticles", m_keepCaloHitParticles);
  }

  /// Post-track action callback.
  ///
  /// Runs once per simulated track at end-of-track. Two passes:
  ///   1. Standard DD4hep tracking-volume filter (setReason +
  ///      setSimulatorStatus) using the `tracking_volume` from the detector
  ///      description.
  ///   2. EIC regional cut: drop low-|p| particles ending in the far
  ///      forward/backward Z regions, after the must-keep guards.
  void Geant4TVEicParticleHandler::end(const G4Track* /* track */, Particle& p) {
    // Geant4Particle fields are stored in CLHEP/Geant4 units (mm, MeV).
    // We compare against properties that arrive from ddsim Python steering
    // already expressed in DD4hep units (cm, GeV). The explicit bridge is:
    //
    //   value_in_dd4hep = raw_field / CLHEP::<unit> * dd4hep::<unit>
    //
    // i.e. strip the Geant4 unit, then re-apply the DD4hep one.
    constexpr double mmCLHEPtoDD4hep  = dd4hep::mm  / CLHEP::mm;
    constexpr double MeVCLHEPtoDD4hep = dd4hep::MeV / CLHEP::MeV;

    // Positions for TGeoVolume::Contains (which works in DD4hep/ROOT units).
    std::array<double, 3> start_point = {p.vsx * mmCLHEPtoDD4hep,
                                         p.vsy * mmCLHEPtoDD4hep,
                                         p.vsz * mmCLHEPtoDD4hep};
    bool starts_in_trk_vol = m_trackingVolume.ptr()->Contains(start_point.data());
    std::array<double, 3> end_point   = {p.vex * mmCLHEPtoDD4hep,
                                         p.vey * mmCLHEPtoDD4hep,
                                         p.vez * mmCLHEPtoDD4hep};
    bool ends_in_trk_vol   = m_trackingVolume.ptr()->Contains(end_point.data());

    setReason(p, starts_in_trk_vol, ends_in_trk_vol);
    setSimulatorStatus(p, starts_in_trk_vol, ends_in_trk_vol);

    // ----- EIC-specific extra cut --------------------------------------
    // Drop low-momentum particles that ended in the far forward or far
    // backward Z regions. All thresholds (m_forwardZ, m_backwardZ,
    // m_forwardMomentumMin, m_backwardMomentumMin) are in DD4hep units.

    // Respect upstream "must-keep" reasons before zeroing.
    dd4hep::detail::ReferenceBitMask<int> reason(p.reason);
    if (reason.isSet(dd4hep::sim::G4PARTICLE_PRIMARY))             return;
    if (reason.isSet(dd4hep::sim::G4PARTICLE_KEEP_ALWAYS))         return;
    if (reason.isSet(dd4hep::sim::G4PARTICLE_CREATED_TRACKER_HIT)) return;
    if (m_keepCaloHitParticles && reason.isSet(dd4hep::sim::G4PARTICLE_CREATED_CALORIMETER_HIT)) return;

    const double pmag2 = (p.psx * p.psx + p.psy * p.psy + p.psz * p.psz) * (MeVCLHEPtoDD4hep * MeVCLHEPtoDD4hep);
    const double endZ  = p.vez * mmCLHEPtoDD4hep;

    if ((endZ > m_forwardZ  && pmag2 < m_forwardMomentumMin * m_forwardMomentumMin) ||
        (endZ < m_backwardZ && pmag2 < m_backwardMomentumMin * m_backwardMomentumMin)) {
      p.reason = 0;
    }
  }

  /// Post-event action callback
  void Geant4TVEicParticleHandler::end(const G4Event* /* event */) {}

} // namespace npdet::sim

namespace dd4hep::sim {
  using Geant4TVEicParticleHandler = npdet::sim::Geant4TVEicParticleHandler;
}

DECLARE_GEANT4ACTION(Geant4TVEicParticleHandler)

# Geant4TVEicParticleHandler

EIC-specific user particle handler. It is a drop-in replacement for DD4hep's
`Geant4TVUserParticleHandler` that adds a configurable **regional
forward/backward low-momentum cut** on top of the standard tracking-volume
MC-truth filtering.

**Plugin factory name:** `Geant4TVEicParticleHandler`
**Header / class namespace:** `npdet::sim::Geant4TVEicParticleHandler`
(also aliased as `dd4hep::sim::Geant4TVEicParticleHandler` for compatibility)

## What it does

The handler inherits from `dd4hep::sim::Geant4UserParticleHandler` and runs
once per simulated track at end-of-track. It performs two passes:

1. **Standard tracking-volume filtering** (verbatim from DD4hep's
   `Geant4TVUserParticleHandler` / `Geant4UserParticleHandlerHelper`)

2. **EIC regional cut** (new): low-momentum particles that ended in the far
   forward or far backward Z regions are dropped, subject to a small set of
   "must-keep" guards. This addresses the case where the global 1 MeV
   `MinimalKineticEnergy` cut is too loose to keep MCParticle multiplicity
   under control in regions where reconstruction is anyway impossible.

## Enabling the handler

In a Python steering file:

```python
from DDG4 import MeV, GeV, cm, mm, m

SIM.part.userParticleHandler = "Geant4TVEicParticleHandler"

# Optional: tune the regional cut. Defaults shown.
# The defaults for ForwardRegionZ / BackwardRegionZ match the legacy
# tracker_region_zmax / tracker_region_zmin constants in
# epic/compact/tracking_region.xml.
SIM.part.userParticleHandlerOptions = {
    "ForwardRegionZ":       335 * cm,     # +Z dead-zone boundary (positive)
    "BackwardRegionZ":     -175 * cm,     # -Z dead-zone boundary (negative)
    "ForwardMomentumMin":   100 * MeV,    # |p| cut in +Z dead zone
    "BackwardMomentumMin":  100 * MeV,    # |p| cut in -Z dead zone
    "KeepCaloHitParticles": False,        # see "Calo policy" below
}
```

Equivalent npsim/ddsim command-line flag:

```
--part.userParticleHandler=Geant4TVEicParticleHandler
```

The handler requires `tracking_volume` to be defined in the loaded detector
description (epic provides it via `tracking_region.xml`).

## Plugin properties

All thresholds are expressed in DD4hep units (cm, GeV). When set from
Python use the unit symbols (`MeV`, `mm`, `m`) so the values are
unit-correct regardless of host unit system.

| Property               | Type   | Units    | Meaning                                                                       |
|------------------------|--------|----------|-------------------------------------------------------------------------------|
| `ForwardRegionZ`       | double | length   | Particles ending at `endZ > ForwardRegionZ` are subject to the forward cut.   |
| `BackwardRegionZ`      | double | length   | Particles ending at `endZ < BackwardRegionZ` are subject to the backward cut. |
| `ForwardMomentumMin`   | double | momentum | Minimum \|p\| to be saved in the forward region                               |
| `BackwardMomentumMin`  | double | momentum | Minimum \|p\| to be saved in the backward region                              |
| `KeepCaloHitParticles` | bool   | —        | When true, unconditionally save particles that produced a calorimeter hit     |
| `OutputLevel`          | int    | —        | DD4hep `PrintLevel` (`VERBOSE=1 … ALWAYS=7`); inherited from `Geant4Action`   |

The boundaries are **signed Z values**.

## How the regional cut decides

After the standard filtering has run, the handler evaluates each surviving
particle (`p.reason != 0`) in this order:

1. `G4PARTICLE_PRIMARY` set → keep. Primaries are never touched.
2. `G4PARTICLE_KEEP_ALWAYS` set → keep. Honors explicit user opt-in.
3. `G4PARTICLE_CREATED_TRACKER_HIT` set → keep. Load-bearing: protects MC
   truth of particles that made hits in far-forward trackers
   (B0, Roman pots, off-momentum trackers) — all of which sit inside the
   tracking-volume extrusion.
4. `G4PARTICLE_CREATED_CALORIMETER_HIT` set **and** `KeepCaloHitParticles`
   is true → keep. 
5. Otherwise: compute `endZ` and `|p|` and apply
   - if `endZ > ForwardRegionZ`  and `|p| < ForwardMomentumMin`  → `p.reason = 0`
   - else if `endZ < BackwardRegionZ` and `|p| < BackwardMomentumMin` → `p.reason = 0`
     (with `BackwardRegionZ` itself negative)

The cut only **adds** rejection on top of the standard handler: a particle
already dropped by `setReason()` is unaffected (its reason is already 0).

## Calo policy and the `KeepCaloHitParticles` knob

Upstream DD4hep treats calorimeter hits as second-class for MC-truth
bookkeeping — `setReason()` will happily drop a particle whose only
contribution is a calo hit. This is *intentional* upstream: it keeps event
sizes manageable but means calorimeter shower secondaries can have their
MCParticle link remapped to a distant ancestor (the hit itself is still
written; only the truth link is degraded).

This handler exposes the choice as the `KeepCaloHitParticles` property:

- `KeepCaloHitParticles = False` (default) — match upstream policy.
- `KeepCaloHitParticles = True` — protect any particle with the
  `G4PARTICLE_CREATED_CALORIMETER_HIT` bit. Use for analyses that need
  full ZDC / B0ECal / forward calo MC truth (e.g. Lambda → n+π⁰ where
  the gammas hit ZDC).

The default is `false` deliberately — calorimeter showers are one of the
main contributors to large `MCParticles` collections in EIC simulations.
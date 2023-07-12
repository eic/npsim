#!/usr/bin/env python
"""
DD4hep simulation with some argument parsing
Based on M. Frank and F. Gaede runSim.py
   @author  A.Sailer
   @version 0.1

Modified with standard EIC EPIC requirements.
"""
from __future__ import absolute_import, unicode_literals
import logging
import sys

from DDSim.DD4hepSimulation import DD4hepSimulation


if __name__ == "__main__":
  logging.basicConfig(format='%(name)-16s %(levelname)s %(message)s', level=logging.INFO, stream=sys.stdout)
  logger = logging.getLogger('DDSim')

  SIM = DD4hepSimulation()

  # Ensure that Cerenkov and optical physics are always loaded
  def setupCerenkov(kernel):
    from DDG4 import PhysicsList
    seq = kernel.physicsList()
    cerenkov = PhysicsList(kernel, 'Geant4CerenkovPhysics/CerenkovPhys')
    cerenkov.MaxNumPhotonsPerStep = 10
    cerenkov.MaxBetaChangePerStep = 10.0
    cerenkov.TrackSecondariesFirst = False
    cerenkov.VerboseLevel = 0
    cerenkov.enableUI()
    seq.adopt(cerenkov)
    ph = PhysicsList(kernel, 'Geant4OpticalPhotonPhysics/OpticalGammaPhys')
    ph.addParticleConstructor('G4OpticalPhoton')
    ph.VerboseLevel = 0
    ph.enableUI()
    seq.adopt(ph)
    return None
  SIM.physics.setupUserPhysics(setupCerenkov)

  # Allow energy depositions to 0 energy in trackers (which include optical detectors)
  SIM.filter.tracker = 'edep0'

  # Some detectors are only sensitive to optical photons
  SIM.filter.filters['opticalphotons'] = dict(
    name='ParticleSelectFilter/OpticalPhotonSelector',
    parameter={"particle": "opticalphoton"},
  )
  # This could probably be a substring
  SIM.filter.mapDetFilter['DRICH'] = 'opticalphotons'
  SIM.filter.mapDetFilter['MRICH'] = 'opticalphotons'
  SIM.filter.mapDetFilter['PFRICH'] = 'opticalphotons'
  SIM.filter.mapDetFilter['DIRC'] = 'opticalphotons'

  # Use the optical tracker for the DRICH
  SIM.action.mapActions['DRICH'] = 'Geant4OpticalTrackerAction'
  SIM.action.mapActions['MRICH'] = 'Geant4OpticalTrackerAction'
  SIM.action.mapActions['PFRICH'] = 'Geant4OpticalTrackerAction'
  SIM.action.mapActions['DIRC'] = 'Geant4OpticalTrackerAction'

  # Set energy thresholds for all detectors
  # Note: For most detectors we can apply this threshold either here, in
  # digitization, or in reconstruction. To allow studies at each stage,
  # only some of the threshold may be applied here, so another 1/3 can be
  # applied at the digitization. Ideally nothing is applied at the
  # reconstruction anymore.
  # Ref: https://docs.google.com/spreadsheets/d/1s8oXj36SqIh7TJeHFH89gQ_ayU1_SVEpWQNkx6sETKs/
  energy_deposit_minimum_cuts = {
     "VertexBarrel": "0.65*keV",
     "SiBarrel": "0.65*keV",
     "MPGDBarrel": "0.25*keV",
     "TrackerEndcap": "0.65*keV",
     "BarrelTOF": "0.5*keV",
     "ForwardTOF": "0.5*keV",
     "EcalEndcapN": "5*MeV",
     "EcalBarrelImaging": "15*keV",
     "EcalEndcapP": "3*MeV",
     "B0Tracker": "1*keV",
     "B0ECal": "1*MeV",
     "ForwardRomanPot": "1*keV",
     "ForwardOffMTracker": "1*keV",
     "BackwardsTagger": "1*keV",
     "ZDC_Crystal": "1*MeV",
     "ZDC_WSi": "10*MeV",
     "ZDC_PbSi": "100*MeV",
     "ZDC_PbSci": "100*MeV",
  }
  for detector, cut in energy_deposit_minimum_cuts.items():
    name = f"EnergyDepositMinimumCut/{detector}/{cut}"
    SIM.filter.filters[name] = dict(name=name, parameter={"Cut": cut})
    SIM.filter.mapDetFilter[detector] = name

  # Parse remaining options (command line and steering file override above)
  SIM.parseOptions()

  try:
    SIM.run()
  except NameError as e:
    if "global name" in str(e):
      globalToSet = str(e).split("'")[1]
      logger.fatal("Unknown global variable, please add\nglobal %s\nto your steeringFile" % globalToSet)

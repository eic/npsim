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
  # Ref: https://docs.google.com/spreadsheets/d/1s8oXj36SqIh7TJeHFH89gQ_ayU1_SVEpWQNkx6sETKs/
  thresholds = {
     "VertexBarrel": 0.65*keV,
  }
  for detector, threshold in thresholds:
    name = f"EnergyDepositMinimumCut/{detector}"
    SIM.filter.filters[detector] = dict(name, {"Cut": threshold})
    SIM.filter.mapDetFilter[detector] = detector

  # Parse remaining options (command line and steering file override above)
  SIM.parseOptions()

  try:
    SIM.run()
  except NameError as e:
    if "global name" in str(e):
      globalToSet = str(e).split("'")[1]
      logger.fatal("Unknown global variable, please add\nglobal %s\nto your steeringFile" % globalToSet)

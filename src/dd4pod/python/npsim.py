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

  RUNNER = DD4hepSimulation()

  # Parse remaining options (command line and steering file override above)
  # This is done before updating the settings to workaround issue reported in
  # https://github.com/AIDASoft/DD4hep/pull/1376
  RUNNER.parseOptions()

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
  RUNNER.physics.setupUserPhysics(setupCerenkov)

  # Allow energy depositions to 0 energy in trackers (which include optical detectors)
  RUNNER.filter.tracker = 'edep0'

  # Some detectors are only sensitive to optical photons
  RUNNER.filter.filters['opticalphotons'] = dict(
    name='ParticleSelectFilter/OpticalPhotonSelector',
    parameter={"particle": "opticalphoton"},
  )
  # This could probably be a substring
  RUNNER.filter.mapDetFilter['DRICH'] = 'opticalphotons'
  RUNNER.filter.mapDetFilter['RICHEndcapN'] = 'opticalphotons'
  RUNNER.filter.mapDetFilter['DIRC'] = 'opticalphotons'

  # Use the optical tracker for the DRICH
  RUNNER.action.mapActions['DRICH'] = 'Geant4OpticalTrackerAction'
  RUNNER.action.mapActions['RICHEndcapN'] = 'Geant4OpticalTrackerAction'
  RUNNER.action.mapActions['DIRC'] = 'Geant4OpticalTrackerAction'

  # Use the G4PrimaryTransformer::SetKETolerance run action to suppress
  # warnings for off-shell unstable particles
  RUNNER.action.run = {
    "name": "G4PrimaryTransformerSetKEToleranceRunAction",
    "parameter": {
      "KETolerance": "1*MeV"
    }
  }
  
  # Use the optical photon efficiency stacking action for hpDIRC
  RUNNER.action.stack = [
    {
      "name": "OpticalPhotonEfficiencyStackingAction",
      "parameter": {
        "LambdaMin": "180*nm",
        "LambdaMax": "678*nm",
        "LogicalVolume": "bar_vol",
        "Efficiency": [e/100. for e in [
          0,    0,    14.0, 14.8, 14.5, 14.9, 14.4, 14.2, 13.9, 14.6, 15.2, 15.7, 16.4, 16.9, 17.5,
          17.7, 18.1, 18.8, 19.3, 19.8, 20.6, 21.4, 22.4, 23.1, 23.6, 24.1, 24.2, 24.6, 24.8, 25.2,
          25.7, 26.5, 27.1, 28.2, 29.0, 29.9, 30.8, 31.1, 31.7, 31.8, 31.6, 31.5, 31.5, 31.3, 31.0,
          30.8, 30.8, 30.4, 30.2, 30.3, 30.2, 30.1, 30.1, 30.1, 29.8, 29.9, 29.8, 29.7, 29.7, 29.7,
          29.8, 29.8, 29.9, 29.9, 29.8, 29.9, 29.8, 29.9, 29.8, 29.7, 29.8, 29.7, 29.8, 29.6, 29.5,
          29.7, 29.7, 29.8, 30.1, 30.4, 31.0, 31.3, 31.5, 31.8, 31.8, 31.9, 32.0, 32.0, 32.0, 32.0,
          32.2, 32.2, 32.1, 31.8, 31.8, 31.8, 31.7, 31.6, 31.6, 31.7, 31.5, 31.5, 31.4, 31.3, 31.3,
          31.2, 30.8, 30.7, 30.5, 30.3, 29.9, 29.5, 29.3, 29.2, 28.6, 28.2, 27.9, 27.8, 27.3, 27.0,
          26.6, 26.1, 25.9, 25.5, 25.0, 24.6, 24.2, 23.8, 23.4, 23.0, 22.7, 22.4, 21.9, 21.4, 21.2,
          20.7, 20.3, 19.8, 19.6, 19.3, 18.9, 18.7, 18.3, 17.9, 17.8, 17.8, 16.7, 16.5, 16.4, 16.0,
          15.6, 15.6, 15.2, 14.9, 14.6, 14.4, 14.1, 13.8, 13.6, 13.3, 13.0, 12.8, 12.6, 12.3, 12.0,
          11.9, 11.7, 11.5, 11.2, 11.1, 10.9, 10.7, 10.4, 10.3, 9.9,  9.8,  9.6,  9.3,  9.1,  9.0,
          8.8,  8.5,  8.3,  8.3,  8.2,  7.9,  7.8,  7.7,  7.5,  7.3,  7.1,  6.9,  6.7,  6.6,  6.3,
          6.2,  6.0,  5.8,  5.7,  5.6,  5.4,  5.2,  5.1,  4.9,  4.8,  4.6,  4.5,  4.4,  4.2,  4.1,
          4.0,  3.8,  3.7,  3.5,  3.3,  3.2,  3.1,  3.0,  2.9,  2.5,  2.4,  2.4,  2.3,  2.3,  2.1,
          1.8,  1.6,  1.5,  1.5,  1.6,  1.8,  1.9,  1.4,  0.8,  0.9,  0.8,  0.7,  0.6,  0.3,  0.3,
          0.5,  0.3,  0.4,  0.3,  0.1,  0.2,  0.1,  0.2,  0.3,  0.0
        ]]
      }
    }
  ]

  try:
    sys.exit(RUNNER.run())
  except NameError as e:
    if "global name" in str(e):
      globalToSet = str(e).split("'")[1]
      logger.fatal("Unknown global variable, please add\nglobal %s\nto your steeringFile" % globalToSet)

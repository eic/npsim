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
#ifndef DETECTORCHECKSUMRUNACTION_H
#define DETECTORCHECKSUMRUNACTION_H

#include <DDG4/Geant4Random.h>
#include <DDG4/Geant4RunAction.h>
#include <DDG4/RunParameters.h>
#include <format>

#if __has_include(<DD4hep/plugins/DetectorChecksum.h>)
#include <DD4hep/plugins/DetectorChecksum.h>
#endif

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {
        
/// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
namespace sim {

    using dd4hep::detail::DetectorChecksum;
    using hashMap_t = std::map<std::string, DetectorChecksum::hash_t>;

    template <class T = hashMap_t> void RunParameters::ingestParameters(T const& hashMap) {
      for (auto& [name, hash] : hashMap) {
        // hashes are 64 bit, so we have to cast to string
        m_strValues[name + "_hash"] = {std::format("{:x}",hash)};
      }
    }

    class DetectorChecksumRunAction: public Geant4RunAction {
    public:
      /// Standard constructor with initializing arguments
      DetectorChecksumRunAction(Geant4Context* c, const std::string& n)
      : Geant4RunAction(c, n) {
        declareProperty("ignoreDetectors", m_ignoreDetectors);
      };
      /// Default destructor
      virtual ~DetectorChecksumRunAction() {};

      void begin(const G4Run* run);
    private:
      hashMap_t getHashMap(Detector& detector) const;
      std::set<std::string> m_ignoreDetectors;
    };

void DetectorChecksumRunAction::begin(const G4Run* /* run */) {
  try {
    auto* parameters = context()->run().extension<RunParameters>(false);
    if (!parameters) {
      parameters = new RunParameters();
      context()->run().addExtension<RunParameters>(parameters);
    }
    parameters->ingestParameters(getHashMap(context()->detectorDescription()));
  } catch(std::exception &e) {
    printout(ERROR,"DetectorChecksumRunAction::begin","Failed to register run parameters: %s", e.what());
  }
}

std::map<std::string,DetectorChecksum::hash_t>
DetectorChecksumRunAction::getHashMap(Detector& detector) const {
  std::map<std::string, DetectorChecksum::hash_t> hashMap;
#if __has_include(<DD4hep/plugins/DetectorChecksum.h>)
  // Determine detector checksum
  // FIXME: ctor expects non-const detector
  DetectorChecksum checksum(detector);
  checksum.debug        = 0;
  checksum.precision    = 3;
  checksum.hash_meshes  = true;
  checksum.hash_readout = true;
  for (const auto& [name, det] : detector.world().children()) {
    if (m_ignoreDetectors.contains(name)) {
      continue;
    }
    checksum.analyzeDetector(det);
    DetectorChecksum::hashes_t hash_vec{checksum.handleHeader().hash};
    checksum.checksumDetElement(0, det, hash_vec, true);
    DetectorChecksum::hash_t hash =
        dd4hep::detail::hash64(&hash_vec[0], hash_vec.size() * sizeof(DetectorChecksum::hash_t));
    hashMap[name] = hash;
  }
#endif
  return hashMap;
}

} // End namespace sim
} // End namespace dd4hep

#endif // DETECTORCHECKSUMRUNACTION_H

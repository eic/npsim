#include "npdet/DetectorVersionRunAction.h"

// DD4hep includes
#include "DD4hep/Printout.h"
#include "DDG4/Geant4RunAction.h"
#include "DDG4/Factories.h"
#include "DDG4/Geant4Kernel.h"
#include "DD4hep/Detector.h"

// Geant4 includes
#include "G4Run.hh"

using namespace dd4hep::sim;

// Macro to suppress unused parameter warnings
#define UNUSED(x) (void)(x)

namespace npdet {

DetectorVersionRunAction::DetectorVersionRunAction(Geant4Context* context, const std::string& name)
    : Geant4RunAction(context, name) {
}

void DetectorVersionRunAction::begin(const G4Run* run) {
  UNUSED(run);
  // Get the DD4hep detector description
  auto& detector = context()->kernel().detectorDescription();
  
  try {
    // Try to get the epic_version constant
    double epic_version = detector.constantAsDouble("epic_version");
    
    // Store the epic_version in the action's properties
    this->property("epic_version").set(epic_version);
    dd4hep::printout(dd4hep::INFO, "DetectorVersionRunAction", 
                    "Stored epic_version = %f in action properties", epic_version);
    
  } catch (const std::exception& e) {
    dd4hep::printout(dd4hep::WARNING, "DetectorVersionRunAction", 
                    "Failed to get epic_version constant: %s", e.what());
  }
}

void DetectorVersionRunAction::end(const G4Run* run) {
  UNUSED(run);
}

} // namespace npdet

// Factory declaration for the RunAction
DECLARE_GEANT4ACTION_NS(npdet, DetectorVersionRunAction)

#ifndef NPDET_DETECTORVERSIONRUNACTION_H
#define NPDET_DETECTORVERSIONRUNACTION_H

#include "DDG4/Geant4Action.h"
#include "DDG4/Geant4RunAction.h"
#include "DD4hep/Detector.h"
#include "DDG4/RunParameters.h"

// Geant4 includes
#include "G4Run.hh"

/// Namespace for the npsim toolkit
namespace npdet {

/// DD4hep RunAction plugin for storing detector version information
/**
 * This class implements a Geant4 RunAction plugin that stores the detector
 * version information from DD4hep constants into run parameters.
 */
class DetectorVersionRunAction : public dd4hep::sim::Geant4RunAction {
public:
  /// Constructor
  /**
   * @param context The context object for the action
   * @param name The name of the action
   */
  DetectorVersionRunAction(dd4hep::sim::Geant4Context* context, const std::string& name);

  /// Destructor
  virtual ~DetectorVersionRunAction() = default;

  /// Begin-of-run callback
  virtual void begin(const G4Run* run) override;

  /// End-of-run callback
  virtual void end(const G4Run* run) override;
};

} // namespace npdet

#endif // NPDET_DETECTORVERSIONRUNACTION_H
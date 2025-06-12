#ifndef G4PRIMARYTRANSFORMERSETKETOLERANCERUNACTION_H
#define G4PRIMARYTRANSFORMERSETKETOLERANCERUNACTION_H

#include "DDG4/Geant4RunAction.h"

#include "G4Version.hh"
#include "G4SystemOfUnits.hh"
#if G4VERSION_NUMBER >= 1130
#include "G4ExceptionSeverity.hh"
#include "G4PrimaryTransformer.hh"
#endif

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {
        
  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    class G4PrimaryTransformerSetKEToleranceRunAction: public Geant4RunAction {
    public:
      /// Standard constructor with initializing arguments
      G4PrimaryTransformerSetKEToleranceRunAction(Geant4Context* c, const std::string& n)
      : Geant4RunAction(c, n) {
        declareProperty("KETolerance", m_KETolerance); // in G4 units
        declareProperty("KESeverity", m_KESeverity);
      };
      /// Default destructor
      virtual ~G4PrimaryTransformerSetKEToleranceRunAction() = default;
      /// begin-of-run callback
      void begin(const G4Run*)  override {
        #if G4VERSION_NUMBER >= 1130
        printout(INFO, name(), "setting KE tolerance to %f MeV", m_KETolerance / CLHEP::MeV);
        printout(INFO, name(), "setting KE severity to %s", m_KESeverity.c_str());
        G4ExceptionSeverity KESeverity{G4ExceptionSeverity::JustWarning};
        if (m_KESeverity == "IgnoreTheIssue") KESeverity = G4ExceptionSeverity::IgnoreTheIssue;
        G4PrimaryTransformer::SetKETolerance(m_KETolerance, KESeverity);
        #endif
      };
    private:
      double m_KETolerance{1.0 * dd4hep::MeV};
      std::string m_KESeverity{"JustWarning"};
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif // G4PRIMARYTRANSFORMERSETKETOLERANCERUNACTION_H

#ifndef DD4HEP_DDG4_EICInteractionVertexBoost_H
#define DD4HEP_DDG4_EICInteractionVertexBoost_H

// Framework include files
#include "DDG4/Geant4GeneratorAction.h"

/// Namespace for the AIDA detector description toolkit
namespace dd4hep {

  /// Namespace for the Geant4 based simulation part of the AIDA detector description toolkit
  namespace sim {

    /// Action class to boost the primary vertex (and all outgoing particles) of a single interaction
    /**
     * The vertex boost is steered by the Lorentz transformation angle.
     * The interaction to be modified is identified by the interaction's unique mask.
     *
     *  \author  M.Frank
     *  \version 1.0
     *  \ingroup DD4HEP_SIMULATION VertexBoost
     */
    class EICInteractionVertexBoost: public Geant4GeneratorAction {
    public:
      /// Interaction definition
      typedef Geant4PrimaryInteraction Interaction;

    protected:
      /// Property: The constant Lorentz transformation angle
      //double m_angle;
      /// Crossing angles relative to central B-field solenoid.
      double m_ionCrossingAngle = 0.0166667;
      double m_eCrossingAngle   = 0.00833333;
      /// Property: Unique identifier of the interaction to be modified
      int m_mask;

      /// Action routine to boost one single interaction according to the properties
      void boost(Interaction* interaction)  const;

    public:
      /// Inhibit default constructor
      EICInteractionVertexBoost() = delete;
      /// Standard constructor
      EICInteractionVertexBoost(Geant4Context* context, const std::string& name);
      /// Default destructor
      virtual ~EICInteractionVertexBoost();
      /// Callback to generate primary particles
      virtual void operator()(G4Event* event);
    };
  }    // End namespace sim
}      // End namespace dd4hep

#endif /* DD4HEP_DDG4_EICInteractionVertexBoost_H  */

#ifndef DD4HEP_DDG4_EICInteractionVertexBoost_H
#define DD4HEP_DDG4_EICInteractionVertexBoost_H

/** \addtogroup GeneratorAction Generator Actions
 * \brief  Here are some generator actions.
 *
 *  Generator A B C
 *
 * @{
   \addtogroup VertexBoosting Vertex Boost
 * \brief Boost the primary vertex (and all outgoing particles) of a single interaction.
 *
 * Generates the crossing angle.
 *
 * Here is an example of usage in python:
 *
 */

// Framework include files
#include "DDG4/Geant4GeneratorAction.h"

namespace npdet {

  namespace sim {

    using namespace dd4hep::sim;
    /// Action class to boost the primary vertex (and all outgoing particles) of a single interaction
    /**
     * The vertex boost is steered by the Lorentz transformation angle.
     * The interaction to be modified is identified by the interaction's unique mask.
     *
     *  \ingroup GeneratorAction VertexBoosting EIC
     */
    class EICInteractionVertexBoost: public Geant4GeneratorAction {
    public:
      /// Interaction definition
      using Interaction = Geant4PrimaryInteraction ;

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

//@}
#endif /* DD4HEP_DDG4_EICInteractionVertexBoost_H  */

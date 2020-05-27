#ifndef DD4HEP_DDG4_EICInteractionVertexSmear_H
#define DD4HEP_DDG4_EICInteractionVertexSmear_H

/** \addtogroup GeneratorAction
 * @{
   \addtogroup VertexSmearing Vertex Smearing
 * \brief Smear the primary vertex (and all outgoing particles) of a single interaction.
 *
 * Here is an example of usage in python:
 *
 */

// Framework include files
#include "DDG4/Geant4GeneratorAction.h"

// ROOT include files
#include "Math/Vector4D.h"

namespace npdet {
  namespace sim {

    using namespace dd4hep::sim;

    /** Smear the beam to account for the IP beam divergence.
     *
     * Action class to smear the primary vertex (and all outgoing particles) of a single interaction
     * The vertex smearing is steered by a 3D gaussian given by a constant offset and
     * the corresponding errors. The interaction to be modified is identified by the
     * interaction's unique mask.
     *
     *  \ingroup GeneratorAction VertexSmearing EIC
     *
     */
    class EICInteractionVertexSmear: public Geant4GeneratorAction {
    public:
      /// Interaction definition
      using Interaction = Geant4PrimaryInteraction;

    protected:
      /// Property: The constant smearing offset
      ROOT::Math::PxPyPzEVector m_offset = {0, 0, 0, 0};
      /// Property: sigma_x,y in units of angle.
      ROOT::Math::PxPyPzEVector m_sigma_Ion      = {0.000103, 0.000195, 0.0, 0.0};
      ROOT::Math::PxPyPzEVector m_sigma_Electron = {0.000215, 0.000156, 0.0, 0.0};
      /// Property: Unique identifier of the interaction created
      int m_mask;

      /// Action routine to smear one single interaction according to the properties
      void smear(Interaction* interaction)  const;
      
    public:
      /// Inhibit default constructor
      EICInteractionVertexSmear() = delete;
      /// Inhibit copy constructor
      EICInteractionVertexSmear(const EICInteractionVertexSmear& copy) = delete;
      /// Standard constructor
      EICInteractionVertexSmear(Geant4Context* context, const std::string& name);
      /// Default destructor
      virtual ~EICInteractionVertexSmear();
      /// Callback to generate primary particles
      virtual void operator()(G4Event* event);
    };
  }    // End namespace sim
}      // End namespace dd4hep



//@}
#endif /* DD4HEP_DDG4_EICInteractionVertexSmear_H  */

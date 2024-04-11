// Framework include files
#include <DD4hep/Printout.h>
#include <DD4hep/InstanceCount.h>
#include <DDG4/Factories.h>
#include <DDG4/Geant4Random.h>
#include <DDG4/Geant4Context.h>
#include <DDG4/Geant4InputHandling.h>
#include <DDG4/Geant4Action.h>
#include <DDG4/Geant4PhysicsList.h>

// eAST include files
#include <eAST/eASTPhysicsList.hh>

namespace dd4hep { 
  namespace sim {

    // Wrapper for eASTPhysicsList
    class eASTPhysicsListPlugin : public G4VPhysicsConstructor {
    protected:
      /// Reference to eAST physics list object
      eASTPhysicsList* m_eASTPhysicsList{nullptr};

    public:
      /// Standard constructor
      eASTPhysicsListPlugin()
      : G4VPhysicsConstructor() {
        m_eASTPhysicsList = new eASTPhysicsList();
      };
      /// Default destructor
      virtual ~eASTPhysicsListPlugin() = default;

      virtual void ConstructParticle() {
        m_eASTPhysicsList->ConstructParticle();
      };
      virtual void ConstructProcess() {
        m_eASTPhysicsList->ConstructProcess();
      }
    };

    DECLARE_GEANT4_PHYSICS(eASTPhysicsListPlugin)

  }     /* End namespace sim   */
}       /* End namespace dd4hep */


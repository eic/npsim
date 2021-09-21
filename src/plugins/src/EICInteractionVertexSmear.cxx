#include "DDG4/Factories.h"

// Framework include files
#include "DD4hep/Printout.h"
#include "DD4hep/InstanceCount.h"
#include "DDG4/Geant4Random.h"
#include "DDG4/Geant4Context.h"
#include "DDG4/Geant4InputHandling.h"
#include "npdet/EICInteractionVertexSmear.h"

// C/C++ include files
#include <cmath>

#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "Math/LorentzRotation.h"
#include "Math/AxisAngle.h"
#include "Math/Rotation3D.h"
#include "Math/RotationX.h"
#include "Math/RotationY.h"
#include "Math/Boost.h"
#include "Math/BoostX.h"
#include "Math/BoostX.h"
#include "Math/PxPyPzM4D.h"

namespace npdet::sim {

using namespace dd4hep::sim;

/// Standard constructor
EICInteractionVertexSmear::EICInteractionVertexSmear(Geant4Context* ctxt, const std::string& nam)
  : Geant4GeneratorAction(ctxt, nam)
{
  dd4hep::InstanceCount::increment(this);
  declareProperty("Offset", m_offset);
  declareProperty("Sigma_ion",  m_sigma_Ion);
  declareProperty("Sigma_e",  m_sigma_Electron);
  declareProperty("Mask",   m_mask = 0);
  m_needsControl = true;
}

/// Default destructor
EICInteractionVertexSmear::~EICInteractionVertexSmear() { dd4hep::InstanceCount::decrement(this); }

/// Action to smear one single interaction according to the properties
void EICInteractionVertexSmear::smear(Interaction* inter) const {
  using namespace ROOT::Math;
  Geant4Random& rndm = context()->event().random();
  if (inter) {
    double    dx  = rndm.gauss(m_offset.x(), m_sigma_Ion.x());
    double    dy  = rndm.gauss(m_offset.y(), m_sigma_Ion.y());
    //double    dz  = rndm.gauss(m_offset.z(), m_sigma_Ion.z());
    //double    dt  = rndm.gauss(m_offset.t(), m_sigma_Ion.t());
    //double    dxe = rndm.gauss(m_offset.x(), m_sigma_Electron.x());
    //double    dye = rndm.gauss(m_offset.y(), m_sigma_Electron.y());
    //double    dze = rndm.gauss(m_offset.z(), m_sigma_Electron.z());
    //double    dte = rndm.gauss(m_offset.t(), m_sigma_Electron.t());
    XYZVector ion_dir(0, 0, 1.0);
    RotationY rotx_ion(-dx);
    RotationX roty_ion(dy);
    auto      new_ion_dir = rotx_ion(roty_ion(ion_dir));

    Geant4PrimaryEvent::Interaction::VertexMap::iterator   iv;
    Geant4PrimaryEvent::Interaction::ParticleMap::iterator ip;

    double alpha         = new_ion_dir.Theta();
    auto   rotation_axis = ion_dir.Cross(new_ion_dir).Unit();

    if (inter->locked) {
      this->abortRun("Locked interactions may not be boosted!",
                     "Cannot boost interactions with a native G4 primary record!");
    } else if (alpha != 0.0) {

      double tanalpha  = std::tan(alpha/2.0);
      double gamma     = std::sqrt(1 + tanalpha * tanalpha);
      double betagamma = tanalpha;
      double beta      = betagamma / gamma;

      Polar3D boost_vec(beta, M_PI / 2.0, new_ion_dir.Phi());

      std::cout << " beta = " << beta << "\n";

      Boost           bst(boost_vec);
      LorentzRotation roty(AxisAngle(rotation_axis, alpha / 2.0));

      // Now move begin and end-vertex of all primary vertices accordingly
      for (iv = inter->vertices.begin(); iv != inter->vertices.end(); ++iv) {
        for (Geant4Vertex* v : (*iv).second) {
          XYZTVector v0(v->x, v->y, v->z, v->time);
          auto       v1 = roty(bst * v0);
          v->x          = v1.x();
          v->y          = v1.y();
          v->z          = v1.z();
          v->time       = v1.t();
        }
      }
      // Now move begin and end-vertex of all primary and generator particles accordingly
      for (ip = inter->particles.begin(); ip != inter->particles.end(); ++ip) {
        Geant4ParticleHandle p = (*ip).second;
        XYZTVector           v0{p->vsx, p->vsy, p->vsz, p->time};
        auto                 v1 = roty(bst * v0);

        PxPyPzMVector p0{p->psx, p->psy, p->psz, p->mass};
        auto          p1 = roty(bst * p0);

        p->vsx  = v1.x();
        p->vsy  = v1.y();
        p->vsz  = v1.z();
        p->time = v1.t();

        p->psx = p1.x();
        p->psy = p1.y();
        p->psz = p1.z();
      }
    }

    // smearInteraction(this,inter,dx,dy,dz,dt);
    return;
  } else {
    //print("+++ Smearing primary vertex for interaction type %d (%d Vertices, %d particles) "
    //      "by (%+.2e mm, %+.2e mm, %+.2e mm, %+.2e ns)",
    //      m_mask, int(inter->vertices.size()), int(inter->particles.size()), dx, dy, dz, dt);
    print("+++ No interaction of type %d present.", m_mask);
  }
}

/// Callback to generate primary particles
void EICInteractionVertexSmear::operator()(G4Event*) {
  typedef std::vector<Geant4PrimaryInteraction*> _I;
  Geant4PrimaryEvent* evt = context()->event().extension<Geant4PrimaryEvent>();

  if ( m_mask >= 0 )  {
    Interaction* inter = evt->get(m_mask);
    smear(inter);
    return;
  }
  _I interactions = evt->interactions();
  for(_I::iterator i=interactions.begin(); i != interactions.end(); ++i)
    smear(*i);
}
}

namespace dd4hep::sim {
using EICInteractionVertexSmear = npdet::sim::EICInteractionVertexSmear;
}
DECLARE_GEANT4ACTION(EICInteractionVertexSmear)


#include "DDG4/Factories.h"

// Framework include files
#include "DD4hep/Printout.h"
#include "DD4hep/InstanceCount.h"
#include "DDG4/Geant4InputHandling.h"
#include "npdet/EICInteractionVertexBoost.h"

#include "Math/Vector4D.h"
#include "Math/LorentzRotation.h"
#include "Math/Boost.h"
#include "Math/BoostX.h"
#include "Math/BoostX.h"
#include "Math/PxPyPzM4D.h"

using namespace dd4hep::sim;

/// Standard constructor
EICInteractionVertexBoost::EICInteractionVertexBoost(Geant4Context* ctxt, const std::string& nam)
  : Geant4GeneratorAction(ctxt, nam)
{
  InstanceCount::increment(this);
  //declareProperty("Angle", m_angle = 0);
  declareProperty("IonCrossingAngle", m_ionCrossingAngle = 0.0166667);
  declareProperty("ElectronCrossingAngle", m_eCrossingAngle = 0.00833333);
  declareProperty("Mask",  m_mask = 1);
  m_needsControl = true;
}

/// Default destructor
EICInteractionVertexBoost::~EICInteractionVertexBoost() {
  InstanceCount::decrement(this);
}

/// Action to boost one single interaction according to the properties
void EICInteractionVertexBoost::boost(Interaction* inter)  const  {
  using namespace ROOT::Math;
      //    boostInteraction(this, inter, m_angle);
      // int dd4hep::sim::boostInteraction(const Geant4Action* caller,
      //                                  Geant4PrimaryEvent::Interaction* inter,
      //                                  double alpha)
  if (inter) {
    Geant4PrimaryEvent::Interaction::VertexMap::iterator   iv;
    Geant4PrimaryEvent::Interaction::ParticleMap::iterator ip;

    double alpha = m_ionCrossingAngle+m_eCrossingAngle;

    if (inter->locked) {
      this->abortRun("Locked interactions may not be boosted!",
                       "Cannot boost interactions with a native G4 primary record!");
    } else if (alpha != 0.0) {

      double tanalpha  = std::tan(alpha/2.0);
      double gamma     = std::sqrt(1 + tanalpha * tanalpha);
      double betagamma = tanalpha;
      double beta      = betagamma / gamma;

      std::cout << " beta   = " << beta  << "\n";
      std::cout << " gamma  = " << gamma << "\n";
      std::cout << " alpha  = " << alpha*180.0/M_PI << " deg\n";

      BoostX           bst(beta);
      LorentzRotation roty(RotationY(alpha/2.0-m_eCrossingAngle));

      // Now move begin and end-vertex of all primary vertices accordingly
      for (iv = inter->vertices.begin(); iv != inter->vertices.end(); ++iv) {
        for (Geant4Vertex* v : (*iv).second) {
          XYZTVector v0(v->x,v->y,v->z,v->time);
          auto v1 = roty(bst*v0);
          //double t = gamma * v->time + betagamma * v->x / CLHEP::c_light;
          //double x = gamma * v->x + betagamma * CLHEP::c_light * v->time;
          //double y = v->y;
          //double z = v->z;
          v->x     = v1.x();
          v->y     = v1.y();
          v->z     = v1.z();
          v->time  = v1.t();
        }
      }
      // Now move begin and end-vertex of all primary and generator particles accordingly
      for (ip = inter->particles.begin(); ip != inter->particles.end(); ++ip) {
        Geant4ParticleHandle p = (*ip).second;
        //double               t = gamma * p->time + betagamma * p->vsx / CLHEP::c_light;
        //double               x = gamma * p->vsx + betagamma * CLHEP::c_light * p->time;
        //double               y = p->vsy;
        //double               z = p->vsz;

        XYZTVector v0{p->vsx,p->vsy,p->vsz,p->time};
        auto v1 = roty(bst*v0);

        PxPyPzMVector p0{p->psx,p->psy,p->psz,p->mass};
        auto p1 = roty(bst*p0);
        //double m  = p->mass;
        //double e2 = SQR(p->psx) + SQR(p->psy) + SQR(p->psz) + SQR(m);
        //double px = betagamma * std::sqrt(e2) + gamma * p->psx;
        //double py = p->psy;
        //double pz = p->psz;

        p->vsx  = v1.x();
        p->vsy  = v1.y();
        p->vsz  = v1.z();
        p->time = v1.t();

        p->psx = p1.x();
        p->psy = p1.y();
        p->psz = p1.z();

        std::cout << p1 << "\n";
        std::cout << " theta = " << p1.Theta() << " \n";
      }
    }
  }
  else {
    print("+++ No interaction of mask/type %d present.", m_mask);
  }
}

/// Callback to generate primary particles
void EICInteractionVertexBoost::operator()(G4Event*) {
  typedef std::vector<Geant4PrimaryInteraction*> _I;
  Geant4PrimaryEvent* evt = context()->event().extension<Geant4PrimaryEvent>();

  if ( m_mask >= 0 )  {
    std::cout << " mask is not zero\n";
    Interaction* inter = evt->get(m_mask);
    boost(inter);
    return;
  }
  _I interactions = evt->interactions();
  for(_I::iterator i=interactions.begin(); i != interactions.end(); ++i)
    boost(*i);
}

DECLARE_GEANT4ACTION(EICInteractionVertexBoost)


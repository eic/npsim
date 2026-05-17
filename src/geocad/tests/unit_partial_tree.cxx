#include <iostream>

#include "src/TOCCToStep.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TGeoVolume.h"

namespace {
TGeoManager* make_test_geometry() {
  auto* geom = new TGeoManager("npdet_geo_test", "npdet geocad unit test");
  auto* mat  = new TGeoMaterial("Vacuum", 0, 0, 0);
  auto* med  = new TGeoMedium("Vacuum", 1, mat);

  auto* top   = geom->MakeBox("top", med, 100., 100., 100.);
  auto* child = geom->MakeBox("child", med, 10., 10., 10.);
  top->AddNode(child, 1);
  geom->SetTopVolume(top);
  geom->CloseGeometry();
  return geom;
}
} // namespace

int main() {
  auto* geom = make_test_geometry();

  TOCCToStep converter;
  converter.OCCShapeCreation(geom, 1.0);

  const bool found_child = converter.OCCPartialTreeCreation(geom, "child", 2);
  const bool found_fake  = converter.OCCPartialTreeCreation(geom, "does_not_exist", 2);

  delete geom;

  if (!found_child || found_fake) {
    std::cerr << "Unexpected partial tree lookup results\n";
    return 1;
  }

  return 0;
}

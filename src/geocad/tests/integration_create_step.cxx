#include <filesystem>
#include <iostream>
#include <string>

#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMedium.h"
#include "TGeoToStep.h"
#include "TGeoVolume.h"

namespace fs = std::filesystem;

namespace {
TGeoManager* make_test_geometry() {
  auto* geom = new TGeoManager("npdet_geo_test", "npdet geocad integration test");
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

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Expected output path argument\n";
    return 1;
  }

  const fs::path out_path = argv[1];
  fs::remove(out_path);

  TGeoToStep converter(make_test_geometry());
  converter.CreateGeometry(out_path.string().c_str(), 2, 1.0);

  if (!fs::exists(out_path) || fs::file_size(out_path) == 0) {
    std::cerr << "STEP output file was not created\n";
    return 1;
  }

  return 0;
}

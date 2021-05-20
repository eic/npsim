#include "settings.h"

#include "DD4hep/Detector.h"
#include "DD4hep/Printout.h"
#include "DDG4/Geant4Data.h"
#include "DDRec/CellIDPositionConverter.h"
#include "DDRec/SurfaceManager.h"
#include "DDRec/Surface.h"
#include "DD4hep/DD4hepUnits.h"
#include "TGeoNode.h"
#include "TGeoManager.h"

void print_daughter_nodes(TGeoNode* node, int print_depth) {
  TGeoIterator nextNode( node->GetVolume() );
  int path_index = 0;
  TGeoNode* currentNode = nullptr;

  nextNode.SetType(0);
  while( (currentNode = nextNode()) ) {
    // Not iterator level starts at 1 
    auto nlevel = nextNode.GetLevel();
    if( nlevel > print_depth ) {
      continue;
    }
    for(int i=0;i<nlevel; i++){
      std::cout << "  \t";
    }
    std::cout << "/" <<currentNode->GetName() << " \t(vol: " << currentNode->GetVolume()->GetName() << ")" << "\n";
  }
}
void run_list_mode(const settings& s)
{
  dd4hep::setPrintLevel(dd4hep::WARNING);
  gErrorIgnoreLevel = kWarning;// kPrint, kInfo, kWarning,

  // -------------------------
  // Get the DD4hep instance
  // Load the compact XML file
  dd4hep::Detector& detector = dd4hep::Detector::getInstance();
  detector.fromCompact(s.infile);

  if(s.part_name_levels.size() == 0 ){
    std::cout << gGeoManager->GetPath() << "\n";
    print_daughter_nodes(gGeoManager->GetTopNode(), 1);
  }
  for(const auto& [p,l] : s.part_name_levels){
    bool dir = gGeoManager->cd(p.c_str());
    if (!dir) {
      std::cerr << p << " not found!\n";
      continue;
    }
    std::cout << p << std::endl;
    TGeoNode *node = gGeoManager->GetCurrentNode();
    print_daughter_nodes(node, l);
  }

  //detector.manager().GetTopVolume()->GetNodes()->Print();
  //detector.manager().GetTopVolume()->GetNode(0)->Dump();
} 
//______________________________________________________________________________

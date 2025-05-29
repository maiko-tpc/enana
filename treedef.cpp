#include "analysis.hpp"

void analysis::TreeDef(){
  tree = new TTree("tree", "tree");

  tree->Branch("eve", &eve, "eve/I");
  tree->Branch("blk", &blk, "blk/I");  

  tree->Branch("madc", &evt.madc,
	       Form("adc[%d]/i:counter[%d]/l",
                    N_MADC_CH, N_MADC));
  
  tree->Branch("ppac_f2u_good", &evt.ppac_f2u_good, "ppac_f2u_good/I");
  tree->Branch("ppac_f2d_good", &evt.ppac_f2d_good, "ppac_f2d_good/I");  
  tree->Branch("ppac_f3u_good", &evt.ppac_f3u_good, "ppac_f3u_good/I");
  tree->Branch("ppac_f3d_good", &evt.ppac_f3d_good, "ppac_f3d_good/I");  
}

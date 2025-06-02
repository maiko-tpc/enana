#include "analysis.hpp"

void analysis::TreeDef(){
  tree = new TTree("tree", "tree");

  tree->Branch("eve", &eve, "eve/I");
  tree->Branch("blk", &blk, "blk/I");  

  tree->Branch("madc", &evt.madc,
	       Form("adc[%d]/i:counter[%d]/l",
                    N_MADC_CH, N_MADC));
  
  tree->Branch("ppac_good", evt.ppac_good, "ppac_good[4]/O");
  
  tree->Branch("ppac_pos_raw", evt.ppac_pos_raw,
	       "ppac_pos_raw[4][2]/I");
}

#include "analysis.hpp"

void analysis::TreeDef(){
  tree = new TTree("tree", "tree");

  tree->Branch("eve", &eve, "eve/I");
  tree->Branch("blk", &blk, "blk/I");  

  tree->Branch("madc", &evt.madc,
	       Form("adc[%d]/i:counter[%d]/l",
                    N_MADC_CH, N_MADC));

  tree->Branch("mdpp", &evt.mdpp,
	       Form("adc[%d]/i:tdc[%d]/i:counter[%d]/l",
                    N_MDPP_CH, N_MDPP_CH, N_MDPP));
  
  tree->Branch("ppac_good", evt.ppac_good, "ppac_good[4]/O");
  
  tree->Branch("ppac_pos_raw", evt.ppac_pos_raw,
	       "ppac_pos_raw[4][2]/I");

  tree->Branch("ppac_pos_cal", evt.ppac_pos_cal,
	       "ppac_pos_cal[4][2]/F");

  tree->Branch("ppac_ang", evt.ppac_ang,
	       "ppac_ang[2][2]/F");
  
  tree->Branch("ssd", &evt.ssd,
	       Form("adc[%d]/I:ene[%d]/F:tdc[%d]/I",
		    N_SSD, N_SSD, N_SSD));

  tree->Branch("ref_tdc", &evt.ref_tdc, "ref_tdc/I");
  tree->Branch("rf", evt.rf, Form("rf[%d]/I", N_RF));
  tree->Branch("pl_tdc", evt.pl_tdc, Form("pl_tdc[%d]/I", N_PL_CH));  

  tree->Branch("is_pid_f2", &evt.is_pid_f2, "is_pid_f2/O");
  tree->Branch("is_pid_f3", &evt.is_pid_f3, "is_pid_f3/O");  
}

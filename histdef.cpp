#include "analysis.hpp"

void analysis::HistDef(){
  char *ppac_name[4] = {"F2U", "F2D", "F3U", "F3D"};

  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i] =  new TH1F(Form("h_madc%d", i),
			 Form("MADC32 ADC ch%02d", i),
			 8196, 0, 8196);
    vec_th1.push_back(hmadc[i]);
  }

  for(int i=0; i<4; i++){
    hppac_good[i] = new TH1F(Form("h_ppac%d_good", i),
			     Form("%s PPAC good hit", ppac_name[i]),
			     2, 0, 2);
    vec_th1.push_back(hppac_good[i]);
  }
} // end of histdef

void analysis::HistFill(){
  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i]->Fill(evt.madc.adc[i]);
  }

  hppac_good[0]->Fill(evt.ppac_f2u_good);
  hppac_good[1]->Fill(evt.ppac_f2d_good);
  hppac_good[2]->Fill(evt.ppac_f3u_good);
  hppac_good[3]->Fill(evt.ppac_f3d_good);  
}


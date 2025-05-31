#include "analysis.hpp"

void analysis::HistDef(){
  char *ppac_name[N_PPAC] = {"F2U", "F2D", "F3U", "F3D"};

  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i] =  new TH1F(Form("h_madc%d", i),
			 Form("MADC32 ADC ch%02d", i),
			 8196, 0, 8196);
    vec_th1.push_back(hmadc[i]);
  }

  for(int i=0; i<N_PPAC; i++){
    hppac_good[i] = new TH1F(Form("h_ppac%d_good", i),
			     Form("%s PPAC good hit", ppac_name[i]),
			     2, 0, 2);
    vec_th1.push_back(hppac_good[i]);

    hppac_pos2d[i] = new TH2F(Form("h_ppac%d_pos2d", i),
			      Form("%s PPAC 2D position", ppac_name[i]),
			      500,-1000,1000, 500,-1000,1000);
    vec_th2.push_back(hppac_pos2d[i]);
    hppac_pos2d[i]->SetDrawOption("colz");
  }
  
} // end of histdef

void analysis::HistFill(){
  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i]->Fill(evt.madc.adc[i]);
  }

  for(int i=0; i<N_PPAC; i++){
    hppac_good[i]->Fill(evt.ppac_good[i]);

    if(evt.ppac_good[i]==1){
      hppac_pos2d[i]->Fill(evt.ppac_pos_raw[i][0], evt.ppac_pos_raw[i][1]);
    }
  }
}


#include "analysis.hpp"

void analysis::HistDef(){
  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i] =  new TH1F(Form("h_madc%d", i),
			 Form("MADC32 ADC ch%02d", i),
			 8196, 0, 8196);
  }
}
void analysis::HistFill(){
  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i]->Fill(evt.madc.adc[i]);
  }
}


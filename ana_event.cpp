#include "ana_event.hpp"
#include "analysis.hpp"

void analysis::ana_event(){
  evt.eve++;

  // MADC analysis
  ana_madc32();

  // PPAC ana
  for(int i=0; i<N_PPAC; i++){
    ppac[i].analyze(&evt);
    evt.ppac_good[i] = ppac[i].IsGoodHit();
  }
  
  for(int i=0; i<N_PPAC; i++){
    for(int j=0; j<N_PPAC_CH; j++){
      evt.ppac_tdc[i][j] = ppac[i].tdc_val[j];
    }
  }

  for(int i=0; i<N_PPAC; i++){
    for(int j=0; j<2; j++){ // X and Y
      evt.ppac_pos_raw[i][j] = ppac[i].pos_raw[j];
    }
  }
  
  // http control
  extern int CLEAR_FLAG;

  if(GetWeb() && CLEAR_FLAG==1){
    HttpHistReset();  // only reset when the bottun is pressed
    CLEAR_FLAG=0;
  }
}


void analysis::init_event(){
  evt.v1190_hit_all.clear();
  evt.mxdc32_hit_all.clear();  

  for(int i=0; i<N_PPAC; i++){
    evt.ppac_good[i]=0;

    for(int j=0; j<2; j++){
      evt.ppac_pos_raw[i][j] = -1000;
    }
  }
}

int analysis::ana_madc32(){
  int mxdc32_size = evt.mxdc32_hit_all.size();
  unsigned int tmp_ch=0;
  unsigned int tmp_adc=0;
  unsigned long int tmp_counter=0;
  for(int i=0; i<mxdc32_size; i++){
    tmp_ch  = evt.mxdc32_hit_all[i].ch;
    if(tmp_ch!=32)tmp_adc = (unsigned int)evt.mxdc32_hit_all[i].adc;
    if(tmp_ch!=33)tmp_counter = evt.mxdc32_hit_all[i].adc;    

    evt.madc.adc[tmp_ch] = tmp_adc;
    evt.madc.counter[0] = tmp_counter;    
  }

  return 0;
}


ana_ppac::ana_ppac(){
  tdc_geo = 0;
  for(int i=0; i<PPAC_TDC_N_CH; i++){
    tdc_ch[i] = 0;
  }
}

ana_ppac::~ana_ppac(){
}

void ana_ppac::SetPPACi(int index){
  ippac = index;
}

void ana_ppac::SetTDCGeo(unsigned int geo){
  tdc_geo = geo;
}

void ana_ppac::SetTDCCh(unsigned int *ch){
  for(int i=0; i<PPAC_TDC_N_CH; i++){
    tdc_ch[i] = ch[i];
  }
}

int ana_ppac::analyze(evtdata *evt){

  // initialize parameters
  for(int i=0; i<PPAC_TDC_N_CH; i++){
    tdc_val[i]=0;
    tdc_hit[i]=0;
  }

  for(int i=0; i<2; i++){
    pos_raw[i] = -10000;
    pos_cal[i] = -10000;    
  }

  hit_bit=0;
  
  int v1190_size = evt->v1190_hit_all.size();
  
  // get TDC hit
  for(int i=0; i< v1190_size; i++){
    if(evt->v1190_hit_all[i].geo == tdc_geo){
      for(int j=0; j<PPAC_TDC_N_CH; j++){
	if(evt->v1190_hit_all[i].ch == tdc_ch[j] && tdc_hit[j]==0){
	  tdc_val[j] = evt->v1190_hit_all[i].lead_raw;
	  tdc_hit[j] = 1;
	}
      }
    }
  } // end of  for(int i=0; i< v1190_size; i++){

  // calculate position
  if(tdc_hit[0]==1 && tdc_hit[2]==1){ // X=L-R
    pos_raw[0] = tdc_val[2] - tdc_val[0];
    pos_cal[0] = pos_raw[0];
  }

  if(tdc_hit[1]==1 && tdc_hit[3]==1){ // Y=D-U
    pos_raw[1] = tdc_val[1] - tdc_val[3];
    pos_cal[1] = pos_raw[1];
  }  
  
  return 0;
}

bool ana_ppac::IsGoodHit(){
  for(int i=0; i<4; i++){
    if(tdc_hit[i]!=1) return 0;
  }
  return 1;
}

int ana_ppac::GetPosRaw(int ixy){
  return pos_raw[ixy];
}

double ana_ppac::GetPosCal(int ixy){
  return pos_cal[ixy];
}

#include "ana_event.hpp"
#include "analysis.hpp"

void analysis::ana_event(){
  evt.eve++;

  // MADC analysis
  ana_madc32();

  // MDPP analysis
  ana_mdpp16();

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
      evt.ppac_pos_cal[i][j] = ppac[i].pos_cal[j];
    }
  }
  
  for(int i=0; i<2; i++){ // X & Y angle loop
    evt.ppac_ang[0][i] = (ppac[1].pos_cal[i] - ppac[0].pos_cal[i])/400.0; //F2
    evt.ppac_ang[1][i] = (ppac[3].pos_cal[i] - ppac[2].pos_cal[i])/500.0; //F3   
  }

  // SSD ana
  ana_ssd();
  ana_rf();
  ana_pl();  

  // PID ana
  if(evt.use_pid_f2){
    evt.is_pid_f2 = pid_f2->IsInside(evt.rf[2]-evt.ref_tdc, evt.ssd.ene[0]);
  }
  if(evt.use_pid_f3){
    evt.is_pid_f3 = pid_f3->IsInside(evt.rf[2]-evt.ref_tdc, evt.ssd.ene[1]);
  }

  // http control
  extern int CLEAR_FLAG;
  //  extern int STOP_FLAG;  

  if(GetWeb() && CLEAR_FLAG==1){
    HttpHistReset();  // only reset when the bottun is pressed
    CLEAR_FLAG=0;
  }
}


void analysis::init_event(){
  evt.v1190_hit_all.clear();
  evt.mxdc32_hit_all.clear();
  evt.mdpp16_hit_all.clear();    

  for(int i=0; i<N_PPAC; i++){
    evt.ppac_good[i]=0;

    for(int j=0; j<2; j++){
      evt.ppac_pos_raw[i][j] = -1000;
      evt.ppac_pos_cal[i][j] = -110;
    }
    for(int i=0; i<2; i++){
      for(int j=0; j<2; j++){
	evt.ppac_ang[i][j] = -110;
      }
    }
  }
  
  for(int i=0; i<N_SSD; i++){
    evt.ssd.adc[i] = 0;
    evt.ssd.ene[i] = -1;
    evt.ssd.tdc[i] = -1000;
  }

  evt.ref_tdc = 0;
  for(int i=0; i<N_RF; i++) evt.rf[i] = -1000;
  for(int i=0; i<N_PL_CH; i++) evt.pl_tdc[i] = -1000;  

  evt.is_pid_f2 = 0;
  evt.is_pid_f3 = 0;  

} // end of init

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

int analysis::ana_mdpp16(){
  int mdpp16_size = evt.mdpp16_hit_all.size();
  unsigned int tmp_ch=0;
  unsigned int tmp_adc=0;
  unsigned int tmp_tdc=0;  
  for(int i=0; i<mdpp16_size; i++){
    tmp_ch  = evt.mdpp16_hit_all[i].ch;

    if(tmp_ch<16){
      tmp_adc = (unsigned int)evt.mdpp16_hit_all[i].adc;
      evt.madc.adc[tmp_ch] = tmp_adc;
    }
    if(tmp_ch>=16 && tmp_ch<32){
      tmp_tdc = (unsigned int)evt.mdpp16_hit_all[i].adc;
      evt.madc.adc[tmp_ch-16] = tmp_tdc;
    }

    if(tmp_ch==34)evt.madc.counter[0] = evt.mdpp16_hit_all[i].adc;
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
  if(tdc_hit[0]==1 && tdc_hit[2]==1){ // X=L-R, same as Trong's definition
    pos_raw[0] = tdc_val[0] - tdc_val[2];
    pos_cal[0] = cal_par_p0[ippac][0] + pos_raw[0]*cal_par_p1[ippac][0];
    pos_cal[0]+= offset[ippac][0];  // offset to adjust to Trong's result
  }

  if(tdc_hit[1]==1 && tdc_hit[3]==1){ // Y=D-U
    pos_raw[1] = tdc_val[1] - tdc_val[3];
    pos_cal[1] = cal_par_p0[ippac][1] + pos_raw[1]*cal_par_p1[ippac][1];
    pos_cal[1]+= offset[ippac][1];
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


void analysis::ana_ssd(){
  evt.ssd.adc[0] = evt.madc.adc[1];
  evt.ssd.adc[1] = evt.madc.adc[4];  

  evt.ssd.ene[0] = 0.0085221*evt.madc.adc[1] - 0.2563; // F2
  evt.ssd.ene[1] = 0.0092344*evt.madc.adc[4] - 0.8454; // F3 

  int v1190_size = evt.v1190_hit_all.size();
  for(int i=0; i<v1190_size; i++){
    if(evt.v1190_hit_all[i].ch == 32){ // F2
      evt.ssd.tdc[0] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 36){ // F2
      evt.ssd.tdc[0] = evt.v1190_hit_all[i].lead_raw;
    }
  }
}


void analysis::ana_rf(){

  int v1190_size = evt.v1190_hit_all.size();
  for(int i=0; i<v1190_size; i++){
    if(evt.v1190_hit_all[i].ch == 31) evt.ref_tdc = evt.v1190_hit_all[i].lead_raw;

    if(evt.v1190_hit_all[i].ch == 12){ 
      evt.rf[0] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 13){ 
      evt.rf[1] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 14){ 
      evt.rf[2] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 15){ 
      evt.rf[3] = evt.v1190_hit_all[i].lead_raw;
    }
  }
}

void analysis::ana_pl(){

  int v1190_size = evt.v1190_hit_all.size();
  for(int i=0; i<v1190_size; i++){
    if(evt.v1190_hit_all[i].ch == 26){ 
      evt.pl_tdc[0] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 27){ 
      evt.pl_tdc[1] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 28){ 
      evt.pl_tdc[2] = evt.v1190_hit_all[i].lead_raw;
    }
    if(evt.v1190_hit_all[i].ch == 29){ 
      evt.pl_tdc[3] = evt.v1190_hit_all[i].lead_raw;
    }
  }
}

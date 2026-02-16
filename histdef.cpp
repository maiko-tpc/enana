#include "analysis.hpp"

const double min_z = -500;
const double max_z = 3500;
const double step_z = 1;
int size_z = (max_z-min_z)/step_z;
const double attpc_z = 2400;


void analysis::HistDef(){
  char *ppac_name[N_PPAC] = {"F2U", "F2D", "F3U", "F3D"};

  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i] =  new TH1F(Form("h_madc%d", i),
			 Form("MADC32 ADC ch%02d", i),
			 8196, 0, 8196);
    vec_th1.push_back(hmadc[i]);
  }
  hmadc_total = new TH1F("h_mdac_total", "MADC32 ADC ch16-31(except 28)", 8196, 0, 8196);
  vec_th1.push_back(hmadc_total);

  for(int i=0; i<N_PPAC; i++){
    hppac_good[i] = new TH1F(Form("h_ppac%d_good", i),
			     Form("%s PPAC good hit", ppac_name[i]),
			     2, 0, 2);
    vec_th1.push_back(hppac_good[i]);

    hppac_pos2d[i] = new TH2F(Form("h_ppac%d_pos2d", i),
			      Form("%s PPAC 2D position", ppac_name[i]),
			      500,-1000,1000, 500,-1000,1000);
    vec_th2.push_back(hppac_pos2d[i]);

    hppac_pos2d_cal[i] = new TH2F(Form("h_ppac%d_pos2d_cal", i),
			      Form("%s PPAC 2D position mm", ppac_name[i]),
			      250,-50,50, 250,-50,50);
    vec_th2.push_back(hppac_pos2d_cal[i]);
  }

  h_f2_pos[0] = new TH1F("h_f2_pos_x", "F2 viewer position X", 200,-100, 100);
  h_f2_pos[1] = new TH1F("h_f2_pos_y", "F2 viewer position Y", 200,-100, 100);
  vec_th1.push_back(h_f2_pos[0]);  
  vec_th1.push_back(h_f2_pos[1]);  
  
  hppac_track[0][0] = new TH2F("h_ppac_track0", "F2 PPAC track Z-X",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track[0][1] = new TH2F("h_ppac_track1", "F2 PPAC track Z-Y",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track[1][0] = new TH2F("h_ppac_track2", "F3 PPAC track Z-X",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track[1][1] = new TH2F("h_ppac_track3", "F3 PPAC track Z-Y",
			    size_z,min_z,max_z, 200,-50,50);


  hppac_track_pid[0][0] = new TH2F("h_ppac_track0_pid", "F2 track Z-X PID",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track_pid[0][1] = new TH2F("h_ppac_track1_pid", "F2 track Z-Y PID",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track_pid[1][0] = new TH2F("h_ppac_track2_pid", "F3 track Z-X PID",
			    size_z,min_z,max_z, 200,-50,50);
  hppac_track_pid[1][1] = new TH2F("h_ppac_track3_pid", "F3 track Z-Y PID",
			    size_z,min_z,max_z, 200,-50,50);
  
  for(int i=0; i<2; i++){
    for(int j=0; j<2; j++){
      vec_th2.push_back(hppac_track[i][j]);
      vec_th2.push_back(hppac_track_pid[i][j]);      
    }
  }
  
  h_attpc_x  = new TH1F("h_attpc_x",  Form("Beam X  pos at ATTPC (z=%d)", (int)attpc_z),
		       200, -50, 50);
  h_attpc_y  = new TH1F("h_attpc_y",  Form("Beam Y  pos at ATTPC (z=%d)", (int)attpc_z),
		       200, -50, 50);
  h_attpc_xy = new TH2F("h_attpc_xy", Form("Beam XY pos at ATTPC (z=%d)", (int)attpc_z),
			200, -50, 50, 200, -50, 50);

  vec_th1.push_back(h_attpc_x);
  vec_th1.push_back(h_attpc_y);
  vec_th2.push_back(h_attpc_xy);    

  for(int i=0; i<N_SSD; i++){
    hssd_ene[i] = new TH1F(Form("h_ssd_ene%d", i), Form("SSD energy ch%d", i),
			   512, 0, 60);
    vec_th1.push_back(hssd_ene[i]);
  }
  
  hpid_f2 = new TH2F("h_pid_f2", "Si energy vs RF F2",
		     1000,0,5000, 500,0,60);
  hpid_f3 = new TH2F("h_pid_f3", "Si energy vs RF F3",
		     1000,0,5000, 500,0,60);

  vec_th2.push_back(hpid_f2);
  vec_th2.push_back(hpid_f3);  

  // TOF
  for(int i=0; i<4; i++){
    h_tof_f3ppac[i] = new TH1F(Form("h_tof_f3ppac%d",i),
			       Form("TOF betwen F3PPAC and RF-ch%d",i),
			       1000, 4000, 6000);
    vec_th1.push_back(h_tof_f3ppac[i]);
  }
  
} // end of histdef


constexpr double params[16][2] = {
	{-37., 0.471},
	{-34.,0.597},
	{-33.,0.549},
	{-33, 0.623},
	{-33, 0.553},
	{-31, 0.693},
	{-30, 0.574},
	{-32, 0.654},
	{-40, 0.600},
	{-32.,0.509},
	{-39.,0.532},
	{-31.,0.618},
	{-40.,0.502},
	{-32.,0.489},
	{-40.,0.517},
	{-34.,0.516}
};

void analysis::HistFill(){
  for(int i=0; i<N_MADC_CH; i++){
    hmadc[i]->Fill(evt.madc.adc[i]);
  }

  for(int i=0; i<N_PPAC; i++){
    hppac_good[i]->Fill(evt.ppac_good[i]);

    if(evt.ppac_good[i]==1){
      hppac_pos2d[i]->Fill(evt.ppac_pos_raw[i][0], evt.ppac_pos_raw[i][1]);
      hppac_pos2d_cal[i]->Fill(evt.ppac_pos_cal[i][0], evt.ppac_pos_cal[i][1]);      
    }
  }

  if(evt.ppac_good[0]==1 && evt.ppac_good[1]==1){
    h_f2_pos[0]->Fill(evt.f2_pos[0]);
    h_f2_pos[1]->Fill(evt.f2_pos[1]);    
  }

  for(int i=0; i<N_SSD; i++){
    hssd_ene[i]->Fill(evt.ssd.ene[i]);
  }

  // track fill
  double z=0;
  double x,y;

  // F2
  if( evt.ppac_good[0]==1 && evt.ppac_good[1]==1){
    for(z=min_z; z<max_z; z+=step_z){
      x = evt.ppac_pos_cal[0][0] + z*evt.ppac_ang[0][0];
      y = evt.ppac_pos_cal[0][1] + z*evt.ppac_ang[0][1];
      hppac_track[0][0]->Fill(z, x);
      hppac_track[0][1]->Fill(z, y);      

      if(evt.is_pid_f2==1){
	hppac_track_pid[0][0]->Fill(z, x);
	hppac_track_pid[0][1]->Fill(z, y);      
      }
      
    }
  }

  // F3
  if( evt.ppac_good[2]==1 && evt.ppac_good[3]==1){
    for(z=min_z; z<max_z; z+=step_z){
      x = evt.ppac_pos_cal[2][0] + z*evt.ppac_ang[1][0];
      y = evt.ppac_pos_cal[2][1] + z*evt.ppac_ang[1][1];
      hppac_track[1][0]->Fill(z, x);
      hppac_track[1][1]->Fill(z, y);      
      if(evt.is_pid_f3==1){
	hppac_track_pid[1][0]->Fill(z, x);
	hppac_track_pid[1][1]->Fill(z, y);      
      }

      if( (z-0.5)<attpc_z && (z+0.5)>attpc_z ){
	h_attpc_x->Fill(x);
	h_attpc_y->Fill(y);
	h_attpc_xy->Fill(x,y);		
      }

    }
  }

  // PID
  hpid_f2->Fill(evt.rf[2]-evt.ref_tdc, evt.ssd.ene[0]);
  hpid_f3->Fill(evt.rf[2]-evt.ref_tdc, evt.ssd.ene[1]);  

  for(int i=0; i<4; i++){
    h_tof_f3ppac[i]->Fill(evt.rf[i]-evt.ref_tdc);
  }
  
} /// end of HistFill()

int analysis::CloseROOT(){

  printf("closing root file...\n");
  
  int size1 = vec_th1.size();
  for(int i=0; i<size1; i++){
    vec_th1[i]->Write();
  }

  int size2 = vec_th2.size();
  for(int i=0; i<size2; i++){
    vec_th2[i]->Write();
  }
  
  tree->Write();
  outroot->Close();
  return 0;
}

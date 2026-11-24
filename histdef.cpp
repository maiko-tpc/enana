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

  
  // MWDC
  for(int i=0; i<N_MWDC; i++){
    for(int j=0; j<N_MWDC_PLANE; j++){
      h_mwdc_hit[i][j] = new TH1F(Form("h_mwdc_hit_%d_%d", i,j),
				  Form("MWDC %d PL%d Hit", i,j),
				  N_MWDC_WIRE, 0, N_MWDC_WIRE);
      vec_th1.push_back(h_mwdc_hit[i][j]);
    }
  }
  

  // Explicit histogram <-> event-value bindings.  These mappings deliberately
  // exclude multi-fill histograms (tracks, MWDC hit patterns, MADC total).
  // The callback is the actual value extractor; the expression is a label.
  for (int i = 0; i < N_MADC_CH; ++i) {
    cutHistogramRegistry.Register1D(hmadc[i], "/MADC",
      "madc.adc[" + std::to_string(i) + "]",
      [i](const evtdata &e) { return double(e.madc.adc[i]); });
  }

  for (int i = 0; i < N_PPAC; ++i) {
    const std::string n = std::to_string(i);
    cutHistogramRegistry.Register1D(hppac_good[i], "/PPAC",
      "ppac_good[" + n + "]",
      [i](const evtdata &e) { return double(e.ppac_good[i]); });

    auto valid = [i](const evtdata &e) { return e.ppac_good[i] == 1; };
    const std::string condition = "ppac_good[" + n + "]";
    cutHistogramRegistry.Register2D(hppac_pos2d[i], "/PPAC",
      "ppac_pos_raw[" + n + "][0]",
      [i](const evtdata &e) { return double(e.ppac_pos_raw[i][0]); },
      "ppac_pos_raw[" + n + "][1]",
      [i](const evtdata &e) { return double(e.ppac_pos_raw[i][1]); },
      valid, condition);
    cutHistogramRegistry.Register2D(hppac_pos2d_cal[i], "/PPAC",
      "ppac_pos_cal[" + n + "][0]",
      [i](const evtdata &e) { return double(e.ppac_pos_cal[i][0]); },
      "ppac_pos_cal[" + n + "][1]",
      [i](const evtdata &e) { return double(e.ppac_pos_cal[i][1]); },
      valid, condition);
  }

  auto f2Valid = [](const evtdata &e) {
    return e.ppac_good[0] == 1 && e.ppac_good[1] == 1;
  };
  for (int axis = 0; axis < 2; ++axis) {
    cutHistogramRegistry.Register1D(h_f2_pos[axis], "/PPAC",
      "f2_pos[" + std::to_string(axis) + "]",
      [axis](const evtdata &e) { return double(e.f2_pos[axis]); },
      f2Valid, "ppac_good[0] && ppac_good[1]");
  }

  for (int i = 0; i < N_SSD; ++i) {
    cutHistogramRegistry.Register1D(hssd_ene[i], "/SSD",
      "ssd.ene[" + std::to_string(i) + "]",
      [i](const evtdata &e) { return double(e.ssd.ene[i]); });
  }

  cutHistogramRegistry.Register2D(hpid_f2, "/PID",
    "rf[2]-ref_tdc",
    [](const evtdata &e) { return double(e.rf[2] - e.ref_tdc); },
    "ssd.ene[0]",
    [](const evtdata &e) { return double(e.ssd.ene[0]); });
  cutHistogramRegistry.Register2D(hpid_f3, "/PID",
    "rf[2]-ref_tdc",
    [](const evtdata &e) { return double(e.rf[2] - e.ref_tdc); },
    "ssd.ene[1]",
    [](const evtdata &e) { return double(e.ssd.ene[1]); });

  for (int i = 0; i < 4; ++i) {
    cutHistogramRegistry.Register1D(h_tof_f3ppac[i], "/PID",
      "rf[" + std::to_string(i) + "]-ref_tdc",
      [i](const evtdata &e) { return double(e.rf[i] - e.ref_tdc); });
  }

  // HistFill() uses the F3 track extrapolated to z=attpc_z.
  auto f3Valid = [](const evtdata &e) {
    return e.ppac_good[2] == 1 && e.ppac_good[3] == 1;
  };
  auto attpcX = [](const evtdata &e) {
    return double(e.ppac_pos_cal[2][0] + attpc_z*e.ppac_ang[1][0]);
  };
  auto attpcY = [](const evtdata &e) {
    return double(e.ppac_pos_cal[2][1] + attpc_z*e.ppac_ang[1][1]);
  };
  const std::string attpcCondition = "ppac_good[2] && ppac_good[3]";
  cutHistogramRegistry.Register1D(h_attpc_x, "/PPAC",
    "ppac_pos_cal[2][0]+2400*ppac_ang[1][0]",
    attpcX, f3Valid, attpcCondition);
  cutHistogramRegistry.Register1D(h_attpc_y, "/PPAC",
    "ppac_pos_cal[2][1]+2400*ppac_ang[1][1]",
    attpcY, f3Valid, attpcCondition);
  cutHistogramRegistry.Register2D(h_attpc_xy, "/PPAC",
    "ppac_pos_cal[2][0]+2400*ppac_ang[1][0]", attpcX,
    "ppac_pos_cal[2][1]+2400*ppac_ang[1][1]", attpcY,
    f3Valid, attpcCondition);

  // Single-call example: no analysis.hpp pointer, manual Fill(),
  // Web registration or vector push_back is needed.
  cutHistogramRegistry.Book1D(
      "h_rf2_ref_booked", "RF ch2 - reference TDC (booked)", "/PID",
      1000, 0, 5000,
      "rf[2]-ref_tdc",
      [](const evtdata &e) { return double(e.rf[2] - e.ref_tdc); });

  cutHistogramRegistry.Book2D(
      "h_rf0_ssd0_booked", "RF ch0 vs SSD0 energy (booked)", "/PID",
      1000, 0, 5000, 500, 0, 60,
      "rf[0]-ref_tdc",
      [](const evtdata &e) { return double(e.rf[0] - e.ref_tdc); },
      "ssd.ene[0]",
      [](const evtdata &e) { return double(e.ssd.ene[0]); });

  cutHistogramRegistryInfo.SetTitle(cutHistogramRegistry.ToJson().c_str());
  printf("Registered %zu cut histogram mappings (metadata and value callbacks)\n",
         cutHistogramRegistry.Entries().size());

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
  // Evaluate the registered 1-point histogram variables and fill each once.
  // The same callbacks are available to event-by-event cut evaluation.
  cutHistogramRegistry.FillAll(evt);

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


    }
  }

  // MWDC
  for(int i=0; i<N_MWDC; i++){
    for(int j=0; j<N_MWDC_PLANE; j++){
      for(int k=0; k<N_MWDC_WIRE; k++){
	if(evt.mwdc_hit[i][j][k]==1){
	  h_mwdc_hit[i][j]->Fill(k, 1.0);
	}
      }
    }
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
  
  // Booked histograms are not members of vec_th1/vec_th2.
  // Write them exactly once, alongside the original histograms.
  outroot->cd();
  for (const auto &entry : cutHistogramRegistry.Entries()) {
    if (!entry.booked) continue;
    if (entry.dimension == 1) entry.hist1d->Write();
    else entry.hist2d->Write();
  }

  tree->Write();
  outroot->Close();
  return 0;
}

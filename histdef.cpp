#include "analysis.hpp"

const double min_z = -500;
const double max_z = 3500;
const double step_z = 1;
int size_z = (max_z-min_z)/step_z;
const double attpc_z = 2400;


void analysis::HistDef(){
  // A histogram is now booked in one call. The Registry handles normal Fill,
  // Web publication, output writing, reset, and cut-variable lookup.
  // Keep the existing member pointers where other analysis code refers to them.
  const char *ppac_name[N_PPAC] = {"F2U", "F2D", "F3U", "F3D"};

  for (int i = 0; i < N_MADC_CH; ++i) {
    const std::string name  = Form("h_madc%d", i);
    const std::string title = Form("MADC32 ADC ch%02d", i);
    hmadc[i] = cutHistogramRegistry.Book1D(
        name, title, "/MADC", 8196, 0, 8196,
        "madc.adc[" + std::to_string(i) + "]");
  }

  // MADC total takes several ADC samples per event, not one scalar value.
  // Its dedicated Fill logic can be restored separately without any change
  // to histogram creation, HTTP registration, or ROOT output.
  hmadc_total = cutHistogramRegistry.BookManual1D(
      "h_mdac_total", "MADC32 ADC ch16-31(except 28)", "/MADC",
      8196, 0, 8196);

  for (int i = 0; i < N_PPAC; ++i) {
    const std::string n = std::to_string(i);
    const std::string goodName  = Form("h_ppac%d_good", i);
    const std::string goodTitle = Form("%s PPAC good hit", ppac_name[i]);
    hppac_good[i] = cutHistogramRegistry.Book1D(
        goodName, goodTitle, "/PPAC", 2, 0, 2,
        "ppac_good[" + n + "]");

    const std::string condition = "ppac_good[" + n + "]";
    const std::string rawName  = Form("h_ppac%d_pos2d", i);
    const std::string rawTitle = Form("%s PPAC 2D position", ppac_name[i]);
    hppac_pos2d[i] = cutHistogramRegistry.Book2D(
        rawName, rawTitle, "/PPAC", 500, -1000, 1000, 500, -1000, 1000,
        "ppac_pos_raw[" + n + "][0]",
        "ppac_pos_raw[" + n + "][1]",
        condition);

    const std::string calName  = Form("h_ppac%d_pos2d_cal", i);
    const std::string calTitle = Form("%s PPAC 2D position mm", ppac_name[i]);
    hppac_pos2d_cal[i] = cutHistogramRegistry.Book2D(
        calName, calTitle, "/PPAC", 250, -50, 50, 250, -50, 50,
        "ppac_pos_cal[" + n + "][0]",
        "ppac_pos_cal[" + n + "][1]",
        condition);
  }

  h_f2_pos[0] = cutHistogramRegistry.Book1D(
      "h_f2_pos_x", "F2 viewer position X", "/PPAC", 200, -100, 100,
      "f2_pos[0]",
      "ppac_good[0] && ppac_good[1]");
  h_f2_pos[1] = cutHistogramRegistry.Book1D(
      "h_f2_pos_y", "F2 viewer position Y", "/PPAC", 200, -100, 100,
      "f2_pos[1]",
      "ppac_good[0] && ppac_good[1]");

  // Track histograms are multi-fill: one event contributes O(4000) samples.
  // A standard Book2D would additionally auto-fill once per event, which is
  // incorrect, so use the manual booking variant and keep HistFill's loops.
  hppac_track[0][0] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track0", "F2 PPAC track Z-X", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track[0][1] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track1", "F2 PPAC track Z-Y", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track[1][0] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track2", "F3 PPAC track Z-X", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track[1][1] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track3", "F3 PPAC track Z-Y", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track_pid[0][0] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track0_pid", "F2 track Z-X PID", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track_pid[0][1] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track1_pid", "F2 track Z-Y PID", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track_pid[1][0] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track2_pid", "F3 track Z-X PID", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);
  hppac_track_pid[1][1] = cutHistogramRegistry.BookManual2D(
      "h_ppac_track3_pid", "F3 track Z-Y PID", "/PPAC",
      size_z, min_z, max_z, 200, -50, 50);

  const std::string attpcCondition = "ppac_good[2] && ppac_good[3]";
  h_attpc_x = cutHistogramRegistry.Book1D(
      "h_attpc_x", Form("Beam X  pos at ATTPC (z=%d)", (int)attpc_z),
      "/PPAC", 200, -50, 50,
      "ppac_pos_cal[2][0]+2400*ppac_ang[1][0]", attpcCondition);
  h_attpc_y = cutHistogramRegistry.Book1D(
      "h_attpc_y", Form("Beam Y  pos at ATTPC (z=%d)", (int)attpc_z),
      "/PPAC", 200, -50, 50,
      "ppac_pos_cal[2][1]+2400*ppac_ang[1][1]", attpcCondition);
  h_attpc_xy = cutHistogramRegistry.Book2D(
      "h_attpc_xy", Form("Beam XY pos at ATTPC (z=%d)", (int)attpc_z),
      "/PPAC", 200, -50, 50, 200, -50, 50,
      "ppac_pos_cal[2][0]+2400*ppac_ang[1][0]",
      "ppac_pos_cal[2][1]+2400*ppac_ang[1][1]",
      attpcCondition);

  for (int i = 0; i < N_SSD; ++i) {
    const std::string name  = Form("h_ssd_ene%d", i);
    const std::string title = Form("SSD energy ch%d", i);
    hssd_ene[i] = cutHistogramRegistry.Book1D(
        name, title, "/SSD", 512, 0, 60,
        "ssd.ene[" + std::to_string(i) + "]");
  }

  hpid_f2 = cutHistogramRegistry.Book2D(
      "h_pid_f2", "Si energy vs RF F2", "/PID",
      1000, 0, 5000, 500, 0, 60,
      "rf[2]-ref_tdc",
      "ssd.ene[0]");
  hpid_f3 = cutHistogramRegistry.Book2D(
      "h_pid_f3", "Si energy vs RF F3", "/PID",
      1000, 0, 5000, 500, 0, 60,
      "rf[2]-ref_tdc",
      "ssd.ene[1]");

  for (int i = 0; i < 4; ++i) {
    const std::string name  = Form("h_tof_f3ppac%d", i);
    const std::string title = Form("TOF betwen F3PPAC and RF-ch%d", i);
    h_tof_f3ppac[i] = cutHistogramRegistry.Book1D(
        name, title, "/PID", 1000, 4000, 6000,
        "rf[" + std::to_string(i) + "]-ref_tdc");
  }

  // MWDC hit histograms use one Fill per firing wire, not one per event.
  for (int i = 0; i < N_MWDC; ++i) {
    for (int j = 0; j < N_MWDC_PLANE; ++j) {
      const std::string name  = Form("h_mwdc_hit_%d_%d", i, j);
      const std::string title = Form("MWDC %d PL%d Hit", i, j);
      h_mwdc_hit[i][j] = cutHistogramRegistry.BookManual1D(
          name, title, "/MWDC", N_MWDC_WIRE, 0, N_MWDC_WIRE);
    }
  }

  // Keep the two test histograms from the previous one-call patch.
  cutHistogramRegistry.Book1D(
      "h_rf2_ref_booked", "RF ch2 - reference TDC (booked)", "/PID",
      1000, 0, 5000, "rf[2]-ref_tdc");
  cutHistogramRegistry.Book2D(
      "h_rf0_ssd0_booked", "RF ch0 vs SSD0 energy (booked)", "/PID",
      1000, 0, 5000, 500, 0, 60,
      "rf[0]-ref_tdc",
      "ssd.ene[0]");

  cutHistogramRegistryInfo.SetTitle(cutHistogramRegistry.ToJson().c_str());
  printf("Booked %zu histograms in the Registry (including manual multi-fill entries)\n",
         cutHistogramRegistry.Entries().size());
} // end of HistDef()


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

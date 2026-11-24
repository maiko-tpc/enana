#include "analysis.hpp"

bool bResetHist = FALSE;

void analysis::MakeTHttp(int portnum){
  serv = new THttpServer(Form("http:%d?thrds=2;rw", portnum));

  // Navigation header for Cut Editors, with the original ROOT browser below.
  // The original interface remains available at /default.htm.
  serv->SetDefaultPage("./web/cutui/home.html");

  // TCutG editor page
  serv->AddLocation("cutui/", "./web/cutui");

  // The browser writes cut coordinates into this TNamed object's title.
  // The analysis thread validates them and writes the ROOT file.
  serv->Register("/CutEditor", &cutSaveRequest);
  serv->Register("/CutEditor", &cutSaveResult);
  serv->Register("/CutEditor", &cutLoadRequest);
  serv->Register("/CutEditor", &cutLoadResult);
  serv->Register("/CutEditor", &cutListRequest);
  serv->Register("/CutEditor", &cutListResult);
  serv->Register("/CutEditor", &cutApplyRequest);
  serv->Register("/CutEditor", &cutApplyResult);
  serv->Register("/CutEditor", &cutHistogramRegistryInfo);
  serv->Register("/CutEditor", &genericCutRequest);
  serv->Register("/CutEditor", &genericCutResult);
  /* auto refresh every 1 sec */
  serv->SetItemField("/", "_update", "1000");
  serv->SetItemField("/", "_updateperiod", "1000");
  serv->SetItemField("/", "_scan", "5000");
  
  /* Clear button definition */
  string clearcmd = Form("gSystem->Exec(\"kill -%d %d\");", SIGUSR2, pid);
  serv->RegisterCommand("/Commands/Clear", clearcmd.c_str(),
  			"button;rootsys/icons/refresh.png");

  for(int i=0; i<N_MADC_CH; i++){
    serv->Register("/MADC", hmadc[i]);
  }
  serv->Register("/MADC", hmadc_total);
  
  for(int i=0; i<4; i++){
    serv->Register("/PPAC", hppac_good[i]);
  }
  for(int i=0; i<4; i++){
    serv->Register("/PPAC", hppac_pos2d[i]);
  }
  for(int i=0; i<4; i++){
    serv->Register("/PPAC", hppac_pos2d_cal[i]);
  }

  for(int i=0; i<2; i++){
    for(int j=0; j<2; j++){
      serv->Register("/PPAC", hppac_track[i][j]);
    }
  }
  for(int i=0; i<2; i++){
    for(int j=0; j<2; j++){
      serv->Register("/PPAC", hppac_track_pid[i][j]);
    }
  }

  serv->Register("/PPAC", h_f2_pos[0]);
  serv->Register("/PPAC", h_f2_pos[1]);  

  serv->Register("/PPAC", h_attpc_x);
  serv->Register("/PPAC", h_attpc_y);
  serv->Register("/PPAC", h_attpc_xy);    
  
  for(int i=0; i<N_SSD; i++){
    serv->Register("/SSD", hssd_ene[i]);
  }

  serv->Register("/PID", hpid_f2);
  serv->Register("/PID", hpid_f3);  

  for(int i=0; i<4; i++){
    serv->Register("/PID", h_tof_f3ppac[i]);
  }
  
  // MWDC
  for(int i=0; i<N_MWDC; i++){
    for(int j=0; j<N_MWDC_PLANE; j++){
      serv->Register("/MWDC", h_mwdc_hit[i][j]);
    }
  }

  // Auto-publish histograms created by Book1D()/Book2D().
  // Existing registered histograms use the manual registrations above,
  // so none of them are registered twice.
  for (const auto &entry : cutHistogramRegistry.Entries()) {
    if (!entry.booked) continue;
    const auto sep = entry.path.find_last_of('/');
    const std::string group = sep == 0 ? "/" : entry.path.substr(0, sep);
    if (entry.dimension == 1)
      serv->Register(group.c_str(), entry.hist1d);
    else
      serv->Register(group.c_str(), entry.hist2d);
  }
} // end of MakeHttp()

void analysis::HttpHistReset(){
  printf("clear function\n");

  int size1 = vec_th1.size();
  for(int i=0; i<size1; i++){
    vec_th1[i]->Reset();
  }

  int size2 = vec_th2.size();
  for(int i=0; i<size2; i++){
    vec_th2[i]->Reset();
  }
  // Histograms booked through the Registry are not in vec_th1/vec_th2.
  for (const auto &entry : cutHistogramRegistry.Entries()) {
    if (!entry.booked) continue;
    if (entry.dimension == 1) entry.hist1d->Reset();
    else entry.hist2d->Reset();
  }
}

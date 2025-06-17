#include "analysis.hpp"

bool bResetHist = FALSE;

void analysis::MakeTHttp(int portnum){
  serv = new THttpServer(Form("http:%d?thrds=2;rw", portnum));

  /* Clear button definition */
  string clearcmd = Form("gSystem->Exec(\"kill -%d %d\");", SIGUSR2, pid);
  serv->RegisterCommand("/Commands/Clear", clearcmd.c_str(),
  			"button;rootsys/icons/refresh.png");

  for(int i=0; i<N_MADC_CH; i++){
    serv->Register("/MADC", hmadc[i]);
  }

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

  serv->Register("/PPAC", h_attpc_x);
  serv->Register("/PPAC", h_attpc_y);
  serv->Register("/PPAC", h_attpc_xy);    
  
  for(int i=0; i<N_SSD; i++){
    serv->Register("/SSD", hssd_ene[i]);
  }

  serv->Register("/PID", hpid_f2);
  serv->Register("/PID", hpid_f3);  

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
}

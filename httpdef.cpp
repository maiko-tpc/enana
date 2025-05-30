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
}

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

#include "analysis.hpp"

bool bResetHist = FALSE;

void analysis::MakeTHttp(int portnum){
  serv = new THttpServer(Form("http:%d?thrds=2;rw", portnum));

  /* Clear button definition */
  //  string clearcmd = Form("gSystem->Exec(\"kill -%d %d\");", SIGUSR2, pid);
  //  serv->RegisterCommand("/Commands/Clear", clearcmd.c_str(),
  //			"button;rootsys/icons/refresh.png");

  for(int i=0; i<N_MADC_CH; i++){
    serv->Register("/MADC", hmadc[i]);
  }
}

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

  // All histogram Web registrations are derived from Book* paths.
  // Manual multi-point histograms are also published by this loop.
  // Auto-publish histograms created by Book1D()/Book2D().
  // All histograms were booked in one place; publish each exactly once.
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

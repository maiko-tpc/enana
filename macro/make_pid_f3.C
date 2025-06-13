{
  string pidfname = "macro/pid_f3.root";

  TCutG *grpid;
  //  grpid = (TCutG*)gROOT->GetListOfSpecials()->FindObject("CUTG"); // for ROOT v5
  grpid = (TCutG*)gROOT->FindObject("CUTG"); // for ROOT v6

  if(grpid==NULL){
    printf("No CUTG found!\n");
    printf("First, make CUTG object by View-->Toolbar-->Graphical Cut\n");
  }

  if(grpid!=NULL){
    grpid->SetName("pidcut_f3");
    TFile *file_grpid = new TFile(pidfname.c_str(), "RECREATE");
    grpid->Write();
    file_grpid->Close();
    printf("Saved PID file: %s\n", pidfname.c_str());

  }
}

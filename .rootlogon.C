{
  gStyle->SetPadGridX(1);
  gStyle->SetPadGridY(1);  

  // read the F2 PID files
  string fname_pidf2 = "macro/pid_f2.root";
  TFile *fpidf2 = new TFile(fname_pidf2.c_str());
  if(fpidf2->IsOpen() == false) printf("No F2 PID file found: %s\n", fname_pidf2.c_str());

  if(fpidf2->IsOpen() == true){    
    TCutG *pidf2 = (TCutG*)fpidf2->Get("pidcut_f2");
    pidf2->SetLineColor(2);
    pidf2->SetLineWidth(2);    
    printf("\n------------\n");
    printf("F2 PID cut name: %s\n", pidf2->GetName());
    printf("PID Par X: %s\n", pidf2->GetVarX());
    printf("PID Par Y: %s\n", pidf2->GetVarY());    
    printf("\n------------\n\n");    
  }


  // read the F3 PID files
  string fname_pidf3 = "macro/pid_f3.root";
  TFile *fpidf3 = new TFile(fname_pidf3.c_str());
  if(fpidf3->IsOpen() == false) printf("No F3 PID file found: %s\n", fname_pidf3.c_str());

  if(fpidf3->IsOpen() == true){
    TCutG *pidf3 = (TCutG*)fpidf3->Get("pidcut_f3");
    pidf3->SetLineColor(2);
    pidf3->SetLineWidth(2);    
    printf("\n------------\n");
    printf("F3 PID cut name: %s\n", pidf3->GetName());
    printf("PID Par X: %s\n", pidf3->GetVarX());
    printf("PID Par Y: %s\n", pidf3->GetVarY());    
    printf("\n------------\n\n");    
  }
}

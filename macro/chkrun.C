{
  TCanvas c2;
  c2.Divide(2,2);

  c2.cd(1);
  h_ppac_track2->Draw("colz");

  c2.cd(2);
  h_ppac_track3->Draw("colz");

  c2.cd(3);
  tree->Draw("rf[3]-ref_tdc>>h1(500,4000,6000)");

  c2.cd(4);
  h_attpc_xy->Draw("colz");

  c2.SaveAs("pic.pdf");

  TString file = "pic.pdf";
  TString printer = "rcc2570a";
    
  char ans;
  std::cout << "Print " << file << " ? (y/n): ";
  std::cin >> ans;
  
  if (ans == 'y' || ans == 'Y') {
    TString cmd = Form("lpr -P %s %s", printer.Data(), file.Data());
    int ret = gSystem->Exec(cmd);
    if (ret == 0)
      std::cout << "Printed successfully on " << printer << std::endl;
        else
	  std::cout << "Error printing (code " << ret << ")" << std::endl;
  } else {
    std::cout << "Printing canceled." << std::endl;
  }
}


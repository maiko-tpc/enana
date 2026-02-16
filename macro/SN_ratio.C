{
  //Calcurate S/N ratio of secondary beam
  TCanvas c1;
  tree-> Draw("rf[3]-ref_tdc >>h1(500, 4000, 6000)", "", "");

  double SXmin=4200; //low Energy edge of Signal peak ()
  double SXmax=4400; //high Energy edge of Signal peak () 
  int bin1= h1->GetXaxis()->FindBin(SXmin);
  int bin2= h1->GetXaxis()->FindBin(SXmax);
  int Signal= h1->Integral(bin1, bin2);

  double NXmin=4400; //low Energy edge of Noise peak ()
  double NXmax=4600; //high Energy edge of Noise peak ()
  int bin3= h1->GetXaxis()->FindBin(NXmin);
  int bin4= h1->GetXaxis()->FindBin(NXmax);
  int Noise= h1->Integral(bin3, bin4); 
  cout << "S/N = " <<(double)Signal / Noise<<endl;
}

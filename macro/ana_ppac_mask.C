{
  TFile *infile = new TFile("root/run0009.root");

  vector<float> xrange = {-350, -250, -180, -90, 10, 110, 210, 300, 380};
  vector<float> yrange = {-350, -250, -150, -50, 50, 150, 250, 350};  

  double x_min = xrange[0];
  double x_max = xrange.back();  

  double y_min = yrange[0];
  double y_max = yrange.back();  

  tree->Draw("ppac_pos_raw[][1]:ppac_pos_raw[][0]>>h1(200,-1000,1000,200,-1000,1000)",
	     "","colz");
}

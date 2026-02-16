void gamma() {
    TString filename = "../GAGG_137Cs_370V.root";  // <-- change this
    TFile *f = TFile::Open(filename, "READ");
    //TTree *t = (TTree*)f->Get("tree");

    TCanvas *c = new TCanvas("c", "MADC32 Channels", 1600, 1400);
    c->Divide(4, 4);  // 16 histograms (channels 16–31)

    for (int ch = 16; ch <= 31; ++ch) {
    	//TH1D* h2 = new TH1D(("h2_%d", ch), Form("madc[%d] Spectrum", ch), 200,200,2200);
        c->cd(ch - 15); // place in pad 1–16

        TString hname = Form("h_madc%d", ch);
        TH1F *h = (TH1F*) f->Get(hname);
        if (!h) {
            printf("Warning: histogram %s not found!\n", hname.Data());
            continue;
        }

        h->GetXaxis()->SetRangeUser(200,1800);
        h->SetLineColor(kBlue+1);
        h->SetLineWidth(2);
        h->GetXaxis()->SetTitle("ADC");
        h->GetYaxis()->SetTitle("Counts");
        //h->Draw();

        TF1 *fit = new TF1(Form("fit_%d", ch), "gaus");
        h->Fit(fit, "Q", "", 1000, 1500); 

        // Print fit results
        double mean  = fit->GetParameter(1);
        double sigma = fit->GetParameter(2);
        printf("Channel %2d: mean = %8.2f, sigma = %8.2f\n", ch, mean, sigma);
    }

    c->Update();
}


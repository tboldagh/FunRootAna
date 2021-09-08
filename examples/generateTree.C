{
  TFile f("TestTree.root", "RECREATE"); 
  TTree * t = new TTree("points", "Example tree with points");
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> z;
  int category = 0;
  t->Branch("x", &x);
  t->Branch("y", &y);
  t->Branch("z", &z);

  t->Branch("category", &category);

  for ( int i = 0; i < 1000; ++i ) {
      x.clear();
      for ( int p = 0; p < gRandom->Integer(100); ++p) {
        x.push_back(gRandom->Exp(25));
        y.push_back(gRandom->Exp(20+p));
        z.push_back(gRandom->Gaus(0, 2));
      }
      category = gRandom->Gaus( *std::max_element(z.begin(), z.end())/2.0, 1);
      if ( category < 0 ) category = 0;
      if (category > 4 ) category = 4;

    t->Fill();
  }
  f.Write();
  f.Close();
  std::cout << "... generated " << std::endl;
  exit(0);
}